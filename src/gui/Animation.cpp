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
#include <brisk/gui/internal/Animation.hpp>

namespace Brisk {

#ifndef PI
#define PI 3.1415926545
#endif

float easeInSine(float t) {
    return std::sin(1.5707963 * t);
}

float easeOutSine(float t) {
    return 1 + std::sin(1.5707963 * (--t));
}

float easeInOutSine(float t) {
    return 0.5 * (1 + std::sin(3.1415926 * (t - 0.5)));
}

float easeInQuad(float t) {
    return t * t;
}

float easeOutQuad(float t) {
    return t * (2 - t);
}

float easeInOutQuad(float t) {
    return t < 0.5 ? 2 * t * t : t * (4 - 2 * t) - 1;
}

float easeInCubic(float t) {
    return t * t * t;
}

float easeOutCubic(float t) {
    return --t, 1 + t * t * t;
}

float easeInOutCubic(float p) {
    if (p < 0.5) {
        return 4 * p * p * p;
    } else {
        float f = ((2 * p) - 2);
        return 0.5 * f * f * f + 1;
    }
}

float easeInQuart(float t) {
    t *= t;
    return t * t;
}

float easeOutQuart(float t) {
    t = (--t, t * t);
    return 1 - t * t;
}

float easeInOutQuart(float t) {
    if (t < 0.5) {
        t *= t;
        return 8 * t * t;
    } else {
        t = (--t, t * t);
        return 1 - 8 * t * t;
    }
}

float easeInQuint(float t) {
    float t2 = t * t;
    return t * t2 * t2;
}

float easeOutQuint(float t) {
    float t2 = (--t, t * t);
    return 1 + t * t2 * t2;
}

float easeInOutQuint(float t) {
    float t2;
    if (t < 0.5) {
        t2 = t * t;
        return 16 * t * t2 * t2;
    } else {
        t2 = (--t, t * t);
        return 1 + 16 * t * t2 * t2;
    }
}

float easeInExpo(float t) {
    return (std::pow(2, 8 * t) - 1) / 255;
}

float easeOutExpo(float t) {
    return 1 - std::pow(2, -8 * t);
}

float easeInOutExpo(float t) {
    if (t < 0.5) {
        return (std::pow(2, 16 * t) - 1) / 510;
    } else {
        return 1 - 0.5 * std::pow(2, -16 * (t - 0.5));
    }
}

float easeInCirc(float t) {
    return 1 - std::sqrt(1 - t);
}

float easeOutCirc(float t) {
    return std::sqrt(t);
}

float easeInOutCirc(float t) {
    if (t < 0.5) {
        return (1 - std::sqrt(1 - 2 * t)) * 0.5;
    } else {
        return (1 + std::sqrt(2 * t - 1)) * 0.5;
    }
}

float easeInBack(float t) {
    return t * t * (2.70158 * t - 1.70158);
}

float easeOutBack(float t) {
    return --t, 1 + t * t * (2.70158 * t + 1.70158);
}

float easeInOutBack(float t) {
    if (t < 0.5) {
        return t * t * (7 * t - 2.5) * 2;
    } else {
        return --t, 1 + t * t * 2 * (7 * t + 2.5);
    }
}

float easeInElastic(float t) {
    float t2 = t * t;
    return t2 * t2 * std::sin(t * PI * 4.5);
}

float easeOutElastic(float t) {
    float t2 = (t - 1) * (t - 1);
    return 1 - t2 * t2 * std::cos(t * PI * 4.5);
}

float easeInOutElastic(float t) {
    float t2;
    if (t < 0.45) {
        t2 = t * t;
        return 8 * t2 * t2 * std::sin(t * PI * 9);
    } else if (t < 0.55) {
        return 0.5 + 0.75 * std::sin(t * PI * 4);
    } else {
        t2 = (t - 1) * (t - 1);
        return 1 - 8 * t2 * t2 * std::sin(t * PI * 9);
    }
}

float easeInBounce(float t) {
    return std::pow(2, 6 * (t - 1)) * std::abs(std::sin(t * PI * 3.5));
}

float easeOutBounce(float t) {
    return 1 - std::pow(2, -6 * t) * std::abs(std::cos(t * PI * 3.5));
}

float easeInOutBounce(float t) {
    if (t < 0.5) {
        return 8 * std::pow(2, 8 * (t - 1)) * std::abs(std::sin(t * PI * 7));
    } else {
        return 1 - 8 * std::pow(2, -8 * t) * std::abs(std::sin(t * PI * 7));
    }
}

float easeLinear(float t) {
    return t;
}

void PropertyAnimations::startAnimation(PropertyId propertyId, AnimationFunction animationFunc) {
    m_animations[propertyId] = { Seconds(frameStartTime), std::move(animationFunc) };
}

bool PropertyAnimations::isActive() const noexcept {
    return !m_animations.empty();
}

void PropertyAnimations::tick() {
    for (auto it = m_animations.begin(); it != m_animations.end();) {
        if (!it->second.animation(animationSpeed * (Seconds(frameStartTime) - it->second.startTime)))
            [[unlikely]] {
            it = m_animations.erase(it);
        } else {
            ++it;
        }
    }
}

TransitionParams PropertyAnimations::getTransitionParams(PropertyId id) const {
    auto it = m_transitions.find(id);
    return it == m_transitions.end() ? TransitionParams{} : it->second;
};

void PropertyAnimations::setTransitionParams(PropertyId id, const TransitionParams& params) {
    if (params.duration <= 0s) [[unlikely]] {
        if (auto it = m_animations.find(id); it != m_animations.end()) {
            // If the transition duration is zero, we immediately apply the value
            // and remove the animation.
            it->second.animation(86400s); // Force immediate transition
            m_animations.erase(it);
        }
        m_transitions.erase(id);
    } else {
        m_transitions[id] = params;
    }
}

double TransitionParams::at(Seconds time) const noexcept {
    if (time.count() <= delay.count()) [[unlikely]]
        return 0;
    if (duration.count() <= 0) [[unlikely]]
        return 1;
    return std::clamp((time.count() - delay.count()) / duration.count(), 0.0, 1.0);
}

double animationSpeed = 1.0;

} // namespace Brisk
