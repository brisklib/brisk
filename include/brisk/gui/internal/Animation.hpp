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
 */
#pragma once

#include <brisk/core/BasicTypes.hpp>
#include <brisk/graphics/Color.hpp>
#include <brisk/graphics/Geometry.hpp>
#include <brisk/window/Window.hpp>

namespace Brisk {

using EasingFunction = float (*)(float);

float easeInSine(float t);
float easeOutSine(float t);
float easeInOutSine(float t);
float easeInQuad(float t);
float easeOutQuad(float t);
float easeInOutQuad(float t);
float easeInCubic(float t);
float easeOutCubic(float t);
float easeInOutCubic(float t);
float easeInQuart(float t);
float easeOutQuart(float t);
float easeInOutQuart(float t);
float easeInQuint(float t);
float easeOutQuint(float t);
float easeInOutQuint(float t);
float easeInExpo(float t);
float easeOutExpo(float t);
float easeInOutExpo(float t);
float easeInCirc(float t);
float easeOutCirc(float t);
float easeInOutCirc(float t);
float easeInBack(float t);
float easeOutBack(float t);
float easeInOutBack(float t);
float easeInElastic(float t);
float easeOutElastic(float t);
float easeInOutElastic(float t);
float easeInBounce(float t);
float easeOutBounce(float t);
float easeInOutBounce(float t);
float easeLinear(float t);

namespace Internal {

template <typename T>
struct Transition {
    Transition(T value) : current(value), stopValue(value) {}

    constexpr static float disabled = -1;
    float startTime                 = disabled;
    T current;
    T startValue;
    T stopValue;

    bool set(T value, float transitionDuration) {
        if (transitionDuration == 0) {
            if (value == current)
                return false;
            current   = value;
            stopValue = value;
            startTime = disabled;
            return true;
        } else {
            startTime  = frameStartTime;
            startValue = current;
            stopValue  = value;
            return true;
        }
    }

    bool tick(float transitionDuration, EasingFunction easing) {
        if (isActive()) {
            float elapsed = frameStartTime - startTime;
            if (elapsed >= transitionDuration) {
                startTime = disabled;
                current   = stopValue;
            } else {
                current = mix(easing(elapsed / transitionDuration), startValue, stopValue);
            }
            return true;
        }
        return false;
    }

    bool isActive() const noexcept {
        return startTime >= 0;
    }
};

template <typename T>
struct Transition2 {
    Transition2(T current) : stopValue(current) {}

    constexpr static float disabled = -1;
    float startTime                 = disabled;
    T startValue;
    T stopValue;

    bool set(T& current, T value, float transitionDuration) {
        if (transitionDuration == 0) {
            if (value == current)
                return false;
            current   = value;
            stopValue = value;
            startTime = disabled;
            return true;
        } else {
            startTime  = frameStartTime;
            startValue = current;
            stopValue  = value;
            return true;
        }
    }

    void tick(T& current, float transitionDuration, EasingFunction easing) {
        if (isActive()) {
            float elapsed = frameStartTime - startTime;
            if (elapsed >= transitionDuration) {
                startTime = disabled;
                current   = stopValue;
            } else {
                current = mix(easing(elapsed / transitionDuration), startValue, stopValue);
            }
        }
    }

    bool isActive() const noexcept {
        return startTime >= 0;
    }
};
} // namespace Internal
} // namespace Brisk
