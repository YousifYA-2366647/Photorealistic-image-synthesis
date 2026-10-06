cmake_minimum_required(VERSION 3.21)  # 3.21 for $<TARGET_RUNTIME_DLLS:...>

#-----------------------------------------------------------------------------
# Pick up vcpkg automatically on Windows.
#
# Everyone's reflex is "mkdir build && cd build && cmake ..", which skips the
# vcpkg toolchain file and then fails on the first missing dependency. Rather
# than scold, find vcpkg ourselves. The presets remain the documented path;
# this just makes the habitual invocation work identically, including the
# custom triplet that keeps Embree buildable on older Visual Studio versions
# (see triplets/x64-windows-cgproject.cmake).
#
# One trap this has to dodge: Visual Studio ships its own copy of vcpkg under
# VC/vcpkg, and the Developer Command Prompt points VCPKG_ROOT at it. That
# copy is frozen at the vcpkg-tool version of the VS release -- 2023-06-22 for
# VS 17.7, which cannot read today's port manifests and dies with "mismatched
# type: expected an identifier". Newer VS releases bundle a usable one. So we
# prefer a standalone clone when there is one, and fall back to the bundled
# copy with a warning rather than blocking.
#
# Must come before project().
#-----------------------------------------------------------------------------
function(cg_is_vs_bundled path out)
    file(TO_CMAKE_PATH "${path}" _p)
    string(TOLOWER "${_p}" _p)
    if(_p MATCHES "microsoft visual studio")
        set(${out} TRUE PARENT_SCOPE)
    else()
        set(${out} FALSE PARENT_SCOPE)
    endif()
endfunction()

