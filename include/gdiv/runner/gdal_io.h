#pragma once
#include <string>
#include <vector>
#include <optional>
#include <memory>
#include <gdal_priv.h>

namespace gdiv::runner {

    struct DatasetInfo {
        int width = 0, height = 0, bands = 0;
        std::optional<double> nodata;
    };

    struct Window {
        int x = 0, y = 0, w = 0, h = 0;
    };

    using GDALDatasetPtr = std::unique_ptr<GDALDataset, void(*)(GDALDataset*)>;

    GDALDatasetPtr open_readonly(const std::string& path);

    DatasetInfo describe(GDALDataset* ds);

    /** Read a window（all bands or selected band）, output by double, 。
     *  out.size() will be set:  w*h*bands。
     */
    void read_window(GDALDataset* ds, const Window& win, std::vector<double>& out,
                     int first_band = 1, int band_count = 0 /*0=all*/);

} // namespace gdiv::runner