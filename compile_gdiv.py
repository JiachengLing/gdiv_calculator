# run_calc_table.py — For each factor, output {factor}_msr (=std) and {factor}_mean.
#                     SHDI/LSI are computed only for whitelisted factors.
#                     SHDI can optionally output class proportion columns.

from pathlib import Path
import ctypes as C
import pandas as pd
import numpy as np
import sys, os

# === 1) Set directories ===
BASE_DIR  = Path(r"E:\gdiv_calculator")
DATA_ROOT = BASE_DIR / "DATA"
DLL_DIR   = BASE_DIR / "dist" / "Release"         # or "Debug"
DLL_PATH  = DLL_DIR / "gdiv_toolbox.dll"

# Factors that are categorical (SHDI and LSI can be computed for them)
SHDI_FACTORS = ["SoilType", "Forest"]
LSI_FACTORS  = ["SoilType", "Forest"]

# Define which class codes are included in SHDI computation.
# If a factor is missing here, it will be skipped (to avoid miscalculation).
SHDI_CLASSES = {
    "SoilType": [1, 2, 3, 4, 5],
    "Forest":   [0, 1]
}

# Write SHDI class proportions to CSV?
# If True, columns like {factor}_cls<code>_p will be added.
WRITE_SHDI_PROBS = True

# === 2) Load DLL ===
os.add_dll_directory(str(DLL_DIR.resolve()))
if "CONDA_PREFIX" in os.environ:
    os.add_dll_directory(os.environ["CONDA_PREFIX"] + r"\Library\bin")

gdiv = C.CDLL(str(DLL_PATH))

# === 3) Bind to C interfaces ===
# int gdiv_calculate_msr(const char* path, const RasterOptions* opt,
#                        double* mean, double* stdv, double* vmin, double* vmax, uint64_t* valid);
gdiv.gdiv_calculate_msr.argtypes = [
    C.c_char_p, C.c_void_p,
    C.POINTER(C.c_double), C.POINTER(C.c_double),
    C.POINTER(C.c_double), C.POINTER(C.c_double),
    C.POINTER(C.c_uint64)
]
gdiv.gdiv_calculate_msr.restype = C.c_int

# int gdiv_calculate_shdi(const char* path,
#                         const double* classes, int n_classes,
#                         const RasterOptions* opt,
#                         double* out_shdi, double* probs, uint64_t* out_valid);
gdiv.gdiv_calculate_shdi.argtypes = [
    C.c_char_p,
    C.POINTER(C.c_double), C.c_int,
    C.c_void_p,
    C.POINTER(C.c_double), C.POINTER(C.c_double),
    C.POINTER(C.c_uint64)
]
gdiv.gdiv_calculate_shdi.restype = C.c_int

# int gdiv_calculate_lsi(const char* path, const RasterOptions* opt,
#                        double* out_lsi, uint64_t* out_valid);
gdiv.gdiv_calculate_lsi.argtypes = [
    C.c_char_p, C.c_void_p,
    C.POINTER(C.c_double), C.POINTER(C.c_uint64)
]
gdiv.gdiv_calculate_lsi.restype = C.c_int

# === 4) Small helper wrappers ===
def compute_msr_mean_and_msr(path: Path):
    """Return (msr, mean), where msr = std (standard deviation)."""
    p = str(path).encode("utf-8")
    mean = C.c_double(); stdv = C.c_double()
    vmin = C.c_double(); vmax = C.c_double()
    valid = C.c_uint64()
    ret = gdiv.gdiv_calculate_msr(
        p, None, C.byref(mean), C.byref(stdv),
        C.byref(vmin), C.byref(vmax), C.byref(valid)
    )
    if ret != 0:
        raise RuntimeError(f"gdiv_calculate_msr failed: code={ret}, path={path}")
    return stdv.value, mean.value  # (msr=std, mean)

def compute_shdi_and_probs(path: Path, classes):
    """Return (H, probs_array); probs_array has length = len(classes)."""
    if not classes:
        raise ValueError("SHDI classes is empty.")
    p = str(path).encode("utf-8")
    cls = np.asarray(classes, dtype=np.float64)
    H = C.c_double()
    probs_buf = (C.c_double * len(cls))()
    valid = C.c_uint64()
    ret = gdiv.gdiv_calculate_shdi(
        p,
        cls.ctypes.data_as(C.POINTER(C.c_double)), len(cls),
        None,
        C.byref(H), probs_buf, C.byref(valid)
    )
    if ret != 0:
        raise RuntimeError(f"gdiv_calculate_shdi failed: code={ret}, path={path}")
    probs = [probs_buf[i] for i in range(len(cls))]
    return H.value, probs

