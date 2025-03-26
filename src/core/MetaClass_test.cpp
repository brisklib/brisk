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
#include <brisk/core/MetaClass.hpp>
#include <catch2/catch_all.hpp>
#include "Catch2Utils.hpp"

namespace Brisk {

namespace {

// Test classes
class Base {
    BRISK_DYNAMIC_CLASS_ROOT(Base)
public:
    virtual ~Base() = default;
};

class Derived : public Base {
    BRISK_DYNAMIC_CLASS(Derived, Base)
};

class DerivedFurther : public Derived {
    BRISK_DYNAMIC_CLASS(DerivedFurther, Derived)
};

class Unrelated {
    BRISK_DYNAMIC_CLASS_ROOT(Unrelated)
};

} // namespace

TEST_CASE("dynamicCast functionality") {
    SECTION("Casting to same type") {
        Derived d;
        Derived* dPtr = &d;
        auto result   = dynamicCast<Derived*>(&d);
        REQUIRE(result == dPtr);
    }

    SECTION("Upcasting to base") {
        Derived d;
        Base* bPtr = dynamicCast<Base*>(&d);
        REQUIRE(bPtr == static_cast<Base*>(&d));
    }

    SECTION("Upcasting multiple levels") {
        DerivedFurther df;
        Base* bPtr = dynamicCast<Base*>(&df);
        REQUIRE(bPtr == static_cast<Base*>(&df));
    }

    SECTION("Downcasting to derived") {
        Derived d;
        Base* bPtr    = &d;
        Derived* dPtr = dynamicCast<Derived*>(bPtr);
        REQUIRE(dPtr == &d);
    }

    SECTION("Downcasting multiple levels") {
        DerivedFurther df;
        Base* bPtr            = &df;
        DerivedFurther* dfPtr = dynamicCast<DerivedFurther*>(bPtr);
        REQUIRE(dfPtr == &df);
    }

    SECTION("Null pointer handling") {
        Base* nullPtr = nullptr;
        Derived* dPtr = dynamicCast<Derived*>(nullPtr);
        REQUIRE(dPtr == nullptr);
    }
}

TEST_CASE("Class name verification") {
    SECTION("Root class name") {
        Base b;
        REQUIRE(b.dynamicMetaClass()->className == "Base");
        REQUIRE(b.staticMetaClass()->className == "Base");
    }

    SECTION("Derived class name") {
        Derived d;
        REQUIRE(d.dynamicMetaClass()->className == "Derived");
        REQUIRE(d.staticMetaClass()->className == "Derived");
    }

    SECTION("Multiple inheritance levels") {
        DerivedFurther df;
        REQUIRE(df.dynamicMetaClass()->className == "DerivedFurther");
        REQUIRE(df.staticMetaClass()->className == "DerivedFurther");
    }
}

TEST_CASE("isOf functionality") {
    SECTION("Same type") {
        Derived d;
        REQUIRE(isOf<Derived>(&d) == true);
    }

    SECTION("Base type") {
        Derived d;
        REQUIRE(isOf<Base>(&d) == true);
    }

    SECTION("Multiple levels up") {
        DerivedFurther df;
        REQUIRE(isOf<Base>(&df) == true);
        REQUIRE(isOf<Derived>(&df) == true);
    }

    SECTION("Unrelated type") {
        Derived d;
        REQUIRE(isOf<Unrelated>(&d) == false);
    }

    SECTION("Null pointer") {
        Base* nullPtr = nullptr;
        REQUIRE(isOf<Derived>(nullPtr) == false);
    }
}

TEST_CASE("Inheritance hierarchy") {
    DerivedFurther df;
    const MetaClass* meta = df.dynamicMetaClass();

    SECTION("DerivedFurther level") {
        REQUIRE(meta->className == "DerivedFurther");
        meta = meta->classBase;
    }

    SECTION("Derived level") {
        REQUIRE(meta->classBase->className == "Derived");
        meta = meta->classBase->classBase;
    }

    SECTION("Base level") {
        REQUIRE(meta->classBase->classBase->className == "Base");
    }

    SECTION("Root reached") {
        REQUIRE(meta->classBase->classBase->classBase == nullptr);
    }
}
} // namespace Brisk
