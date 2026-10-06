# Custom vcpkg triplet for cgproject on 64-bit Windows.
#
# Identical to the stock x64-windows triplet, except that Embree's highest
# instruction set is capped at AVX2.
#
# Why: Embree's AVX512 kernels call _mm_rsqrt14_ps() and _mm256_rsqrt14_ps().
# Those intrinsics are missing from MSVC toolsets up to and including 14.37
# (Visual Studio 17.7, which ships them only in their 512-bit form), so
# building Embree from source there fails with
#
#     error C3861: '_mm_rsqrt14_ps': identifier not found
#
# Capping the ISA at AVX2 skips those kernels entirely. The cost is small:
# AVX512 buys Embree roughly 10-20% over AVX2, and only on CPUs that actually
# expose it -- most consumer Intel parts since Alder Lake ship with AVX512
# fused off. It also cuts the Embree build time by about a quarter.
#
# If you are on a recent Visual Studio and want the AVX512 kernels, drop the
# VCPKG_TARGET_TRIPLET line from CMakePresets.json to use the stock triplet.

set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE dynamic)

if(PORT STREQUAL "embree")
    list(APPEND VCPKG_CMAKE_CONFIGURE_OPTIONS -DEMBREE_MAX_ISA=AVX2)
endif()
