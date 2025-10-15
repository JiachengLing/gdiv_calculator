#include "gdiv_utils.h"
#include <unordered_map>

int shdi_compute(const char* path, const double* classes, int n_classes,
                 const RasterOptions* opt, double* out_shdi, double* probs, uint64_t* out_valid) {
    std::unordered_map<long long,int> lut;
    for (int i=0;i<n_classes;++i){ lut[(long long)std::llround(classes[i])] = i; probs[i]=0.0; }
    std::vector<uint64_t> counts(n_classes, 0);
    uint64_t n=0;

    int rc = for_each_pixel_double(path, opt, [&](double v){
        auto it = lut.find((long long)std::llround(v));
        if (it!=lut.end()) { counts[it->second]++; ++n; }
    }, /*require_non_empty=*/true);
    if (rc) return rc;

    long double H=0.0L;
    for (int i=0;i<n_classes;++i){
        double p = (double)counts[i] / (double)n;
        probs[i]=p;
        if (p>0) H -= p * std::log(p);
    }
    *out_shdi=(double)H; *out_valid=n; return 0;
}