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
#include <brisk/core/Time.hpp>
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

using Seconds = std::chrono::duration<double>;

using namespace std::chrono_literals;

/**
 * @struct Animated
 * @brief Represents a value that can be animated over time.
 *
 * This structure holds both the target value and the current animated value,
 * allowing smooth transitions or animations between values.
 */
template <typename ValueType, typename AnimatedType = ValueType>
struct Animated {
    /// The target value to which the animation is progressing.
    ValueType value;
    /// The current value in the animation sequence.
    AnimatedType current;
    constexpr Animated() noexcept = default;

    constexpr Animated(ValueType value) noexcept
        requires std::same_as<ValueType, AnimatedType>
        : value(value), current(value) {}

    constexpr Animated(ValueType value, AnimatedType currentValue) noexcept
        : value(std::move(value)), current(std::move(currentValue)) {}

    constexpr bool operator==(const Animated& other) const noexcept = default;
};

// Function that animates a property of a class instance
// Returns true if the animation is still active, false if it has completed
using AnimationFunction = std::function<bool(Seconds /*elapsedTime*/)>;

struct TransitionParams {
    Seconds duration      = 0s;
    EasingFunction easing = easeLinear;
    Seconds delay         = 0s;

    double at(Seconds time) const noexcept;
};

/**
 * @brief Global variable controlling the speed of animations. Default is 1.0.
 */
extern double animationSpeed;

/**
 * @class PropertyAnimations
 * @brief Manages property animations and transitions for GUI widgets.
 *
 * The PropertyAnimations class provides an interface for animating properties over time
 * using customizable transition parameters and easing functions.
 *
 * Thread Safety:
 * - This class is not thread-safe. All methods should be called from the UI thread.
 */
class PropertyAnimations {
public:
    /// @returns true if the property is animated, false otherwise.
    template <typename ValueType>
    bool startTransition(ValueType& value, ValueType targetValue, PropertyId propertyId,
                         std::function<void()> changed = nullptr) {
        auto transitionIt = m_transitions.find(propertyId);
        if (transitionIt == m_transitions.end()) {
            return false; // No transition defined for this property
        }
        if (transitionIt->second.duration <= 0s) {
            return false; // Zero duration means no transition
        }
        auto interpFunc = transitionFunction(transitionIt->second, value, std::move(targetValue));
        AnimationFunction animationFunc = animateValue(&value, std::move(interpFunc), std::move(changed));
        startAnimation(propertyId, std::move(animationFunc));
        return true;
    }

    void startAnimation(PropertyId propertyId, AnimationFunction animationFunc);

    /**
     * @brief Checks if there are any active animations.
     *
     * This method returns true if there are animations currently active.
     * It must be called before calling tick during frame processing.
     *
     * @return true if there are active animations, false otherwise.
     */
    bool isActive() const noexcept;

    /**
     * @brief Advances all active animations and removes completed animations.
     *
     * @param frameTime The current time in seconds, used to calculate the elapsed time for each animation.
     */
    void tick();

    /**
     * @brief Retrieves the transition parameters associated with a given property ID.
     *
     * @param id The identifier of the property for which to retrieve transition parameters.
     * @return TransitionParams The transition parameters if the given property has transition,
     * default-initialized TransitionParams otherwise.
     */
    TransitionParams getTransitionParams(PropertyId id) const;

    /**
     * @brief Sets the transition parameters for a specific property.
     *
     * @param id The identifier of the property for which to set the transition parameters.
     * @param params The transition parameters to apply to the property.
     */
    void setTransitionParams(PropertyId id, const TransitionParams& params);

private:
    template <typename ValueType>
    static auto transitionFunction(const TransitionParams& params, ValueType initialValue,
                                   ValueType finalValue) noexcept {
        return [&params, initialValue = std::move(initialValue),
                finalValue = std::move(finalValue)](Seconds time) -> std::pair<ValueType, bool> {
            double t = params.at(time);
            return { mix(params.easing(t), initialValue, finalValue), t < 1.0 };
        };
    }

    template <typename ValueType, typename InterpolationFunction>
    static AnimationFunction animateValue(ValueType* valueToAnimate, InterpolationFunction&& func,
                                          std::function<void()> changed) {
        return [valueToAnimate, func = std::move(func), changed = std::move(changed)](Seconds time) -> bool {
            std::pair<ValueType, bool> value = func(time);
            // No need to notify bindings here, because bindings depend on the final values of the properties
            *valueToAnimate                  = value.first;
            if (changed)
                changed();
            return value.second;
        };
    }

    struct Entry {
        Seconds startTime;
        AnimationFunction animation;
    };

    std::map<PropertyId, TransitionParams> m_transitions;
    std::map<PropertyId, Entry> m_animations;
    static double m_speed; // Animation speed multiplier
};

} // namespace Brisk
