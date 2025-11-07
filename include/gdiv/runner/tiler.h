#pragma once
#include <vector>
#include "gdal_io.h"

namespace gdiv::runner {

    /** cut (width x height) to tile size window */
    std::vector<Window> make_tiles(int width, int height, int tile);

} // namespace gdiv::runner
