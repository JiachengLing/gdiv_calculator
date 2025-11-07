#include "gdiv/runner/tiler.h"

namespace gdiv::runner {

    std::vector<Window> make_tiles(int width, int height, int tile) {
        std::vector<Window> tiles;
        if (tile <= 0) { tiles.push_back({0,0,width,height}); return tiles; }

        for (int y = 0; y < height; y += tile) {
            const int h = std::min(tile, height - y);
            for (int x = 0; x < width; x += tile) {
                const int w = std::min(tile, width - x);
                tiles.push_back({x, y, w, h});
            }
        }
        return tiles;
    }

} // namespace gdiv::runner
