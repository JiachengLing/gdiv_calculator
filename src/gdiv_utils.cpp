#include "gdiv_utils.h"
#include <algorithm>
#include <limits>
#include <mutex>
#include <vector>

// Initialization
void gdiv_init_gdal_once() {
    static std::once_flag flag;
    std::call_once(flag, [](){ GDALAllRegister(); });
}

// Resolve nodata
void resolve_nodata(GDALRasterBand* band, const RasterOptions* opt,
                    bool& hasND, double& ndval) {
    int gdalHas = 0;
    const double gdalND = band->GetNoDataValue(&gdalHas);
    if (gdalHas) { hasND = true; ndval = gdalND; return; }
    if (opt && opt->has_nodata) { hasND = true; ndval = opt->nodata; return; }
    hasND = false;
    ndval = std::numeric_limits<double>::quiet_NaN();
}

// Read strategy
bool prefer_full_read(GDALRasterBand* band, const RasterOptions* opt, size_t elem_size) {
    const int w = band->GetXSize(), h = band->GetYSize();
    const unsigned long long need =
        (unsigned long long)w * (unsigned long long)h * (unsigned long long)elem_size;

    const unsigned long long limit =
        (opt && opt->max_bytes_simple) ? opt->max_bytes_simple
                                       : (unsigned long long)256 * 1024 * 1024; // 256MB
    return need <= limit;
}

void compute_steps(GDALRasterBand* band, const RasterOptions* opt,
                   int& stepX, int& stepY) {
    int tileW=0, tileH=0;
    band->GetBlockSize(&tileW, &tileH); // if tiled GEOTIFF, size of tile is given

    const int W = band->GetXSize(), H = band->GetYSize();
    const bool use_tiles = (opt && opt->use_tiles && tileW>0 && tileH>0 && (tileW<W || tileH<H));

    stepX = use_tiles ? tileW : (opt && opt->win_size>0 ? opt->win_size : 512);
    stepY = use_tiles ? tileH : (opt && opt->win_size>0 ? opt->win_size : 512);
}

// Loop through pixels (double)
int for_each_pixel_double(const char* path, const RasterOptions* opt,
                          const std::function<void(double)>& pixel_fn,
                          bool require_non_empty) {
    if (!path) return 100;

    gdiv_init_gdal_once();
    GDALDataset* ds = static_cast<GDALDataset*>(GDALOpen(path, GA_ReadOnly));
    if (!ds) return 1;
    GDALRasterBand* band = ds->GetRasterBand(1);
    if (!band) { GDALClose(ds); return 1; }

    const int W = band->GetXSize(), H = band->GetYSize();

    bool hasND=false; double nd=std::numeric_limits<double>::quiet_NaN();
    resolve_nodata(band, opt, hasND, nd);

    uint64_t seen = 0;

    if (prefer_full_read(band, opt, sizeof(double))) {
        std::vector<double> data((size_t)W * H);
        if (band->RasterIO(GF_Read, 0,0, W,H, data.data(), W,H, GDT_Float64, 0,0) != CE_None) {
            GDALClose(ds); return 2;
        }
        for (double v : data) {
            if (is_invalid(v, hasND, nd)) continue;
            pixel_fn(v);
            ++seen;
        }
    } else {
        int stepX, stepY;
        compute_steps(band, opt, stepX, stepY);

        std::vector<double> block;
        for (int y=0; y<H; y+=stepY) {
            const int hh = std::min(stepY, H - y);
            for (int x=0; x<W; x+=stepX) {
                const int ww = std::min(stepX, W - x);
                block.resize((size_t)ww * hh);
                if (band->RasterIO(GF_Read, x,y, ww,hh, block.data(), ww,hh,
                                   GDT_Float64, 0,0) != CE_None) {
                    GDALClose(ds); return 2;
                }
                for (double v : block) {
                    if (is_invalid(v, hasND, nd)) continue;
                    pixel_fn(v);
                    ++seen;
                }
            }
        }
    }

    GDALClose(ds);
    if (require_non_empty && seen==0) return 3;
    return 0;
}

// Loop through pixels (int)
int for_each_pixel_int(const char* path, const RasterOptions* opt,
                       const std::function<void(int)>& pixel_fn,
                       bool require_non_empty) {
    if (!path) return 100;

    gdiv_init_gdal_once();
    GDALDataset* ds = static_cast<GDALDataset*>(GDALOpen(path, GA_ReadOnly));
    if (!ds) return 1;
    GDALRasterBand* band = ds->GetRasterBand(1);
    if (!band) { GDALClose(ds); return 1; }

    const int W = band->GetXSize(), H = band->GetYSize();

    bool hasND=false; double nd=std::numeric_limits<double>::quiet_NaN();
    resolve_nodata(band, opt, hasND, nd);

    uint64_t seen = 0;

    if (prefer_full_read(band, opt, sizeof(int))) {
        std::vector<int> data((size_t)W * H);
        if (band->RasterIO(GF_Read, 0,0, W,H, data.data(), W,H, GDT_Int32, 0,0) != CE_None) {
            GDALClose(ds); return 2;
        }
        for (int vi : data) {
            // 与 NoData 的比较：转 double 再走统一逻辑
            const double v = static_cast<double>(vi);
            if (is_invalid(v, hasND, nd)) continue;
            pixel_fn(vi);
            ++seen;
        }
    } else {
        int stepX, stepY;
        compute_steps(band, opt, stepX, stepY);

        std::vector<int> block;
        for (int y=0; y<H; y+=stepY) {
            const int hh = std::min(stepY, H - y);
            for (int x=0; x<W; x+=stepX) {
                const int ww = std::min(stepX, W - x);
                block.resize((size_t)ww * hh);
                if (band->RasterIO(GF_Read, x,y, ww,hh, block.data(), ww,hh,
                                   GDT_Int32, 0,0) != CE_None) {
                    GDALClose(ds); return 2;
                }
                for (int vi : block) {
                    const double v = static_cast<double>(vi);
                    if (is_invalid(v, hasND, nd)) continue;
                    pixel_fn(vi);
                    ++seen;
                }
            }
        }
    }

    GDALClose(ds);
    if (require_non_empty && seen==0) return 3;
    return 0;
}