#include <brisk/network/Fetch.hpp>
#include <brisk/network/UserAgent.hpp>
#include <curl/curl.h>
#include <brisk/core/Log.hpp>
#include <brisk/core/Utilities.hpp>
#include <brisk/core/Text.hpp>

namespace Brisk {

struct CurlInit {
    CurlInit() {
        curl_global_init(CURL_GLOBAL_DEFAULT);
    }

    ~CurlInit() {
        curl_global_cleanup();
    }
};

static CurlInit init;

void setupCURLE(void* c) {
    CURL* curl = reinterpret_cast<CURL*>(c);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, httpUserAgent.value_or(defaultHttpUserAgent()).c_str());
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
#ifdef ALLOW_LOCAL_SERVERS
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
#else
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1);
#endif
}

static size_t curlWriter(void* ptr, size_t size, size_t nmemb, Stream* stream) {
    size_t totalSize = size * nmemb;
    return stream->write((const uint8_t*)ptr, totalSize).bytes();
}

static size_t curlReader(void* ptr, size_t size, size_t nmemb, Stream* stream) {
    size_t totalSize = size * nmemb;
    return stream->read((uint8_t*)ptr, totalSize).bytes();
}

static size_t curlHeaderWriter(char* ptr, size_t size, size_t nmemb, std::string* headers) {
    headers->append(ptr, nmemb * size);
    return nmemb * size;
}

static int curlProgress(function<void(intmax_t, intmax_t)>* progress, curl_off_t dltotal, curl_off_t dlnow,
                        curl_off_t ultotal, curl_off_t ulnow) {
    (*progress)(static_cast<intmax_t>(dlnow), static_cast<intmax_t>(dltotal));
    return 0;
}

[[nodiscard]] HTTPResponse httpFetch(const HTTPRequest& request, RC<Stream> requestBody,
                                     RC<Stream> responseBody) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        return HTTPResponse{};
    }
    SCOPE_EXIT {
        curl_easy_cleanup(curl);
    };
    LOG_TRACE(network, "libCURL {}", curl_version());
    std::string headers{};

    curl_easy_setopt(curl, CURLOPT_URL, request.url.c_str());
    setupCURLE(curl);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &curlWriter);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, responseBody.get());
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, &curlHeaderWriter);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, &headers);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, request.followLocation ? 1 : 0);
    if (!request.referer.empty())
        curl_easy_setopt(curl, CURLOPT_REFERER, request.referer.c_str());
    if (request.progressCallback) {
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
        curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, &curlProgress);
        curl_easy_setopt(curl, CURLOPT_XFERINFODATA, &request.progressCallback);
    }
    if (requestBody) {
        curl_easy_setopt(curl, CURLOPT_READFUNCTION, &curlReader);
        curl_easy_setopt(curl, CURLOPT_READDATA, requestBody.get());
        uintmax_t size = requestBody->size();
        if (size != invalidSize) {
            curl_easy_setopt(curl,
                             request.method == HTTPMethod::Put ? CURLOPT_INFILESIZE_LARGE
                                                               : CURLOPT_POSTFIELDSIZE_LARGE,
                             curl_off_t(size));
        } else {
            // curl 7.66+ automatically uses chunked encoding if size is unknown
        }
    }
    curl_slist* headerList = NULL;
    SCOPE_EXIT {
        curl_slist_free_all(headerList);
    };
    if (!request.headers.empty()) {
        for (const std::string& h : request.headers) {
            headerList = curl_slist_append(headerList, h.c_str());
        }
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerList);
    }
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, long(request.timeout.count()));
    switch (request.method) {
    case HTTPMethod::Auto:
    case HTTPMethod::Get:
        break;
    case HTTPMethod::Post:
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        break;
    case HTTPMethod::Put:
        curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
        break;
    case HTTPMethod::Head:
        curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
        break;
    case HTTPMethod::Delete:
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
        break;
    case HTTPMethod::Patch:
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PATCH");
        break;
    default:
        BRISK_UNREACHABLE();
    }
    switch (request.authentication.index()) {
    case 0: // no auth
        break;
    case 1: { // basic
        auto& basic = std::get<HTTPBasicAuth>(request.authentication);
        curl_easy_setopt(curl, CURLOPT_USERNAME, basic.username.c_str());
        curl_easy_setopt(curl, CURLOPT_PASSWORD, basic.password.c_str());
        break;
    }
    case 2: { // bearer
        auto& bearer = std::get<HTTPBearerAuth>(request.authentication);
        curl_easy_setopt(curl, CURLOPT_XOAUTH2_BEARER, bearer.token.c_str());
        break;
    }
    default:
        BRISK_UNREACHABLE();
    }

    HTTPResponse response;

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLcode::CURLE_OK) {
        response.error = static_cast<FetchErrorCode>(res);
    }
    long verifyresult;
    curl_easy_getinfo(curl, CURLINFO_SSL_VERIFYRESULT, &verifyresult);

    const char* effUrl = NULL;
    if (curl_easy_getinfo(curl, CURLINFO_EFFECTIVE_URL, &effUrl) == CURLcode::CURLE_OK) {
        response.effectiveUrl = effUrl;
    }

    long code = 0;
    if (curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code) == CURLcode::CURLE_OK) {
        response.httpCode = code;
    }

    if (!headers.empty()) {
        response.headers = toStrings(split(headers, "\r\n"));
    }

    return response;
}

namespace Internal {
std::string fetchErrorCodeString(FetchErrorCode code) {
    return curl_easy_strerror(static_cast<CURLcode>(code));
}
} // namespace Internal

std::pair<HTTPResponse, Bytes> httpFetchBytes(const HTTPRequest& request) {
    RC<MemoryStream> memory = rcnew MemoryStream{};
    HTTPResponse response   = httpFetch(request, nullptr, memory);
    return { std::move(response), std::move(memory->data()) };
}
} // namespace Brisk