if(WIN32 AND NOT DEFINED CMAKE_TOOLCHAIN_FILE AND NOT DEFINED ENV{CMAKE_TOOLCHAIN_FILE})

    # Candidate locations, best first. A bootstrapped standalone clone wins.
    set(_cg_vcpkg_candidates "")
    if(DEFINED ENV{VCPKG_ROOT})
        list(APPEND _cg_vcpkg_candidates "$ENV{VCPKG_ROOT}")
    endif()
    list(APPEND _cg_vcpkg_candidates
        "C:/vcpkg"
        "C:/dev/vcpkg"
        "$ENV{USERPROFILE}/vcpkg"
        "$ENV{USERPROFILE}/source/repos/vcpkg")

    set(_cg_vcpkg "")
    set(_cg_vcpkg_vs "")          # a VS-bundled copy we deliberately skipped
    set(_cg_vcpkg_unbootstrapped "")

    foreach(_cand IN LISTS _cg_vcpkg_candidates)
        if(NOT EXISTS "${_cand}/scripts/buildsystems/vcpkg.cmake")
            continue()
        endif()

        cg_is_vs_bundled("${_cand}" _is_vs)
        if(_is_vs)
            if(NOT _cg_vcpkg_vs)
                set(_cg_vcpkg_vs "${_cand}")
            endif()
            continue()            # prefer a standalone clone; see below
        endif()

        if(NOT EXISTS "${_cand}/vcpkg.exe")
            if(NOT _cg_vcpkg_unbootstrapped)
                set(_cg_vcpkg_unbootstrapped "${_cand}")
            endif()
            continue()            # cloned but bootstrap-vcpkg.bat not run yet
        endif()

        set(_cg_vcpkg "${_cand}")
        break()
    endforeach()

    if(_cg_vcpkg)
        set(CMAKE_TOOLCHAIN_FILE "${_cg_vcpkg}/scripts/buildsystems/vcpkg.cmake"
            CACHE STRING "vcpkg toolchain, auto-detected")

        if(NOT DEFINED VCPKG_TARGET_TRIPLET)
            set(VCPKG_TARGET_TRIPLET "x64-windows-cgproject"
                CACHE STRING "see triplets/x64-windows-cgproject.cmake")
            set(VCPKG_OVERLAY_TRIPLETS "${CMAKE_CURRENT_SOURCE_DIR}/triplets"
                CACHE STRING "")
        endif()

        message(STATUS "Using vcpkg: ${_cg_vcpkg}")
        if(_cg_vcpkg_vs)
            message(STATUS
                "  (preferred over the copy bundled with Visual Studio at "
                "${_cg_vcpkg_vs})")
        endif()
        message(STATUS "The first configure builds the dependencies and takes a while.")

    elseif(_cg_vcpkg_unbootstrapped)
        message(FATAL_ERROR
            "\n"
            "  Found a vcpkg checkout at\n"
            "      ${_cg_vcpkg_unbootstrapped}\n"
            "  but it has not been bootstrapped yet, so there is no vcpkg.exe.\n"
            "\n"
            "  Run this once:\n"
            "      ${_cg_vcpkg_unbootstrapped}\\bootstrap-vcpkg.bat\n"
            "\n"
            "  then delete this build directory and configure again.\n")

    elseif(_cg_vcpkg_vs)
        # Last resort. Recent Visual Studio releases bundle a vcpkg new enough
        # to work, so use it rather than blocking -- but say what to do if it
        # turns out to be one of the old ones.
        set(CMAKE_TOOLCHAIN_FILE "${_cg_vcpkg_vs}/scripts/buildsystems/vcpkg.cmake"
            CACHE STRING "vcpkg toolchain (bundled with Visual Studio)")

        if(NOT DEFINED VCPKG_TARGET_TRIPLET)
            set(VCPKG_TARGET_TRIPLET "x64-windows-cgproject"
                CACHE STRING "see triplets/x64-windows-cgproject.cmake")
            set(VCPKG_OVERLAY_TRIPLETS "${CMAKE_CURRENT_SOURCE_DIR}/triplets"
                CACHE STRING "")
        endif()

        message(WARNING
            "\n"
            "  Using the vcpkg bundled with Visual Studio:\n"
            "      ${_cg_vcpkg_vs}\n"
            "\n"
            "  That copy is frozen at whatever vcpkg version your Visual Studio\n"
            "  release shipped with. Recent ones are fine. Older ones cannot read\n"
            "  current package manifests and fail with:\n"
            "      $.default-features[0]: mismatched type: expected an identifier\n"
            "\n"
            "  If you see that, install a standalone vcpkg instead:\n"
            "      git clone https://github.com/microsoft/vcpkg C:/vcpkg\n"
            "      C:/vcpkg/bootstrap-vcpkg.bat\n"
            "      setx VCPKG_ROOT C:\\vcpkg\n"
            "\n"
            "  then open a NEW terminal, delete this build directory and try\n"
            "  again. A clone at C:/vcpkg or %USERPROFILE%/vcpkg is preferred\n"
            "  automatically over this one.\n")

    else()
        message(FATAL_ERROR
            "\n"
            "  This project needs vcpkg to supply its dependencies on Windows,\n"
            "  and none was found.\n"
            "\n"
            "  Install it once:\n"
            "      git clone https://github.com/microsoft/vcpkg C:/vcpkg\n"
            "      C:/vcpkg/bootstrap-vcpkg.bat\n"
            "      setx VCPKG_ROOT C:\\vcpkg\n"
            "\n"
            "  Then open a NEW terminal (so VCPKG_ROOT is visible) and run:\n"
            "      cmake --preset windows\n"
            "      cmake --build --preset windows\n"
            "\n"
            "  See README.md, section 2.\n")
    endif()
endif()

project(cgproject LANGUAGES C CXX)

