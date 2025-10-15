#include "gdiv_lsi.h"
#include "gdiv_utils.h"

#include <gdal_priv.h>
#include <vector>
#include <queue>
#include <limits>
#include <cmath>
#include <algorithm>

static inline size_t IDX(int x, int y, int W) { return (size_t)y * (size_t)W + (size_t)x; }

// Compute mean(perimeter/sqrt(area)) across connected components (4/8 connectivity).
int lsi_compute(const char* path,
                const RasterOptions* opt,
                double* out_lsi,
                uint64_t* out_valid)
{
    if (!path || !out_lsi || !out_valid) return 100;

    gdiv_init_gdal_once();
    GDALDataset* ds = static_cast<GDALDataset*>(GDALOpen(path, GA_ReadOnly));
    if (!ds) return 1;
    GDALRasterBand* band = ds->GetRasterBand(1);
    if (!band) { GDALClose(ds); return 1; }

    const int W = band->GetXSize(), H = band->GetYSize();

    // Resolve NoData from file or options
    bool hasND = false; double nd = std::numeric_limits<double>::quiet_NaN();
    resolve_nodata(band, opt, hasND, nd);

    // Read entire raster as double (avoid block/halo complexity for connectivity)
    std::vector<double> buf((size_t)W * H);
    if (band->RasterIO(GF_Read, 0, 0, W, H, buf.data(), W, H, GDT_Float64, 0, 0) != CE_None) {
        GDALClose(ds);
        return 2;
    }

    // Cast valid pixels to int; mark invalid with a sentinel
    const int INVALID = std::numeric_limits<int>::min();
    std::vector<int> vals(buf.size(), INVALID);
    uint64_t valid_px = 0;
    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            double v = buf[IDX(x, y, W)];
            if (is_invalid(v, hasND, nd)) continue;
            vals[IDX(x, y, W)] = (int)std::llround(v);
            ++valid_px;
        }
    }
    if (valid_px == 0) {
        *out_lsi = std::numeric_limits<double>::quiet_NaN();
        *out_valid = 0;
        GDALClose(ds);
        return 3;
    }

    // Connectivity: derive from options (default 8)
    const int conn = (opt ? opt->connectivity : 8);
    const bool use8 = (conn >= 8);

    // Neighbors for component growth
    const int nbh4[4][2] = { {1,0},{-1,0},{0,1},{0,-1} };
    const int nbh8[8][2] = { {1,0},{-1,0},{0,1},{0,-1},{1,1},{1,-1},{-1,1},{-1,-1} };
    const int (*NB)[2] = use8 ? nbh8 : nbh4;
    const int NB_N = use8 ? 8 : 4;

    // Perimeter counting uses 4-neighborhood for stability
    const int per4[4][2] = { {1,0},{-1,0},{0,1},{0,-1} };

    std::vector<char> seen(vals.size(), 0);
    std::queue<std::pair<int,int>> q;

    long double sum_ratio = 0.0L;
    uint64_t patch_count = 0;

    for (int y0 = 0; y0 < H; ++y0) {
        for (int x0 = 0; x0 < W; ++x0) {
            const size_t i0 = IDX(x0, y0, W);
            const int v0 = vals[i0];
            if (v0 == INVALID || seen[i0]) continue;

            uint64_t area = 0;
            uint64_t perim = 0;

            seen[i0] = 1;
            q.push({x0, y0});

            while (!q.empty()) {
                auto [x, y] = q.front(); q.pop();
                ++area;

                // Perimeter via 4-neighborhood
                for (int k = 0; k < 4; ++k) {
                    const int nx = x + per4[k][0];
                    const int ny = y + per4[k][1];
                    if (nx < 0 || ny < 0 || nx >= W || ny >= H) { ++perim; continue; }
                    const int nv = vals[IDX(nx, ny, W)];
                    if (nv == INVALID || nv != v0) ++perim;
                }

                // Grow component with chosen connectivity
                for (int k = 0; k < NB_N; ++k) {
                    const int nx = x + NB[k][0];
                    const int ny = y + NB[k][1];
                    if (nx < 0 || ny < 0 || nx >= W || ny >= H) continue;
                    const size_t ni = IDX(nx, ny, W);
                    if (!seen[ni] && vals[ni] == v0) {
                        seen[ni] = 1;
                        q.push({nx, ny});
                    }
                }
            }

            if (area > 0) {
                const long double ratio = (long double)perim / std::sqrt((long double)area);
                sum_ratio += ratio;
                ++patch_count;
            }
        }
    }

    *out_lsi = (patch_count > 0) ? (double)(sum_ratio / (long double)patch_count)
                                 : std::numeric_limits<double>::quiet_NaN();
    *out_valid = valid_px;

    GDALClose(ds);
    return 0;
}