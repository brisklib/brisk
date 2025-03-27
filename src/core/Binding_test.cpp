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
#include <brisk/core/Binding.hpp>
#include <catch2/catch_all.hpp>
#include "Catch2Utils.hpp"

namespace Brisk {

// Test the basic binding functionality and updates between regions
TEST_CASE("Bindings") {
    REQUIRE(bindings->numRegions() == 0);

    float src = 125;
    float dst = 0.f;

    bindings->registerRegion(toBindingAddress(&src), nullptr);
    bindings->registerRegion(toBindingAddress(&dst), nullptr);
    CHECK(bindings->numRegions() == 2);

    auto h1 = bindings->connectBidir(Value{ &dst },
                                     Value{ &src }.transform(
                                         [](auto x) {
                                             return x * 2;
                                         },
                                         [](auto x) {
                                             return x / 2;
                                         }),
                                     BindType::Immediate, false);
    CHECK(bindings->numHandlers() == 2);

    CHECK(src == 125);
    CHECK(dst == 0);

    bindings->notify(&src);

    CHECK(src == 125);
    CHECK(dst == 250);

    bindings->assign(dst) = 1000;

    CHECK(src == 500);
    CHECK(dst == 1000);

    ++bindings->assign(dst);

    CHECK(src == 500.5);
    CHECK(dst == 1001);

    ++bindings->assign(dst);

    CHECK(src == 501);
    CHECK(dst == 1002);

    bindings->disconnect(h1);

    CHECK(bindings->numHandlers() == 0);

    ++bindings->assign(dst);

    CHECK(src == 501);
    CHECK(dst == 1003);

    bindings->unregisterRegion(toBindingAddress(&src));
    bindings->unregisterRegion(toBindingAddress(&dst));
    CHECK(bindings->numRegions() == 0);
}

// Test handling of constant bindings and ensuring no handlers are created
TEST_CASE("Binding constant") {
    auto src = Value<int>::constant(123);
    int dest;
    BindingRegistration lt{ &dest, nullptr };

    CHECK(bindings->numHandlers() == 0);

    BindingHandle h = bindings->connect(Value{ &dest }, src);
    CHECK(dest == 123);
    CHECK(!h);

    CHECK(bindings->numHandlers() == 0);
}

struct BObject {
    int value = 0;
    BindingRegistration lt{ this, nullptr };
};

// Test the registration and deregistration of bindings with object lifetime
TEST_CASE("BindingRegistration") {
    {
        std::unique_ptr<BObject> b(new BObject{ 2 });

        int receiver = 0;
        BindingRegistration receiverLt{ &receiver, nullptr };

        bindings->connectBidir(Value{ &receiver }, Value{ &b->value }, BindType::Immediate, false);

        CHECK(bindings->numHandlers() == 2);
        CHECK(bindings->numRegions() == 2);

        b.reset();

        CHECK(bindings->numHandlers() == 0);
        CHECK(bindings->numRegions() == 1);
    }

    CHECK(bindings->numRegions() == 0);
}

// Test reentrancy in binding notifications
TEST_CASE("Binding: reentrancy1") {
    int source = 0;
    BindingRegistration sourceLt{ &source, mainScheduler };
    BindingHandle h1;
    int listenerCalled = 0;

    h1                 = bindings->connect(Value<int>::listener(
                               [&]() {
                                   ++listenerCalled; // Increment the counter when the listener is called
                                   bindings->disconnect(h1); // Disconnect the listener after it's called
                               },
                               toBindingAddress(&source)),
                                           Value{ &source }, BindType::Deferred, false);

    mainScheduler->process();
    CHECK(listenerCalled == 0); // Ensure the listener has not been called yet

    bindings->notify(&source); // Trigger a notification on the source
    mainScheduler->process();
    CHECK(listenerCalled == 1); // Check that the listener was called once

    bindings->notify(&source); // Trigger another notification
    mainScheduler->process();
    CHECK(listenerCalled == 1); // Ensure the listener is not called again, since it was disconnected
}

// Test handling multiple listeners with reentrancy
TEST_CASE("Binding: reentrancy2") {
    int source = 0;
    BindingRegistration sourceLt{ &source, mainScheduler };
    BindingHandle h1, h2;
    int listener1Called = 0, listener2Called = 0;
    h1 = bindings->connect(Value<int>::listener(
                               [&]() {
                                   ++listener1Called;
                                   bindings->disconnect(h2);
                               },
                               toBindingAddress(&source)),
                           Value{ &source }, BindType::Deferred, false);

    h2 = bindings->connect(Value<int>::listener(
                               [&]() {
                                   ++listener2Called;
                               },
                               toBindingAddress(&source)),
                           Value{ &source }, BindType::Deferred, false);

    CHECK(listener1Called == 0);
    CHECK(listener2Called == 0);
    bindings->notify(&source); // Trigger a notification on the source
    mainScheduler->process();
    CHECK(listener1Called == 1);
    CHECK(listener2Called == 1);
    bindings->notify(&source); // Trigger a notification on the source
    mainScheduler->process();
    CHECK(listener1Called == 2);
    CHECK(listener2Called == 1);
}

// Test nested listeners in reentrant bindings
TEST_CASE("Binding: reentrancy3") {
    int source = 0;
    BindingRegistration sourceLt{ &source, mainScheduler };
    std::vector<BindingHandle> handles;
    int listener1Called = 0, listener2Called = 0, listener3Called = 0;

    handles.push_back(bindings->connect(
        Value<int>::listener(
            [&]() {
                ++listener1Called; // Increment the first listener call count
                handles.push_back(bindings->connect(
                    Value<int>::listener(
                        [&]() {
                            ++listener2Called; // Increment the second listener call count
                            handles.push_back(bindings->connect(
                                Value<int>::listener(
                                    [&]() {
                                        ++listener3Called; // Increment the third listener call count
                                    },
                                    toBindingAddress(&source)),
                                Value{ &source }, BindType::Deferred, false));
                        },
                        toBindingAddress(&source)),
                    Value{ &source }, BindType::Deferred, false));
            },
            toBindingAddress(&source)),
        Value{ &source }, BindType::Deferred, false));

    CHECK(bindings->numHandlers() == 1); // Verify that there is one handler after the initial connection

    bindings->notify(&source); // Trigger a notification on the source
    mainScheduler->process();
    CHECK(bindings->numHandlers() == 2); // Check that a new handler was created
    CHECK(listener1Called == 1);         // Ensure the first listener was called once
    CHECK(listener2Called == 0);         // Verify the second listener has not been called yet
    CHECK(listener3Called == 0);         // Verify the third listener has not been called yet

    bindings->notify(&source); // Trigger a second notification
    mainScheduler->process();
    CHECK(bindings->numHandlers() == 4); // Check that two more handlers were created
    CHECK(listener1Called == 2);         // Ensure the first listener was called a second time
    CHECK(listener2Called == 1);         // Check that the second listener was called once
    CHECK(listener3Called == 0);         // Verify the third listener has not been called yet

    bindings->notify(&source); // Trigger a third notification
    mainScheduler->process();
    CHECK(bindings->numHandlers() == 7); // Check that three more handlers were created
    CHECK(listener1Called == 3);         // Ensure the first listener was called three times
    CHECK(listener2Called == 3);         // Verify the second listener was called three times
    CHECK(listener3Called == 1);         // Check that the third listener was called once
}

// Test triggering a simple trigger and counting notifications
TEST_CASE("trigger") {
    Trigger<> t1;
    BindingRegistration l(&t1, nullptr);

    int counter = 0;

    bindings->listen(Value{ &t1 }, [&counter]() {
        ++counter;
    });
    bindings->notify(&t1);

    CHECK(counter == 1);

    bindings->notify(&t1);

    CHECK(counter == 2);
}

// Test interconnection between two triggers and count notifications
TEST_CASE("trigger2") {
    Trigger<> t[2];
    BindingRegistration l(&t, nullptr);

    int counter[2]{ 0, 0 };

    bindings->connectBidir(Value{ &t[0] }, Value{ &t[1] }, BindType::Immediate, false);

    bindings->listen(Value{ &t[0] }, [&counter]() {
        ++counter[0];
    });

    bindings->listen(Value{ &t[1] }, [&counter]() {
        ++counter[1];
    });

    bindings->notify(&t[0]);

    CHECK(counter[0] == 1);
    CHECK(counter[1] == 1);
}

// Test immediate bindings with equality conditions
TEST_CASE("Binding equal Immediate") {

    struct {
        int index = 0;
        bool v0   = false;
        bool v1   = false;
        bool v2   = false;
    } data;

    BindingRegistration lt{ &data, nullptr };

    bindings->connectBidir(Value{ &data.v0 }, Value{ &data.index }.equal(0), BindType::Immediate, true, "v0",
                           "index==0");
    bindings->connectBidir(Value{ &data.v1 }, Value{ &data.index }.equal(1), BindType::Immediate, true, "v1",
                           "index==1");
    bindings->connectBidir(Value{ &data.v2 }, Value{ &data.index }.equal(2), BindType::Immediate, true, "v2",
                           "index==2");
    CHECK(data.v0 == true);
    CHECK(data.v1 == false);
    CHECK(data.v2 == false);

    bindings->assign(data.index, 2);
    CHECK(data.v0 == false);
    CHECK(data.v1 == false);
    CHECK(data.v2 == true);

    bindings->assign(data.v1, true);
    CHECK(data.v0 == false);
    CHECK(data.v1 == true);
    CHECK(data.v2 == false);
}

// Test deferred bindings with equality conditions
TEST_CASE("Binding equal Deferred") {

    struct {
        int index = 0;
        bool v0   = false;
        bool v1   = false;
        bool v2   = false;
    } data;

    BindingRegistration lt{ &data, mainScheduler };

    bindings->connectBidir(Value{ &data.v0 }, Value{ &data.index }.equal(0), BindType::Deferred, true, "v0",
                           "index==0");
    bindings->connectBidir(Value{ &data.v1 }, Value{ &data.index }.equal(1), BindType::Deferred, true, "v1",
                           "index==1");
    bindings->connectBidir(Value{ &data.v2 }, Value{ &data.index }.equal(2), BindType::Deferred, true, "v2",
                           "index==2");
    mainScheduler->process();
    CHECK(data.v0 == true);
    CHECK(data.v1 == false);
    CHECK(data.v2 == false);

    bindings->assign(data.index, 2);
    CHECK(data.v0 == true);
    CHECK(data.v1 == false);
    CHECK(data.v2 == false);
    mainScheduler->process();
    CHECK(data.v0 == false);
    CHECK(data.v1 == false);
    CHECK(data.v2 == true);

    bindings->assign(data.v1, true);
    mainScheduler->process();
    CHECK(data.v0 == false);
    CHECK(data.v1 == true);
    CHECK(data.v2 == false);
}

// Test self-notification behavior in bindings
TEST_CASE("Binding self notification") {

    struct {
        int target = 0;
        int source = 123;
    } data;

    int counter = 0;

    BindingRegistration lt{ &data, nullptr };

    Value<int> targetValue = Value<int>{
        &data.target,
        [&counter]() {
            ++counter;
        },
    };

    bindings->connect(targetValue, Value{ &data.source }, BindType::Immediate, true);
    CHECK(data.target == 123);
    CHECK(counter == 1);

    bindings->assign(data.target, 256);
    CHECK(data.target == 256);
    CHECK(counter == 1);

    targetValue.set(100);
    CHECK(data.target == 100);
    CHECK(counter == 2);
}

// Test binding functionality with triggers that pass arguments
TEST_CASE("Binding with arguments") {
    Trigger<std::string> s1;
    BindingRegistration s1_r{ &s1, nullptr };
    std::vector<std::string> events;

    bindings->listen(Value{ &s1 }, [&](std::string val) {
        events.push_back(std::move(val));
    });
    CHECK(events == std::vector<std::string>{});

    s1.trigger("abc");
    CHECK(events == std::vector<std::string>{ "abc" });
    s1.trigger("def");
    CHECK(events == std::vector<std::string>{ "abc", "def" });
    s1.trigger("def");
    CHECK(events == std::vector<std::string>{ "abc", "def", "def" });
}

// Test binding with arguments and ensuring handlers are cleaned up
TEST_CASE("Binding with arguments 3") {
    Trigger<std::string> s1;
    BindingRegistration s1_r{ &s1, nullptr };
    std::vector<std::string> events;
    {
        BindingRegistration cb_r{ &events, nullptr };

        bindings->connect(Value<std::string>::listener( // Create a listener that captures events
                              [&](std::string val) {
                                  events.push_back(std::move(val));
                              },
                              toBindingAddress(cb_r.m_address)),
                          Value{ &s1 }, BindType::Immediate, false);
        CHECK(events == std::vector<std::string>{});

        s1.trigger("abc");
        CHECK(events == std::vector<std::string>{ "abc" });
        s1.trigger("def");
        CHECK(events == std::vector<std::string>{ "abc", "def" });
        s1.trigger("def");
        CHECK(events == std::vector<std::string>{ "abc", "def", "def" });

        CHECK(bindings->numHandlers() == 1);
    }
    CHECK(bindings->numHandlers() == 0);
    s1.trigger("ghi");
    CHECK(events == std::vector<std::string>{ "abc", "def", "def" });
}

// Test intersections of two values and their notification behavior
TEST_CASE("intersections") {

    union {
        uint16_t both = 0; // Union to hold a value that can be accessed as two separate bytes

        struct {
            uint8_t first;  // First byte of the union
            uint8_t second; // Second byte of the union
        };
    } x;

    int firstCounter = 0, secondCounter = 0, bothCounter = 0; // Counters to track listener calls

    BindingRegistration both_r{ &x.both, nullptr }; // Register a binding for the 'both' value

    // Listen for changes to 'first' and increment its counter
    bindings->listen(Value{ &x.first }, [&]() {
        ++firstCounter;
    });

    // Listen for changes to 'second' and increment its counter
    bindings->listen(Value{ &x.second }, [&]() {
        ++secondCounter;
    });

    // Listen for changes to 'both' and increment its counter
    bindings->listen(Value{ &x.both }, [&]() {
        ++bothCounter;
    });

    bindings->notify(&x.first); // Notify listeners of changes to 'first'
    CHECK(firstCounter == 1);   // Verify 'first' listener was called once
    CHECK(secondCounter == 0);  // Ensure 'second' listener was not called
    CHECK(bothCounter == 1);    // Check that 'both' listener was called once

    // Reset counters for the next notification
    firstCounter = 0, secondCounter = 0, bothCounter = 0;
    bindings->notify(&x.second); // Notify listeners of changes to 'second'
    CHECK(firstCounter == 0);    // Ensure 'first' listener was not called
    CHECK(secondCounter == 1);   // Verify 'second' listener was called once
    CHECK(bothCounter == 1);     // Check that 'both' listener was called once

    // Reset counters for the next notification
    firstCounter = 0, secondCounter = 0, bothCounter = 0;
    bindings->notify(&x.both); // Notify listeners of changes to 'both'
    CHECK(firstCounter == 1);  // Verify 'first' listener was called once
    CHECK(secondCounter == 1); // Verify 'second' listener was called once
    CHECK(bothCounter == 1);   // Check that 'both' listener was called once
}

TEST_CASE("Trigger after free") {
    int s       = 0;
    int d       = 0;
    int counter = 0;
    {
        bindings->registerRegion(toBindingAddress(&s), mainScheduler);
        bindings->registerRegion(toBindingAddress(&d), mainScheduler);

        bindings->connect(Value<int>(&d,
                                     [&]() {
                                         ++counter;
                                     }),
                          Value{ &s }, BindType::Deferred, false);
        bindings->assign(s, 123);
        mainScheduler->process();
        CHECK(counter == 1);
        bindings->assign(s, 456);
        mainScheduler->process();
        CHECK(counter == 2);

        bindings->assign(s, 789);
        bindings->unregisterRegion(toBindingAddress(&d));
        mainScheduler->process();
        CHECK(counter == 2);
        bindings->unregisterRegion(toBindingAddress(&s));
    }
}

} // namespace Brisk
