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

#include <brisk/gui/GUI.hpp>
#include <brisk/gui/Groups.hpp>
#include <brisk/core/Utilities.hpp>

namespace Brisk {
template <typename TItem>
struct Selection {
    PossiblyShared<std::vector<TItem>> order;
    std::set<TItem> selection;
    optional<TItem> focused; // last selected

    void selectAll() {
        selection.clear();
        selection.insert(order->begin(), order->end());
    }

    void reset() {
        selection.clear();
        focused.reset();
    }

    void set(TItem value) {
        selection.clear();
        add(std::move(value));
    }

    void add(TItem value) {
        selection.insert(value);
        focused = std::move(value);
    }

    void remove(const TItem& value) {
        selection.erase(value);
    }

    void toggle(TItem value) {
        if (auto it = selection.find(value); it != selection.end()) {
            selection.erase(it);
        } else {
            selection.insert(value);
        }
        focused = std::move(value);
    }

    void selectRange(TItem value) {
        if (!focused)
            return set(std::move(value));
        auto selectedIt = std::find(order->begin(), order->end(), value);
        auto focusedIt  = std::find(order->begin(), order->end(), *focused);
        BRISK_ASSERT(selectedIt != order->end());
        BRISK_ASSERT(focusedIt != order->end());
        if (selectedIt < focusedIt)
            std::swap(selectedIt, focusedIt);
        BRISK_ASSERT(selectedIt >= focusedIt);
        ++selectedIt;
        selection.clear();
        selection.insert(focusedIt, selectedIt);
    }

    bool isSelected(const TItem& value) const {
        return selection.find(value) != selection.end();
    }
};

class WIDGET Table : public Widget {
public:
    using Base                                   = Widget;
    constexpr static std::string_view widgetType = "table";

    using Widget::apply;

    template <WidgetArgument... Args>
    explicit Table(const Args&... args) : Table{ Construction{ widgetType }, std::tuple{ args... } } {
        endConstruction();
    }

    void onEvent(Event& event) override;

    void childrenAdded() override;

    std::array<WidthGroup, 32> columns;

protected:
    Ptr cloneThis() const override;

    explicit Table(Construction construction, ArgumentsView<Table> args);
};

class WIDGET TableRow : public Widget {
public:
    using Base                                   = Widget;
    constexpr static std::string_view widgetType = "tablerow";

    template <WidgetArgument... Args>
    explicit TableRow(const Args&... args) : TableRow(Construction{ widgetType }, std::tuple{ args... }) {
        endConstruction();
    }

protected:
    Ptr cloneThis() const override;

    explicit TableRow(Construction construction, ArgumentsView<TableRow> args);
};

class WIDGET TableHeader : public TableRow {
public:
    using Base                                   = TableRow;
    constexpr static std::string_view widgetType = "tableheader";

    template <WidgetArgument... Args>
    explicit TableHeader(const Args&... args)
        : TableHeader(Construction{ widgetType }, std::tuple{ args... }) {
        endConstruction();
    }

protected:
    Ptr cloneThis() const override;

    explicit TableHeader(Construction construction, ArgumentsView<TableHeader> args);
};

template <typename TItem = bool>
class WIDGET TableRowSelectable : public TableRow {
public:
    using Base                                   = TableRow;
    constexpr static std::string_view widgetType = "tablerow";

    template <WidgetArgument... Args>
    explicit TableRowSelectable(Selection<TItem>* selection, TItem item, const Args&... args)
        : TableRowSelectable(Construction{ widgetType }, selection, item, std::tuple{ args... }) {
        endConstruction();
    }

    void updateState() {
        toggleState(WidgetState::Selected, selection->isSelected(item));
    }

    Selection<TItem>* selection;
    TItem item;

protected:
    void resetSelection() override {
        selection->reset();
    }

    void onEvent(Event& event) override {
        TableRow::onEvent(event);
        auto pressed = event.as<EventMouseButtonPressed>();
        if (pressed && pressed->button == MouseButton::Left) {
            if (pressed->mods && KeyModifiers::Shift)
                selection->selectRange(item);
            else if (pressed->mods && KeyModifiers::Control)
                selection->toggle(item);
            else
                selection->set(item);
            updateState();
            event.stopPropagation();
        }
    }

    Ptr cloneThis() const override {
        BRISK_CLONE_IMPLEMENTATION
    }

    explicit TableRowSelectable(Construction construction, Selection<TItem>* selection, TItem item,
                                ArgumentsView<TableRowSelectable> args)
        : TableRow(construction, args), selection(selection), item(std::move(item)) {}
};

class WIDGET TableCell : public Widget {
public:
    using Base                                   = Widget;
    constexpr static std::string_view widgetType = "tablecell";

    template <WidgetArgument... Args>
    explicit TableCell(const Args&... args) : TableCell(Construction{ widgetType }, std::tuple{ args... }) {
        endConstruction();
    }

protected:
    friend class Table;
    bool m_widthGroupSet = false;

    Ptr cloneThis() const override;

    explicit TableCell(Construction construction, ArgumentsView<TableCell> args);
};
} // namespace Brisk
