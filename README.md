# gdiv_calculator (v 1.1)

A modular C++ geodiversity (spatial metrics) calculation toolbox for raster-based environmental analysis.
It computes spatial indices such as mean, standard deviation (MSR), Shannon Diversity Index (SHDI),
and Landscape Shape Index (LSI) using the GDAL library,
with a ready-to-use Python interface example for CSV generation.

---
## Description

`gdiv_calculator` provides efficient raster metric computation for environmental and biodiversity studies.
It supports both simple statistical metrics and categorical landscape metrics.

Core features include:
- modular C++ design for maintainability
- Built on [GDAL](https://gdal.org) for raster access
- Ready for integration with Python bindings
- Memory-efficient block reading for large rasters
- Example Python integration (.py) for automated metric table generation
  
## Directory Structure
```
gdiv_calculator/
├── CMakeLists.txt
├── include/
│ ├── gdiv_toolbox.h # Public API declarations
│ └── gdiv/runner/ # Runner headers (GDAL IO, tiling, main loop)
│ ├── gdal_io.h
│ ├── tiler.h
│ └── runner.h
├── src/
│ ├── gdiv_toolbox.cpp # C API entry (msr/shdi/lsi dispatch)
│ ├── gdiv_utils.cpp/.h # GDAL init, NODATA handling, etc.
│ ├── msr.cpp # Mean-Std raster computation
│ ├── shdi.cpp # Shannon Diversity Index computation
│ ├── gdiv_lsi.cpp # Landscape Shape Index computation
│ └── runner/ # High-performance raster loop backend
│ ├── gdal_io.cpp
│ ├── tiler.cpp
│ └── runner.cpp
├── tests/
│ └── test_basic.cpp # Example test for MSR calculation
├── example_raster/
│ └── T5FPCF.tiff # Example raster dataset
├── dist/
│ ├── Debug/ # Debug build outputs
│ ├── Release/ # Release build outputs
│ └── results_metrics.csv # Example output from Python script
└── compile_gdiv.py # Python example for DLL integration

```


---

## Build Instructions

### 1. Requirements
- **Compiler:** Visual Studio 2022 / MSVC  
- **Dependency:** [GDAL](https://gdal.org) (e.g., via `conda install -c conda-forge gdal`)  
- **Build System:** CMake ≥ 3.15  

### 2. Build Steps (Windows + Conda)

```powershell
# (1) Activate the base environment
conda activate base

# (2) Create build directory
cmake -S . -B build -A x64 `
  -DGDAL_INCLUDE_DIR="$env:CONDA_PREFIX\Library\include" `
  -DGDAL_LIBRARY="$env:CONDA_PREFIX\Library\lib\gdal.lib"

# (3) Compile in Release mode
cmake --build build --config Release
```
Build artifacts will be placed in:

```
dist/Release/gdiv_toolbox.dll
```

## Run example (C++ test)

After successful build, run the test binary:

```Markdown
cd dist/Release
.\test_basic.exe "E:\gdiv_calculator\example_raster\T5FPCF.tiff"
```

## Run example (Python test)
You can now call the compiled C++ DLL directly from Python to compute metrics and export them to CSV.
Usage:

```
python compile_gdiv.py
```
# What is does:
- Scans all subfolders under `DATA/` (each factor = one folder)
- For each site (raster file), computes:
  - `{factor}_msr` (variance of env factor)
  - `{factor}_mean`
  - `{factor}_shdi`(if factor categorical and in SHDI whitelist)
  - `{factor}_lsi`(if factor in LSI whitelist)
- Write results to `dist/results_metrics.csv`

# Main features
- Uses `ctypes` to bind `gdiv_toolbox.dll`
- Supports SHDI class configuration and optional proportion columns
- Logs warnings when GDAL fails for specific sites
- Works on both Debug and Release builds
  
## How to add new functions?

If you want to implement new raster indices (let's take LSI for example):

1. Create a new file under src/, e.g. src/lsi.cpp

2. Declare its function prototype in include/gdiv_toolbox.h

3. Implement the logic using GDAL’s raster API

4. Add the file to CMakeLists.txt

5. Rebuild the project with CMake

Example:

```cpp
// src/frag.cpp
#include "gdiv_toolbox.h"
#include <cmath>


int frag_compute(const char* path, const RasterOptions* opt, double* out_val, uint64_t* valid) {
// TODO: Implement your custom raster metric
return 0;
}
```

## Testing for new functions

To add a new test:

Create a file in /tests/, e.g. test_lsi.cpp

Include <gdiv_toolbox.h> and write a main() entry point

Add it to CMake as an executable

```bash
cmake --build build --config Release
.\dist\Release\test_frag.exe
```

## Maintenance Notes

- Always handle NODATA properly using resolve_nodata() in gdiv_utils.h
- Always call gdal_init_once() before reading rasters
- Keep functions small and modular; one metric per file
- Use consistent naming: gdiv_calculate_*
- Use English comments for clarity
- For large rasters, prefer the runner subsystem for tile-based iteration


## License

MIT License — free to use, modify, and redistribute with attribution.
Built with GDAL © Open Source Geospatial Foundation.



