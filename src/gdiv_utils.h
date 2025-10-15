#pragma once
#include "gdiv_toolbox.h"
#include <gdal_priv.h>
#include <cstdint>
#include <functional>

// Initialize: register GDAL
void gdiv_init_gdal_once();

// Resolve NODATA
void resolve_nodata(GDALRasterBand* band, const RasterOptions* opt,
                    bool& hasND, double& ndval);

// NODATA  -  inf or equal to nodata
inline bool is_invalid(double v, bool hasND, double ndval) {
    if (!std::isfinite(v)) return true;
    if (!hasND) return false;
    if (std::isnan(ndval)) return std::isnan(v);
    return v == ndval;
}

// Read as a whole, elem_size: double = 8, int = 4
bool prefer_full_read(GDALRasterBand* band, const RasterOptions* opt, size_t elem_size);


void compute_steps(GDALRasterBand* band, const RasterOptions* opt,
                   int& stepX, int& stepY);

// Generalized tool for looping
// Return：0=OK, 1=unable to open, or no band  2 = block read failed
// 3 = no valid pixel -- only when require_non_empty=true

int for_each_pixel_double(const char* path, const RasterOptions* opt,
                          const std::function<void(double)>& pixel_fn,
                          bool require_non_empty = false);

// same as above but read as int
int for_each_pixel_int(const char* path, const RasterOptions* opt,
                       const std::function<void(int)>& pixel_fn,
                       bool require_non_empty = false);
