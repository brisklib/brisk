/*
 * Brisk
 *
 * Cross-platform application framework
 * --------------------------------------------------------------
 *
 * Copyright (C) 2025 Brisk Developers
 *
 * This file is part of the Brisk library.
 *
 * Brisk is dual-licensed under the GNU General Public License, version 2 (GPL-2.0+),
 * and a commercial license. You may use, modify, and distribute this software under
 * the terms of the GPL-2.0+ license if you comply with its conditions.
 *
 * You should have received a copy of the GNU General Public License along with this program.
 * If not, see <http://www.gnu.org/licenses/>.
 *
 * If you do not wish to be bound by the GPL-2.0+ license, you must purchase a commercial
 * license. For commercial licensing options, please visit: https://brisklib.com
 */                                                                                                          \
#pragma once
#include <brisk/core/BasicTypes.hpp>
#include <brisk/core/Bytes.hpp>
#include <brisk/core/Io.hpp>
#include <brisk/core/internal/Function.hpp>
#include <brisk/core/Cryptography.hpp>

namespace Brisk {

/**
 * @brief Enum representing different HTTP methods.
 */
enum class HttpMethod {
    Auto,   ///< HTTP POST if requestBody is not null, GET otherwise.
    Get,    ///< HTTP GET method.
    Post,   ///< HTTP POST method.
    Put,    ///< HTTP PUT method.
    Head,   ///< HTTP HEAD method.
    Delete, ///< HTTP DELETE method.
    Patch   ///< HTTP PATCH method.
};

/**
 * @brief Structure representing HTTP Basic Authentication.
 */
struct HttpBasicAuth {
    std::string username; ///< Username for basic authentication.
    std::string password; ///< Password for basic authentication.
};

/**
 * @brief Structure representing HTTP Bearer Authentication.
 */
struct HttpBearerAuth {
    std::string token; ///< Bearer token for authentication.
};

/**
 * @brief Structure representing an HTTP request.
 */
struct HttpRequest {
    std::string url;                                     ///< The URL for the HTTP request.
    HttpMethod method                = HttpMethod::Auto; ///< The HTTP method for the request.
    std::string referer              = {};               ///< Optional Referer header.
    std::vector<std::string> headers = {};               ///< Additional headers for the request.
    bool followLocation              = true;             ///< Whether to follow redirects.
    std::variant<std::monostate, HttpBasicAuth, HttpBearerAuth> authentication; ///< Authentication options.
    std::chrono::milliseconds timeout = std::chrono::milliseconds(5000);        ///< Request timeout duration.
    function<void(int64_t, int64_t)> progressCallback =
        nullptr; ///< Callback function for reporting progress.
};

/**
 * @brief Enum representing different fetch error codes for HTTP requests.
 */
enum class FetchErrorCode {
    ok = 0,                  ///< No error, successful operation.
    unsupportedProtocol,     ///< Unsupported protocol.
    failedInit,              ///< internal/Initialization failure.
    urlMalformat,            ///< Malformed URL.
    notBuiltIn,              ///< Functionality not built-in.
    couldntResolveProxy,     ///< Could not resolve proxy.
    couldntResolveHost,      ///< Could not resolve host.
    couldntConnect,          ///< Could not connect to the server.
    weirdServerReply,        ///< Unexpected server reply.
    remoteAccessDenied,      ///< Access denied by the server.
    ftpAcceptFailed,         ///< FTP accept failed.
    ftpWeirdPassReply,       ///< Weird FTP password reply.
    ftpAcceptTimeout,        ///< FTP accept timeout.
    ftpWeirdPasvReply,       ///< Weird FTP PASV reply.
    ftpWeird227Format,       ///< Weird FTP 227 response format.
    ftpCantGetHost,          ///< Could not retrieve host from FTP.
    http2,                   ///< HTTP/2 framing layer problem.
    ftpCouldntSetType,       ///< Could not set FTP transfer type.
    partialFile,             ///< Partial file transfer.
    ftpCouldntRetrFile,      ///< Could not retrieve FTP file.
    obsolete20,              ///< Obsolete error code (not used).
    quoteError,              ///< Failure in executing quote command.
    httpReturnedError,       ///< HTTP server returned an error.
    writeError,              ///< Error writing data.
    obsolete24,              ///< Obsolete error code (not used).
    uploadFailed,            ///< Upload failed.
    readError,               ///< Error reading file.
    outOfMemory,             ///< Out of memory error.
    operationTimedout,       ///< Operation timed out.
    obsolete29,              ///< Obsolete error code (not used).
    ftpPortFailed,           ///< FTP PORT command failed.
    ftpCouldntUseRest,       ///< FTP REST command failed.
    obsolete32,              ///< Obsolete error code (not used).
    rangeError,              ///< RANGE command failed.
    httpPostError,           ///< HTTP POST request failed.
    sslConnectError,         ///< SSL connection error.
    badDownloadResume,       ///< Failed to resume download.
    fileCouldntReadFile,     ///< Could not read file.
    ldapCannotBind,          ///< LDAP bind operation failed.
    ldapSearchFailed,        ///< LDAP search operation failed.
    obsolete40,              ///< Obsolete error code (not used).
    functionNotFound,        ///< Function not found.
    abortedByCallback,       ///< Operation aborted by callback.
    badFunctionArgument,     ///< Bad function argument.
    obsolete44,              ///< Obsolete error code (not used).
    interfaceFailed,         ///< Interface operation failed.
    obsolete46,              ///< Obsolete error code (not used).
    tooManyRedirects,        ///< Too many redirects.
    unknownOption,           ///< Unknown option specified.
    setoptOptionSyntax,      ///< Syntax error in setopt option.
    obsolete50,              ///< Obsolete error code (not used).
    obsolete51,              ///< Obsolete error code (not used).
    gotNothing,              ///< No data received.
    sslEngineNotfound,       ///< SSL engine not found.
    sslEngineSetfailed,      ///< Failed to set SSL engine.
    sendError,               ///< Error sending data.
    recvError,               ///< Error receiving data.
    obsolete57,              ///< Obsolete error code (not used).
    sslCertproblem,          ///< Problem with local certificate.
    sslCipher,               ///< Could not use specified SSL cipher.
    peerFailedVerification,  ///< Peer certificate verification failed.
    badContentEncoding,      ///< Unrecognized content encoding.
    obsolete62,              ///< Obsolete error code (not used).
    filesizeExceeded,        ///< File size exceeded the maximum allowed.
    useSslFailed,            ///< Failed to use SSL.
    sendFailRewind,          ///< Failed to rewind and send data.
    sslEngineInitfailed,     ///< SSL engine initialization failed.
    loginDenied,             ///< Login denied (user/password incorrect).
    tftpNotfound,            ///< TFTP file not found.
    tftpPerm,                ///< TFTP permission denied.
    remoteDiskFull,          ///< Remote disk full.
    tftpIllegal,             ///< Illegal TFTP operation.
    tftpUnknownid,           ///< Unknown TFTP transfer ID.
    remoteFileExists,        ///< Remote file already exists.
    tftpNosuchuser,          ///< No such TFTP user.
    obsolete75,              ///< Obsolete error code (not used).
    obsolete76,              ///< Obsolete error code (not used).
    sslCacertBadfile,        ///< Could not load CA certificate file.
    remoteFileNotFound,      ///< Remote file not found.
    ssh,                     ///< SSH layer error.
    sslShutdownFailed,       ///< Failed to shut down SSL connection.
    again,                   ///< Socket is not ready, try again later.
    sslCrlBadfile,           ///< Could not load SSL CRL file.
    sslIssuerError,          ///< SSL issuer verification failed.
    ftpPretFailed,           ///< FTP PRET command failed.
    rtspCseqError,           ///< RTSP CSeq mismatch.
    rtspSessionError,        ///< RTSP session ID mismatch.
    ftpBadFileList,          ///< FTP file list parsing failed.
    chunkFailed,             ///< Chunk callback error.
    noConnectionAvailable,   ///< No connection available.
    sslPinnedpubkeynotmatch, ///< Pinned public key did not match.
    sslInvalidcertstatus,    ///< Invalid SSL certificate status.
    http2Stream,             ///< HTTP/2 stream error.
    recursiveApiCall,        ///< API function called recursively.
    authError,               ///< Authentication function returned error.
    http3,                   ///< HTTP/3 layer problem.
    quicConnectError,        ///< QUIC connection error.
    proxy,                   ///< Proxy handshake error.
    sslClientcert,           ///< Client-side certificate required.
    unrecoverablePoll,       ///< Fatal poll/select error.
    tooLarge                 ///< Data exceeded its maximum allowed size.
};

/**
 * @brief Structure representing an HTTP response.
 */
struct HttpResponse {
    FetchErrorCode error;                    ///< Error code indicating the result of the request.
    std::optional<int> httpCode;             ///< Optional HTTP response code.
    std::optional<std::string> effectiveUrl; ///< Optional effective URL after redirects.
    std::vector<std::string> headers;        ///< Response headers.

