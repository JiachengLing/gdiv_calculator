#include "gdiv_toolbox.h"
#include <gdal_priv.h>
#include <cpl_conv.h>
#include <cmath>
#include <vector>
#include <iostream>

static inline bool prefer_full_read(GDALRasterBand* band, const RasterOptions* opt, size_t elem_size) {
    const int w = band->GetXSize(), h = band->GetYSize();
    uint64_t need = (uint64_t)w * (uint64_t)h * (uint64_t)elem_size;

    uint64_t limit = (opt && opt->max_bytes_simple) ? opt->max_bytes_simple : (uint64_t)256 * 1024 * 1024; // 256MB
    return need <= limit;
}

int gdiv_calculate_msr(const char* path, const RasterOptions* opt,
                       double* mean, double* stdv, double* vmin, double* vmax, uint64_t* valid)
{
    GDALAllRegister();
    GDALDataset* ds = (GDALDataset*) GDALOpen(path, GA_ReadOnly);
    if (!ds) return 1;

    GDALRasterBand* band = ds->GetRasterBand(1);
    int w = band->GetXSize(), h = band->GetYSize();

    std::vector<double> data((size_t)w*h);
    if (band->RasterIO(GF_Read, 0,0,w,h, data.data(), w,h, GDT_Float64, 0,0) != CE_None) {
        GDALClose(ds);
        return 2;
    }

    double sum=0, sq=0;
    double mn=INFINITY, mx=-INFINITY;
    uint64_t n=0;

    for (double v : data) {
        if (!std::isfinite(v)) continue;
        sum+=v; sq+=v*v; ++n;
        if (v<mn) mn=v;
        if (v>mx) mx=v;
    }

    if (n==0) { GDALClose(ds); return 3; }
    *mean = sum/n;
    *stdv = std::sqrt((sq/n) - (*mean)*(*mean));
    *vmin = mn; *vmax = mx; *valid = n;

    GDALClose(ds);
    return 0;
}