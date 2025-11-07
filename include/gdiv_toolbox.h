#pragma once
#include "E:/gdiv_calculator/src/export.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

    typedef struct RasterOptions {
        int      connectivity;       // 4 or 8 (used by LSI)
        double   nodata;             // NoData value
        int      has_nodata;         // 1 = use nodata, 0 = ignore
        int      threads;            // reserved (can be 0)
        int      use_tiles;          // 1 = prefer internal tiling
        int      win_size;           // fallback window size if not tiled (e.g. 512)
        uint64_t max_bytes_simple;   // full-read threshold in bytes (0 -> default 256MB)
    } RasterOptions;

    // exports...
    GDIV_API int gdiv_calculate_msr(const char* path, const RasterOptions* opt,
                                    double* mean, double* stdv, double* vmin, double* vmax, uint64_t* valid);
    GDIV_API int gdiv_calculate_shdi(const char* path,
                                     const double* classes, int n_classes,
                                     const RasterOptions* opt,
                                     double* out_shdi, double* probs, uint64_t* out_valid);
    GDIV_API int gdiv_calculate_lsi(const char* path,
                                    const RasterOptions* opt,
                                    double* out_lsi, uint64_t* out_valid);

#ifdef __cplusplus
} // extern "C"
#endif
