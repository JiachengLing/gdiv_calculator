#include "gdiv_utils.h"

int msr_compute(const char* path, const RasterOptions* opt,
                double* mean, double* var, double* vmin, double* vmax, uint64_t* valid) {
    long double sum = 0, sumsq = 0;
    double mn = std::numeric_limits<double>::infinity();
    double mx = -mn;
    uint64_t n = 0;

    // Loop through all raster pixels (helper from gdiv_utils)
    int rc = for_each_pixel_double(path, opt, [&](double v){
        sum += v;
        sumsq += (long double)v * v;
        ++n;
        if (v < mn) mn = v;
        if (v > mx) mx = v;
    }, /*require_non_empty=*/true);
    if (rc) return rc;

    *mean = (double)(sum / n);

    // Variance = E[x^2] - (E[x])^2
    long double variance = (sumsq / n) - ((long double)(*mean) * (*mean));
    if (variance < 0) variance = 0;

    *var = (double)variance;
    *vmin = mn;
    *vmax = mx;
    *valid = n;

    return 0;
}
