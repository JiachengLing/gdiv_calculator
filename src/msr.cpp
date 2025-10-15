#include "gdiv_utils.h"

int msr_compute(const char* path, const RasterOptions* opt,
                double* mean, double* stdv, double* vmin, double* vmax, uint64_t* valid) {
    long double sum=0, sumsq=0;
    double mn=std::numeric_limits<double>::infinity(), mx=-mn;
    uint64_t n=0;

    int rc = for_each_pixel_double(path, opt, [&](double v){
        sum += v; sumsq += (long double)v*v; ++n;
        if (v<mn) mn=v; if (v>mx) mx=v;
    }, /*require_non_empty=*/true);
    if (rc) return rc;

    *mean = (double)(sum/n);
    long double var = (sumsq/n) - ((long double)(*mean)*(*mean));
    if (var < 0) var = 0;
    *stdv = std::sqrt((double)var);
    *vmin = mn; *vmax = mx; *valid = n;
    return 0;
}