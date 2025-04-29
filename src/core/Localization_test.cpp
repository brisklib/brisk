/*
 * Brisk
 *
 * Cross-platform application framework
 * --------------------------------------------------------------
 *
 * Copyright (C) 2024 Brisk Developers
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
 */
#include <brisk/core/Localization.hpp>
#include <catch2/catch_all.hpp>
#include "Catch2Utils.hpp"

namespace Brisk {

TEST_CASE("_tr udl") {
    CHECK("abc"_tr == "abc"s);
    CHECK("abc||ctx"_tr == "abc"s);

    CHECK("Page {0} of {1}"_trfmt(2, 12) == "Page 2 of 12"s);
    CHECK("Page {0} of {1}||ctx"_trfmt(2, 12) == "Page 2 of 12"s);

    CHECK("Page {0} of {1}||ctx {2}"_trfmt(2, 12) == "Page 2 of 12"s);

    // CHECK("Page {} of {}||ctx"_trfmt() == "Page 2 of 12");
}

TEST_CASE("translation") {

    CHECK("Copy"_tr == "Copy"s);
    CHECK("Cut"_tr == "Cut"s);
    CHECK("Paste"_tr == "Paste"s);

    Rc<const Locale> savedLocale = locale;
    Rc<SimpleLocale> esLocale    = rcnew SimpleLocale();
    locale                       = esLocale;

    CHECK("Copy"_tr == "Copy"s);
    CHECK("Cut"_tr == "Cut"s);
    CHECK("Paste"_tr == "Paste"s);

    esLocale->addTranslation("Copy", "Copiar");
    esLocale->addTranslation("Cut", "Cortar");
    esLocale->addTranslation("Paste", "Pegar");

    CHECK("Copy"_tr == "Copiar"s);
    CHECK("Cut"_tr == "Cortar"s);
    CHECK("Paste"_tr == "Pegar"s);

    locale = std::move(savedLocale);

    CHECK("Copy"_tr == "Copy"s);
    CHECK("Cut"_tr == "Cut"s);
    CHECK("Paste"_tr == "Paste"s);
}
} // namespace Brisk
