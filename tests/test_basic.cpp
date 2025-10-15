#include "E:/gdiv_calculator/include/gdiv_toolbox.h"
#include <iostream>

int main() {
    RasterOptions opt;
    double mean, stdv, vmin, vmax;
    uint64_t valid;
    int ret = gdiv_calculate_msr("E:/gdiv_calculator/example_raster/T5FPCF.tiff", &opt, &mean, &stdv, &vmin, &vmax, &valid);
    if (ret == 0)
        std::cout << "Mean: " << mean << ", Std: " << stdv << ", N: " << valid << std::endl;
    else
        std::cout << "Error code: " << ret << std::endl;
    return 0;
}