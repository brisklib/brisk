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
#include <brisk/widgets/Table.hpp>

namespace Brisk {
void Table::onEvent(Event& event) {
    Widget::onEvent(event);
    auto pressed = event.as<EventMouseButtonPressed>();
    if (pressed && pressed->button == MouseButton::Left) {
        resetSelection();
    }
}

void Table::childrenAdded() {
    Widget::childrenAdded();
    for (const RC<Widget>& w1 : *this) {
        if (TableRow* row = dynamic_cast<TableRow*>(w1.get())) {
            int i = 0;
            for (const RC<Widget>& w2 : *row) {
                if (TableCell* cell = dynamic_cast<TableCell*>(w2.get())) {
                    if (!cell->m_widthGroupSet && i < columns.size()) {
                        cell->apply(&columns[i++]);
                        cell->m_widthGroupSet = true;
                    }
                }
            }
        }
    }
}

Table::Table(Construction construction, ArgumentsView<Table> args)
    : Widget{ construction, std::tuple{ Arg::layout = Layout::Vertical } } {
    args.apply(this);
}

RC<Widget> Table::cloneThis() const { BRISK_CLONE_IMPLEMENTATION }

TableRow::TableRow(Construction construction, ArgumentsView<TableRow> args)
    : Widget(construction, std::tuple{ Arg::layout = Layout::Horizontal }) {
    args.apply(this);
}

RC<Widget> TableRow::cloneThis() const { BRISK_CLONE_IMPLEMENTATION }

RC<Widget> TableHeader::cloneThis() const { BRISK_CLONE_IMPLEMENTATION }

TableHeader::TableHeader(Construction construction, ArgumentsView<TableHeader> args)
    : TableRow(construction, nullptr) {
    args.apply(this);
}

RC<Widget> TableCell::cloneThis() const { BRISK_CLONE_IMPLEMENTATION }

TableCell::TableCell(Construction construction, ArgumentsView<TableCell> args)
    : Widget(construction, std::tuple{ Arg::layout = Layout::Horizontal }) {
    args.apply(this);
}
} // namespace Brisk
