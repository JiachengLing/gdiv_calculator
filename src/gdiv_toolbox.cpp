#include "gdiv_toolbox.h"
#include "gdiv_utils.h"
#include <gdal_priv.h>
#include <cpl_error.h>
#include <stdexcept>

// ==========
int msr_compute(const char* path, const RasterOptions* opt,
                double* mean, double* stdv, double* vmin, double* vmax, uint64_t* valid);

int shdi_compute(
  const char* path,
  const double* classes, int n_classes,
  const RasterOptions* opt,
  double* out_shdi,
  double* probs,
  uint64_t* out_valid
);

int lsi_compute(const char* path, const RasterOptions* opt,
                double* out_lsi, uint64_t* out_valid);

// ===========================================================

static void gdal_init_once() {
    static bool inited = false;
    if (!inited) { GDALAllRegister(); inited = true; }
}

static void set_gdal_throw() {
    CPLSetErrorHandler([](CPLErr, int, const char* msg){
        throw std::runtime_error(msg ? msg : "GDAL error");
    });
}

extern "C" {

    // --- MSR ---
    GDIV_API int gdiv_calculate_msr(const char* path, const RasterOptions* opt,
                                    double* mean, double* stdv, double* vmin, double* vmax, uint64_t* valid)
    {
        try {
            gdal_init_once();
            set_gdal_throw();
            return msr_compute(path, opt, mean, stdv, vmin, vmax, valid);
        } catch (...) {
            return 9;
        }
    }

    // --- SHDI ---
    GDIV_API int gdiv_calculate_shdi(const char* path,
                                     const double* classes, int n_classes,
                                     const RasterOptions* opt,
                                     double* out_shdi, double* probs, uint64_t* out_valid)
    {
        try {
            gdal_init_once();
            set_gdal_throw();
            return shdi_compute(path, classes, n_classes, opt, out_shdi, probs, out_valid);
        } catch (...) {
            return 9;
        }
    }

    // --- LSI ---
    GDIV_API int gdiv_calculate_lsi(const char* path,
                                    const RasterOptions* opt,
                                    double* out_lsi, uint64_t* out_valid)
    {
        try {
            gdal_init_once();
            set_gdal_throw();
            return lsi_compute(path, opt, out_lsi, out_valid);
        } catch (...) {
            return 9;
        }
    }

} // extern "C"