#-----------------------------------------------------------------------------
# Build options
#
#   USE_EMBREE   ON  -> ray traversal goes through Intel Embree (fast)
#                OFF -> ray traversal uses the original srtk uniform grid.
#                       Turn this off if you want to write (or study) an
#                       acceleration structure yourself: both backends sit
#                       behind the exact same rtShootRay()/rtTestVisibility()
#                       API declared in libs/srtk/rt.h.
#
#   USE_OPENMP   Parallelise the per-pixel loops. OFF by default so that
#                single-stepping through your shader in a debugger behaves
#                sanely. Requires USE_EMBREE=ON: the srtk grid mutates the
#                scene while tracing and is *not* thread safe.
#
#   USE_ASSIMP   Build the Open Asset Import Library model loader (.3ds,
#                .obj, .ply, .gltf, .fbx, ...). The .mgf loader is always
#                built and has no external dependencies.
#-----------------------------------------------------------------------------
option(USE_EMBREE "Use Intel Embree for ray traversal" ON)
option(USE_OPENMP "Parallelise the pixel loops with OpenMP" OFF)
option(USE_ASSIMP "Build the Assimp-based model loader" ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Default to a Release build: a debug build of a ray tracer is ~10x slower.
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

# Drop the executable next to the models so that "cgproject models/soda.mgf"
# works from the build directory too.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

#-----------------------------------------------------------------------------
# Dependencies
#
# A missing dependency should tell you how to install it, not bury you in
# CMake internals. cg_missing() does that.
#-----------------------------------------------------------------------------
function(cg_missing what apt dnf brew vcpkg)
    if(VCPKG_TOOLCHAIN)
        # Manifest mode: packages come from vcpkg.json, never from a manual
        # "vcpkg install <port>" -- vcpkg rejects that outright here. So if we
        # got this far, the automatic install did not produce the package.
        set(hint
            "  This build uses vcpkg in manifest mode, so '${vcpkg}' should have been\n"
            "  installed automatically from vcpkg.json. Do NOT run 'vcpkg install ${vcpkg}';\n"
            "  in manifest mode vcpkg refuses per-package arguments.\n"
            "\n"
            "  Instead, delete the build directory and configure again:\n"
            "      cmake --preset windows\n"
            "\n"
            "  If it still fails, the vcpkg build log above the first error says why.\n")
    elseif(WIN32)
        set(hint
            "  It looks like this build directory was configured without vcpkg.\n"
            "  Use the preset, which wires up the toolchain for you:\n"
            "      cmake --preset windows\n"
            "      cmake --build --preset windows\n")
    elseif(APPLE)
        set(hint "  Install it with:\n      brew install ${brew}\n")
    else()
        set(hint
            "  Install it with:\n"
            "      sudo apt install ${apt}\n"
            "      (Fedora: sudo dnf install ${dnf})\n")
    endif()
    message(FATAL_ERROR
        "\n"
        "  Could not find ${what}.\n"
        "\n"
        ${hint}
        "\n"
        "  See README.md for the full setup for your platform.\n")
endfunction()

find_package(OpenGL REQUIRED)

find_package(glfw3 3.3 CONFIG QUIET)
if(NOT glfw3_FOUND)
    cg_missing("GLFW 3 (the window/input library)"
               "libglfw3-dev" "glfw-devel" "glfw" "glfw3")
endif()

if(USE_EMBREE)
    # Accept a prebuilt Embree SDK unpacked into third_party/ (or pointed at
    # by the EMBREE_ROOT environment variable) as well as one provided by
    # vcpkg or the system package manager. Intel's official binary release
    # ships the CMake config package, so it is found the same way.
    if(NOT embree_DIR)
        file(GLOB _embree_cfg
             "${CMAKE_CURRENT_SOURCE_DIR}/third_party/embree*/lib/cmake/embree-*"
             "$ENV{EMBREE_ROOT}/lib/cmake/embree-*")
        if(_embree_cfg)
            list(GET _embree_cfg 0 embree_DIR)
            message(STATUS "Using prebuilt Embree SDK: ${embree_DIR}")
        endif()
    endif()

    # Embree 4 is preferred, Embree 3 (e.g. Ubuntu 22.04's libembree-dev
    # 3.12) works too.
    find_package(embree 4 CONFIG QUIET)
    if(NOT embree_FOUND)
        find_package(embree 3 CONFIG QUIET)
    endif()
    if(NOT embree_FOUND)
        cg_missing("Embree 3 or 4 (the ray traversal kernel).\n"
                   "  Alternatively, unpack Intel's prebuilt Embree release into\n"
                   "  third_party/ , or configure with -DUSE_EMBREE=OFF to fall back\n"
                   "  to the (slower, single threaded) built-in uniform grid"
                   "libembree-dev" "embree-devel" "embree" "embree")
    endif()
endif()

if(USE_ASSIMP)
    find_package(assimp CONFIG QUIET)
    if(NOT assimp_FOUND)
        cg_missing("Assimp (the model loader).\n"
                   "  Or configure with -DUSE_ASSIMP=OFF to build with only the\n"
                   "  .mgf loader, which has no external dependencies"
                   "libassimp-dev" "assimp-devel" "assimp" "assimp")
    endif()

    # stb_image loads texture images referenced by a model. vcpkg provides it
    # via the 'stb' port (assimp already depends on it), Debian/Ubuntu via
    # 'libstb-dev'. Textures are simply skipped if it is absent, so this one
    # is not fatal.
    find_path(STB_INCLUDE_DIR NAMES stb_image.h PATH_SUFFIXES stb)
endif()

if(USE_OPENMP)
    if(NOT USE_EMBREE)
        message(FATAL_ERROR
            "USE_OPENMP requires USE_EMBREE=ON. The srtk uniform grid writes "
            "ray IDs into the scene while tracing and cannot be used from "
            "multiple threads.")
    endif()
    find_package(OpenMP REQUIRED)
endif()

add_subdirectory(libs/srtk)
add_subdirectory(libs/mgflib)

#-----------------------------------------------------------------------------
# The application
#-----------------------------------------------------------------------------
add_executable(cgproject
    main.cpp
    cie.cpp
    rtIndexedFaceSet.cpp
    rtMgf.cpp
    rtOpenGL.cpp
    ScreenBuffer.cpp
)

if(USE_ASSIMP)
    target_sources(cgproject PRIVATE rtAssimp.cpp)
endif()

target_include_directories(cgproject PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/libs
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/srtk
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/mgflib
)

target_link_libraries(cgproject PRIVATE
    srtk
    mgf
    glfw
    OpenGL::GL
    OpenGL::GLU
)

if(USE_ASSIMP)
    target_link_libraries(cgproject PRIVATE assimp::assimp)
    target_compile_definitions(cgproject PRIVATE HAVE_ASSIMP)
    if(STB_INCLUDE_DIR)
        target_include_directories(cgproject PRIVATE ${STB_INCLUDE_DIR})
        target_compile_definitions(cgproject PRIVATE HAVE_STB_IMAGE)
    else()
        message(STATUS
            "stb_image.h not found: models will load without their textures. "
            "Install the 'stb' (vcpkg) or 'libstb-dev' (apt) package to enable them.")
    endif()
endif()

if(USE_OPENMP)
    target_link_libraries(cgproject PRIVATE OpenMP::OpenMP_CXX)
    target_compile_definitions(cgproject PRIVATE HAVE_OPENMP)
endif()

# The fixed-function pipeline is formally deprecated on macOS; silence that
# wall of warnings rather than the real ones.
if(APPLE)
    target_compile_options(cgproject PRIVATE -Wno-deprecated-declarations)
endif()

if(MSVC)
    # The framework predates the secure-CRT warnings by about a decade.
    target_compile_definitions(cgproject PRIVATE _CRT_SECURE_NO_WARNINGS)
    # Let the students see their own warnings, not 4000 lines of vendor noise.
    target_compile_options(cgproject PRIVATE /W3)
endif()

#-----------------------------------------------------------------------------
# Copy the models next to the executable so it can be run straight from the
# build tree, and report the configuration.
#-----------------------------------------------------------------------------
add_custom_command(TARGET cgproject POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/models
            $<TARGET_FILE_DIR:cgproject>/models
    COMMENT "Copying models next to the executable")

# On Windows the DLLs have to sit next to the .exe. vcpkg arranges this by
# itself, but a prebuilt Embree SDK unpacked into third_party/ does not, and
# the failure mode is an unhelpful "0xc000007b" dialog at startup.
if(WIN32)
    add_custom_command(TARGET cgproject POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_RUNTIME_DLLS:cgproject> $<TARGET_FILE_DIR:cgproject>
        COMMAND_EXPAND_LISTS
        COMMENT "Copying dependency DLLs next to the executable")
endif()

if(USE_EMBREE)
    set(_traversal "Embree ${embree_VERSION}")
else()
    set(_traversal "srtk uniform grid (single threaded)")
endif()
if(USE_ASSIMP)
    set(_loaders "mgf, assimp ${assimp_VERSION}")
else()
    set(_loaders "mgf")
endif()

message(STATUS "")
message(STATUS "cgproject configuration")
message(STATUS "  ray traversal : ${_traversal}")
message(STATUS "  model loaders : ${_loaders}")
message(STATUS "  OpenMP        : ${USE_OPENMP}")
if(CMAKE_CONFIGURATION_TYPES)
    message(STATUS "  build type    : chosen at build time (--config Release)")
else()
    message(STATUS "  build type    : ${CMAKE_BUILD_TYPE}")
endif()
message(STATUS "")
