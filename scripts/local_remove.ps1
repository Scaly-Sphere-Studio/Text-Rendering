$ErrorActionPreference = "Stop";

# Source pkg parameters
. .\variables.ps1;

# Expand PATH with script path
$env:PATH += ";$path_ext";

# Remove local pkg
UninstallLocal.ps1 "$pkg_name";
