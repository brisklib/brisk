#include <cstdint>

#include "Utils.h"
#include "../Mask.hpp"

namespace Brisk {

namespace Internal {
DenseMask rasterizePath(const Path& path, FillRule fillRule, Rectangle clip);
}

} // namespace Brisk
