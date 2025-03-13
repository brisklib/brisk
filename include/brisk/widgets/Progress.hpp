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
#pragma once

#include <brisk/core/Binding.hpp>
#include <brisk/widgets/ValueWidget.hpp>

namespace Brisk {

class Progress;

/**
 * @class ProgressBar
 * @brief A non-standalone widget that represents the moving bar within a `Progress` widget.
 *
 * The `ProgressBar` class is always created as a child widget to a `Progress` widget. It visually represents
 * the moving bar that indicates the current progress, while the `Progress` widget represents the entire
 * progress indicator.
 *
 * If a `ProgressBar` instance is not explicitly provided when constructing a `Progress` widget, a default
 * `ProgressBar` instance will be automatically created and associated with the `Progress` widget.
 *
 * @note This class is final and cannot be subclassed.
 */
class ProgressBar final : public Widget {
public:
    using Base                                   = Widget;
    constexpr static std::string_view widgetType = "progressbar";

    /**
     * @brief Constructs a ProgressBar widget.
     *
     * The `ProgressBar` is meant to be used only as a child to the `Progress` widget.
     *
     * @param args Widget arguments and property values.
     */
    template <WidgetArgument... Args>
    explicit ProgressBar(const Args&... args) : Widget(Construction{ widgetType }, std::tuple{ args... }) {
        endConstruction();
    }

protected:
    friend class Progress;
    /**
     * @brief Updates the value of the progress bar.
     */
    void updateValue();
    void onLayoutUpdated() override;
};

/**
 * @class Progress
 * @brief A widget that represents the entire progress indicator, including the progress bar and optional
 * decorations.
 *
 * The `Progress` class manages the progress value, handles user-defined changes.
 *
 * If no custom `ProgressBar` is provided during the construction of `Progress`, a default instance of
 * `ProgressBar` is automatically created and linked to it.
 */
class Progress : public ValueWidget {
public:
    using Base                                   = ValueWidget;
    constexpr static std::string_view widgetType = "progress";

    /**
     * @brief Constructs a Progress widget.
     *
     * @param args Widget arguments and property values.
     * @note If a custom instance of `ProgressBar` widget is not provided, a default one is created.
     */
    template <WidgetArgument... Args>
    explicit Progress(const Args&... args) : Progress(Construction{ widgetType }, std::tuple{ args... }) {
        endConstruction();
    }

protected:
    void onChanged() override;
    explicit Progress(Construction construction, ArgumentsView<Progress> args);
    void onLayoutUpdated() override;
};

void paintProgressIndicator(Canvas& canvas, RectangleF rect, int circles = 3);

} // namespace Brisk
