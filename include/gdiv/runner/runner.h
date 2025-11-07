#pragma once
#include <string>
#include <vector>
#include <optional>
#include <functional>
#include "gdal_io.h"

namespace gdiv::runner {

    struct RunOptions {
        int tile = 512;
        int threads = 0; // 0 = automatic
        std::optional<double> nodata_override;
        bool use_all_bands = false;   // example, if only 1 band, false (use the first band), if all bands, true,
        int first_band = 1;
        int band_count = 0;          // 0=all
    };

    struct Result {
        std::vector<double> values;
    };


    Result process_many(
        const std::vector<std::string>& rasters,
        const RunOptions& opt,
        const std::function<void(const std::vector<double>&, int, int, int,
                                 std::optional<double>)>& on_tile,
        const std::function<double()>& on_finish
    );

    Result process_many_shdi(const std::vector<std::string>& rasters,
                             const RunOptions& opt);

} // namespace gdiv::runner