def compute_lsi(path: Path):
    """Return the LSI value."""
    p = str(path).encode("utf-8")
    out = C.c_double()
    valid = C.c_uint64()
    ret = gdiv.gdiv_calculate_lsi(p, None, C.byref(out), C.byref(valid))
    if ret != 0:
        raise RuntimeError(f"gdiv_calculate_lsi failed: code={ret}, path={path}")
    return out.value

# === 5) Scan data folders ===
# factor = subfolder name
# siteID = filename (without extension)
factors = []
factor_to_files = {}  # {factor: {siteID: Path}}
for sub in sorted(p for p in DATA_ROOT.iterdir() if p.is_dir()):
    factor = sub.name
    factors.append(factor)
    mapping = {}
    for tif in sorted(list(sub.glob("*.tif")) + list(sub.glob("*.tiff"))):
        mapping[tif.stem] = tif
    factor_to_files[factor] = mapping

# Union of all siteIDs across all factors
all_sites = sorted(set().union(*(m.keys() for m in factor_to_files.values())))

# === 6) Calculate metrics and merge results ===
rows = []
for site in all_sites:
    row = {"siteID": site}
    for factor in factors:
        tif = factor_to_files[factor].get(site)

        # Predeclare columns for this factor
        col_msr  = f"{factor}_msr"   # MSR = std
        col_mean = f"{factor}_mean"
        row[col_msr] = None
        row[col_mean] = None

        shdi_cols = []
        if factor in SHDI_FACTORS:
            row[f"{factor}_shdi"] = None
            if WRITE_SHDI_PROBS:
                for code in SHDI_CLASSES.get(factor, []):
                    cname = f"{factor}_cls{code}_p"
                    row[cname] = None
                    shdi_cols.append(cname)

        if factor in LSI_FACTORS:
            row[f"{factor}_lsi"] = None

        if tif is None:
            continue

        # --- Compute MSR (std) and mean: always computed for all factors ---
        try:
            msr_std, mean_val = compute_msr_mean_and_msr(tif)
            row[col_msr]  = msr_std
            row[col_mean] = mean_val
        except Exception as e:
            print(f"[WARN] MSR/mean failed: factor={factor}, site={site}, path={tif}\n  -> {e}", file=sys.stderr)

        # --- SHDI ---
        if factor in SHDI_FACTORS:
            classes = SHDI_CLASSES.get(factor, [])
            if classes:
                try:
                    H, probs = compute_shdi_and_probs(tif, classes)
                    row[f"{factor}_shdi"] = H
                    if WRITE_SHDI_PROBS:
                        for code, pval in zip(classes, probs):
                            row[f"{factor}_cls{code}_p"] = pval
                except Exception as e:
                    print(f"[WARN] SHDI failed: factor={factor}, site={site}, path={tif}\n  -> {e}", file=sys.stderr)
            else:
                print(f"[INFO] SHDI skipped (no classes configured) for factor={factor}", file=sys.stderr)

        # --- LSI ---
        if factor in LSI_FACTORS:
            try:
                row[f"{factor}_lsi"] = compute_lsi(tif)
            except Exception as e:
                print(f"[WARN] LSI failed: factor={factor}, site={site}, path={tif}\n  -> {e}", file=sys.stderr)

    rows.append(row)

df = pd.DataFrame(rows).sort_values("siteID")

# Arrange column order: siteID → each factor’s msr/mean → SHDI (+class proportions) → LSI
ordered = ["siteID"]
for f in factors:
    ordered += [f"{f}_msr", f"{f}_mean"]
for f in factors:
    if f in SHDI_FACTORS:
        ordered.append(f"{f}_shdi")
        if WRITE_SHDI_PROBS:
            for code in SHDI_CLASSES.get(f, []):
                ordered.append(f"{f}_cls{code}_p")
for f in factors:
    if f in LSI_FACTORS:
        ordered.append(f"{f}_lsi")

df = df.reindex(columns=ordered)

# === 7) Save to CSV ===
OUT = BASE_DIR / "dist" / "results_metrics.csv"
OUT.parent.mkdir(parents=True, exist_ok=True)
df.to_csv(OUT, index=False, encoding="utf-8-sig")
print(f"Saved -> {OUT}")
