#pragma once
#include "gdiv_toolbox.h"

int lsi_compute(const char* path,
                const RasterOptions* opt,
                double* out_lsi,
                uint64_t* out_valid);