    /**
     * @brief Checks if the HTTP response indicates a successful request.
     *
     * @return true if the response is successful (HTTP 2xx), false otherwise.
     */
    bool ok() const noexcept {
        return error == FetchErrorCode::ok && httpCode.has_value() && *httpCode >= 200 && *httpCode <= 299;
    }

    /**
     * @brief Checks if the HTTP response indicates a successful request.
     *
     * @return true if the response is successful (HTTP 2xx), false otherwise.
     */
    explicit operator bool() const noexcept {
        return ok();
    }
};

/**
 * @brief Makes an HTTP request.
 *
 * @param request The HttpRequest object containing the request details.
 * @param requestBody The body of the request (if applicable).
 * @param responseBody The stream to store the response body.
 *
 * @return HttpResponse object containing the result of the request.
 */
[[nodiscard]] HttpResponse httpFetch(const HttpRequest& request, Rc<Stream> requestBody,
                                     Rc<Stream> responseBody);

/**
 * @brief Makes an HTTP request and returns the response body as bytes.
 *
 * @param request The HttpRequest object containing the request details.
 *
 * @return A pair containing the HttpResponse and the response body in bytes.
 */
[[nodiscard]] std::pair<HttpResponse, Bytes> httpFetchBytes(const HttpRequest& request);

namespace Internal {
/**
 * @brief Converts a FetchErrorCode to a string representation.
 *
 * @param code The FetchErrorCode to convert.
 *
 * @return A string representing the error code.
 */
std::string fetchErrorCodeString(FetchErrorCode code);
} // namespace Internal

} // namespace Brisk

template <>
struct fmt::formatter<Brisk::FetchErrorCode> : fmt::formatter<std::string> {
    template <typename FormatContext>
    auto format(Brisk::FetchErrorCode value, FormatContext& ctx) const {
        return fmt::formatter<std::string>::format(Brisk::Internal::fetchErrorCodeString(value), ctx);
    }
};
