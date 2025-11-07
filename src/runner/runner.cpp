#include "gdiv/runner/runner.h"
#include "gdiv/runner/tiler.h"
#include <cmath>
#include <thread>
#include <future>
#include <numeric>
#include <unordered_map>

namespace gdiv::runner {

Result process_many(
    const std::vector<std::string>& rasters,
    const RunOptions& opt,
    const std::function<void(const std::vector<double>&, int, int, int,
                             std::optional<double>)>& on_tile,
    const std::function<double()>& on_finish)
{
    Result R;
    R.values.reserve(rasters.size());

    for (const auto& path : rasters) {
        auto ds = open_readonly(path);
        DatasetInfo info = describe(ds.get());
        const int bands = (opt.band_count == 0)
                        ? (opt.use_all_bands ? info.bands : 1)
                        : opt.band_count;
        const int first  = opt.first_band;

        auto tiles = make_tiles(info.width, info.height, opt.tile);

        std::vector<double> buf;
        for (const auto& win : tiles) {
            read_window(ds.get(), win, buf, first, bands);
            on_tile(buf, win.w, win.h, bands,
                    opt.nodata_override.has_value() ? opt.nodata_override
                                                    : info.nodata);
        }

        R.values.push_back(on_finish());
    }
    return R;
}

// ---------------------------
// EXAMPLE: WHOLE SHDI
// ---------------------------
Result process_many_shdi(const std::vector<std::string>& rasters,
                         const RunOptions& opt)
{
    std::unordered_map<long long, uint64_t> hist;
    uint64_t total = 0;

    auto clear_agg = [&](){ hist.clear(); total = 0; };

    auto on_tile = [&](const std::vector<double>& data, int w, int h, int bands,
                       std::optional<double> nodata) {

        (void)bands;
        const size_t n = static_cast<size_t>(w) * h;
        for (size_t i = 0; i < n; ++i) {
            const double v = data[i];
            if (nodata && std::isfinite(*nodata) && v == *nodata) continue;
            if (!std::isfinite(v)) continue;

            // make double int
            // if int, then static_cast<long long>(v)
            long long key = static_cast<long long>(std::llround(v));
            ++hist[key];
            ++total;
        }
    };

    auto on_finish = [&](){
        if (total == 0) return 0.0;
        long double H = 0.0L;
        for (const auto& kv : hist) {
            const long double p = static_cast<long double>(kv.second) / total;
            if (p > 0) H -= p * std::log(p);
        }
        const double ans = static_cast<double>(H);
        clear_agg();
        return ans;
    };

    return process_many(rasters, opt, on_tile, on_finish);
}

} // namespace gdiv::runner
