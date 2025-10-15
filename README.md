# gdiv_calculator 

This is a C++ geodiversity (spatial metrics) calculation toolbox for raster analysis,
designed to compute spatial indices including mean, standard deviation, shannon diversity index of geoelements and landscape shape index using the GDAL library.
Functions can be extended in the future.

---
## Description

`gdiv_calculator` provides a set of tools for environmental and biodiversity studies.  
It currently supports calculating statistical metrics (mean, std, min, max, count) from single-band rasters,  
now it support landscape metrics such as **Mean**, **Variance**, **SHDI**, and **LSI**.

Core features include:
- modular C++ design
- Built on [GDAL](https://gdal.org) for raster access
- Ready for integration with Python bindings
- Memory-efficient block reading for large rasters

## Directory Structure
```
gdiv_calculator/
├── CMakeLists.txt
├── include/
│ └── gdiv_toolbox.h # Public API declarations
├── src/
│ ├── gdiv_toolbox.cpp # Core raster statistics
│ ├── gdiv_utils.cpp/.h # Utilities (GDAL init, NODATA handling, etc.)
│ ├── msr.cpp # Mean-Std-Raster computation
│ ├── shdi.cpp # Shannon Diversity Index (planned)
│ ├── lsi.cpp # Landscape Shape Index (planned)
│ └── export.h # Export macros for DLL builds
├── tests/
│ └── test_basic.cpp # Example test for MSR computation
├── example_raster/
│ └── T5FPCF.tiff # An example raster dataset
└── dist/
└── Release/ # Compiled DLLs and executables
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

## Run example 

After successful build, run the test binary:'

```Markdown
cd dist/Release
.\test_basic.exe "E:\gdiv_calculator\example_raster\T5FPCF.tiff"
```

## How to add new functions?

If you want to implement new raster indices (let's take LSI for example):

Create a new file under src/, e.g. src/lsi.cpp

Declare its function prototype in include/gdiv_toolbox.h

Implement the logic using GDAL’s raster API

Add the file to CMakeLists.txt

Rebuild the project with CMake

Example:

```cpp
// src/lsi.cpp
#include "gdiv_toolbox.h"
#include <cmath>

int gdiv_calculate_lsi(const char* path, const RasterOptions* opt, double* lsi_value) {
    // Implement LSI formula using pixel connectivity or edge detection
    return 0;
}
```

## Testing

To add a new test:

Create a file in /tests/, e.g. test_lsi.cpp

Include <gdiv_toolbox.h> and write a main() entry point

Add it to CMake as an executable

```bash
cmake --build build --config Release
.\dist\Release\test_lsi.exe
```

## Maintenance Notes

All functions should handle NODATA properly using resolve_nodata() from gdiv_utils.h

Always use gdiv_init_gdal_once() before reading rasters

Keep functions small and modular; do not mix multiple index calculations in one file

Use English comments and follow consistent naming: gdiv_calculate_*




