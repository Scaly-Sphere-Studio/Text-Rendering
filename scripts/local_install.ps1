$ErrorActionPreference = "Stop";

# Source variables
. .\variables.ps1;

# Expand PATH with script path
$env:PATH += ";$path_ext";

# Install local pkg
InstallLocal.ps1 "$pkg_name" "$export_zip";
