#include "gdiv/runner/gdal_io.h"
#include <stdexcept>
#include <mutex>

namespace gdiv::runner {

static std::once_flag gdal_once;

GDALDatasetPtr open_readonly(const std::string& path) {
    std::call_once(gdal_once, [] { GDALAllRegister(); });
    GDALDataset* raw = static_cast<GDALDataset*>(
        GDALOpen(path.c_str(), GA_ReadOnly));
    if (!raw) {
        throw std::runtime_error("GDALOpen failed: " + path);
    }
    return GDALDatasetPtr(raw, [](GDALDataset* ds){ GDALClose(ds); });
}

DatasetInfo describe(GDALDataset* ds) {
    if (!ds) throw std::runtime_error("describe(): null dataset");
    DatasetInfo info;
    info.width  = ds->GetRasterXSize();
    info.height = ds->GetRasterYSize();
    info.bands  = ds->GetRasterCount();

    // nodata
    if (info.bands > 0) {
        int success = 0;
        const double nd = ds->GetRasterBand(1)->GetNoDataValue(&success);
        if (success) info.nodata = nd;
    }
    return info;
}

void read_window(GDALDataset* ds, const Window& win, std::vector<double>& out,
                 int first_band, int band_count)
{
    if (!ds) throw std::runtime_error("read_window(): null dataset");
    const int bands_total = ds->GetRasterCount();
    if (band_count == 0) band_count = bands_total;
    if (first_band < 1 || first_band > bands_total)
        throw std::runtime_error("Invalid first_band");

    out.assign(static_cast<size_t>(win.w) * win.h * band_count, 0.0);

    // read through bands
    for (int b = 0; b < band_count; ++b) {
        GDALRasterBand* band = ds->GetRasterBand(first_band + b);
        const CPLErr err = band->RasterIO(
            GF_Read,
            win.x, win.y, win.w, win.h,
            out.data() + static_cast<size_t>(b) * win.w * win.h,
            win.w, win.h, GDT_Float64,
            0, 0, // default - continuous.
            nullptr
        );
        if (err != CE_None) {
            throw std::runtime_error("RasterIO failed at window("
                                     + std::to_string(win.x) + "," + std::to_string(win.y)
                                     + "," + std::to_string(win.w) + "x" + std::to_string(win.h) + ")");
        }
    }
}

} // namespace gdiv::runner
