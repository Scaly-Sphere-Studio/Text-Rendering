# --- FUNCTIONS & VARIABLES ---

# CopyBinary copies the given file to the given folder. Fatal if it does not exist
function(CopyBinary src_dir file_name relative_dst)
    message(STATUS "> ${file_name}")
    # Ensure binary is present
    if (NOT EXISTS "${src_dir}/${file_name}")
        message(FATAL_ERROR "File does not exist: ${src_dir}/${file_name}")
    endif()
    # Copy file
    file(COPY "${src_dir}/${file_name}" DESTINATION "${CURRENT_PACKAGES_DIR}/${relative_dst}")
endfunction()

# arch_dir is the path to the current architecture build, fatal if unsupported architecture
if (VCPKG_TARGET_ARCHITECTURE STREQUAL "x86")
    set(arch_dir "")
elseif (VCPKG_TARGET_ARCHITECTURE STREQUAL "x64")
    set(arch_dir "x64")
else()
    message(FATAL_ERROR "Unsupported architecture: ${VCPKG_TARGET_ARCHITECTURE}")
endif()

# Set paths to headers
set(base_dir "${CMAKE_CURRENT_LIST_DIR}")
set(inc_dir  "${base_dir}/include")
get_filename_component(pkg_name "${base_dir}" NAME)

# --- BINARIES ---

# Release
if (NOT VCPKG_BUILD_TYPE OR VCPKG_BUILD_TYPE STREQUAL "release")
    message(STATUS "Copying release binaries...")
    # Copy binaries
    set(build_dir "${base_dir}/${arch_dir}/Release")
    CopyBinary(${build_dir} "${pkg_name}.lib" "lib")
endif()

# Debug
if (NOT VCPKG_BUILD_TYPE OR VCPKG_BUILD_TYPE STREQUAL "debug")
    message(STATUS "Copying debug binaries...")
    # Copy binaries
    set(build_dir "${base_dir}/${arch_dir}/Debug")
    CopyBinary(${build_dir} "${pkg_name}.lib" "debug/lib")
endif()

# --- HEADERS ---

message(STATUS "Copying headers...")
# Retrieve all headers
file(GLOB_RECURSE headers "${inc_dir}/*.h")
# Copy headers keeping directory structure
foreach (header ${headers})
    # Retrieve relative path
    file(RELATIVE_PATH relative_header "${inc_dir}" "${header}")
    message(STATUS "> ${relative_header}")
    # Set absolute path of target file accordingly
    set(target_file "${CURRENT_PACKAGES_DIR}/include/${relative_header}")
    # Extract directory
    get_filename_component(target_dir ${target_file} DIRECTORY)
    # Copy header to that directory
    file(COPY ${header} DESTINATION ${target_dir})
endforeach()

# --- SHARE ---

message(STATUS "Copying copyright...")
# Create share folder
file(MAKE_DIRECTORY "${CURRENT_PACKAGES_DIR}/share/${pkg_name}")
# Write copyright file
file(WRITE "${CURRENT_PACKAGES_DIR}/share/${pkg_name}/copyright" "SSS use only")
