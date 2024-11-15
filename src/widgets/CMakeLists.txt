#
# Brisk
#
# Cross-platform application framework
# --------------------------------------------------------------
#
# Copyright (C) 2024 Brisk Developers
#
# This file is part of the Brisk library.
#
# Brisk is dual-licensed under the GNU General Public License, version 2 (GPL-2.0+), and a commercial license. You may
# use, modify, and distribute this software under the terms of the GPL-2.0+ license if you comply with its conditions.
#
# You should have received a copy of the GNU General Public License along with this program. If not, see
# <http://www.gnu.org/licenses/>.
#
# If you do not wish to be bound by the GPL-2.0+ license, you must purchase a commercial license. For commercial
# licensing options, please visit: https://brisklib.com
#

add_library(
    brisk-widgets STATIC
    ${PROJECT_SOURCE_DIR}/include/brisk/widgets/Widgets.hpp
    ${PROJECT_SOURCE_DIR}/include/brisk/widgets/TextEditor.hpp
    ${PROJECT_SOURCE_DIR}/include/brisk/widgets/Viewport.hpp
    ${PROJECT_SOURCE_DIR}/include/brisk/widgets/Spinner.hpp
    ${PROJECT_SOURCE_DIR}/include/brisk/widgets/Progress.hpp
    ${PROJECT_SOURCE_DIR}/include/brisk/widgets/Guide.hpp
    ${PROJECT_SOURCE_DIR}/include/brisk/widgets/Notifications.hpp
    ${PROJECT_SOURCE_DIR}/include/brisk/widgets/ComboBox.hpp
    ${PROJECT_SOURCE_DIR}/include/brisk/widgets/Dot.hpp
    ${PROJECT_SOURCE_DIR}/include/brisk/widgets/SpinBox.hpp
    ${PROJECT_SOURCE_DIR}/include/brisk/widgets/Knob.hpp
    ${PROJECT_SOURCE_DIR}/include/brisk/widgets/ImageView.hpp
    ${PROJECT_SOURCE_DIR}/include/brisk/widgets/Slider.hpp
    ${PROJECT_SOURCE_DIR}/include/brisk/widgets/Color.hpp
    ${PROJECT_SOURCE_DIR}/include/brisk/widgets/Hyperlink.hpp
    ${PROJECT_SOURCE_DIR}/include/brisk/widgets/Table.hpp
    ${PROJECT_SOURCE_DIR}/include/brisk/widgets/Pages.hpp
    ${PROJECT_SOURCE_DIR}/include/brisk/widgets/Switch.hpp
    ${PROJECT_SOURCE_DIR}/include/brisk/widgets/ListBox.hpp
    ${PROJECT_SOURCE_DIR}/include/brisk/widgets/RadioButton.hpp
    ${PROJECT_SOURCE_DIR}/include/brisk/widgets/CheckBox.hpp
    ${PROJECT_SOURCE_DIR}/include/brisk/widgets/Text.hpp
    ${PROJECT_SOURCE_DIR}/include/brisk/widgets/Item.hpp
    ${PROJECT_SOURCE_DIR}/include/brisk/widgets/ItemList.hpp
    ${PROJECT_SOURCE_DIR}/include/brisk/widgets/AutoScrollable.hpp
    ${PROJECT_SOURCE_DIR}/include/brisk/widgets/ToggleButton.hpp
    ${PROJECT_SOURCE_DIR}/include/brisk/widgets/Button.hpp
    ${PROJECT_SOURCE_DIR}/include/brisk/widgets/ContextPopup.hpp
    ${PROJECT_SOURCE_DIR}/include/brisk/widgets/PopupBox.hpp
    ${PROJECT_SOURCE_DIR}/include/brisk/widgets/PopupButton.hpp
    ${PROJECT_SOURCE_DIR}/include/brisk/widgets/Spacer.hpp
    ${PROJECT_SOURCE_DIR}/include/brisk/widgets/Line.hpp
    ${PROJECT_SOURCE_DIR}/include/brisk/widgets/ValueWidget.hpp
    ${PROJECT_SOURCE_DIR}/include/brisk/widgets/ScrollBox.hpp
    ${PROJECT_SOURCE_DIR}/include/brisk/widgets/ScrollBar.hpp
    ${PROJECT_SOURCE_DIR}/include/brisk/widgets/PopupDialog.hpp
    ${PROJECT_SOURCE_DIR}/include/brisk/widgets/DialogComponent.hpp
    ${PROJECT_SOURCE_DIR}/include/brisk/widgets/Graphene.hpp
    ${PROJECT_SOURCE_DIR}/src/widgets/TextEditor.cpp
    ${PROJECT_SOURCE_DIR}/src/widgets/Viewport.cpp
    ${PROJECT_SOURCE_DIR}/src/widgets/Spinner.cpp
    ${PROJECT_SOURCE_DIR}/src/widgets/Progress.cpp
    ${PROJECT_SOURCE_DIR}/src/widgets/Guide.cpp
    ${PROJECT_SOURCE_DIR}/src/widgets/Notifications.cpp
    ${PROJECT_SOURCE_DIR}/src/widgets/ComboBox.cpp
    ${PROJECT_SOURCE_DIR}/src/widgets/Dot.cpp
    ${PROJECT_SOURCE_DIR}/src/widgets/SpinBox.cpp
    ${PROJECT_SOURCE_DIR}/src/widgets/Knob.cpp
    ${PROJECT_SOURCE_DIR}/src/widgets/ImageView.cpp
    ${PROJECT_SOURCE_DIR}/src/widgets/Slider.cpp
    ${PROJECT_SOURCE_DIR}/src/widgets/Color.cpp
    ${PROJECT_SOURCE_DIR}/src/widgets/Layouts.cpp
    ${PROJECT_SOURCE_DIR}/src/widgets/Hyperlink.cpp
    ${PROJECT_SOURCE_DIR}/src/widgets/Table.cpp
    ${PROJECT_SOURCE_DIR}/src/widgets/Pages.cpp
    ${PROJECT_SOURCE_DIR}/src/widgets/Switch.cpp
    ${PROJECT_SOURCE_DIR}/src/widgets/ListBox.cpp
    ${PROJECT_SOURCE_DIR}/src/widgets/RadioButton.cpp
    ${PROJECT_SOURCE_DIR}/src/widgets/CheckBox.cpp
    ${PROJECT_SOURCE_DIR}/src/widgets/Text.cpp
    ${PROJECT_SOURCE_DIR}/src/widgets/Item.cpp
    ${PROJECT_SOURCE_DIR}/src/widgets/ItemList.cpp
    ${PROJECT_SOURCE_DIR}/src/widgets/AutoScrollable.cpp
    ${PROJECT_SOURCE_DIR}/src/widgets/ToggleButton.cpp
    ${PROJECT_SOURCE_DIR}/src/widgets/Button.cpp
    ${PROJECT_SOURCE_DIR}/src/widgets/ContextPopup.cpp
    ${PROJECT_SOURCE_DIR}/src/widgets/PopupBox.cpp
    ${PROJECT_SOURCE_DIR}/src/widgets/PopupButton.cpp
    ${PROJECT_SOURCE_DIR}/src/widgets/Spacer.cpp
    ${PROJECT_SOURCE_DIR}/src/widgets/Line.cpp
    ${PROJECT_SOURCE_DIR}/src/widgets/ValueWidget.cpp
    ${PROJECT_SOURCE_DIR}/src/widgets/ScrollBox.cpp
    ${PROJECT_SOURCE_DIR}/src/widgets/ScrollBar.cpp
    ${PROJECT_SOURCE_DIR}/src/widgets/PopupDialog.cpp
    ${PROJECT_SOURCE_DIR}/src/widgets/DialogComponent.cpp
    ${PROJECT_SOURCE_DIR}/src/widgets/Graphene.cpp)

set_property(TARGET brisk-widgets PROPERTY EXPORT_NAME Widgets)
add_library(Brisk::Widgets ALIAS brisk-widgets)

target_link_libraries(brisk-widgets PUBLIC brisk-gui)

set_target_properties(brisk-widgets PROPERTIES CXX_CLANG_TIDY "${CLANG_TIDY_ARGS}")
