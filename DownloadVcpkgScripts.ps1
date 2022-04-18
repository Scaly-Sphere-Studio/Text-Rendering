git -C $PSScriptRoot submodule add https://github.com/Scaly-Sphere-Studio vcpkg_scripts;
git -C $PSScriptRoot submodule foreach git fetch;
git -C $PSScriptRoot submodule update --recursive --remote;