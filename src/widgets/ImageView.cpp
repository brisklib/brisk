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
#include <brisk/widgets/ImageView.hpp>

namespace Brisk {

void ImageView::paint(Canvas& canvas) const {
    paintBackground(canvas, m_rect);
    Size size;
    if (m_image && m_rect.area() > 0) {
        size = m_image->size();
        canvas.drawImage(m_rect, m_image);
    }
}

void SvgImageView::paint(Canvas& canvas) const {
    paintBackground(canvas, m_rect);
    Size size = m_rect.size();
    if (!m_image || m_image->size() != size) {
        m_image = m_svg.render(size);
    }
    if (m_image) {
        size = m_image->size();
        canvas.drawImage(m_rect, m_image);
    }
}

Rc<Widget> SvgImageView::cloneThis() const { BRISK_CLONE_IMPLEMENTATION }

Rc<Widget> ImageView::cloneThis() const { BRISK_CLONE_IMPLEMENTATION }

ImageView::ImageView(Construction construction, Rc<Image> image, ArgumentsView<ImageView> args)
    : Widget(construction, nullptr), m_image(std::move(image)) {
    args.apply(this);
}
} // namespace Brisk
