# mgflib: the reference Materials and Geometry Format parser (LBL, 1994-1995).
#
# This is pre-standard K&R C: old-style function definitions, `register`, and
# prototype-less `extern char *strchr();` declarations. Those were removed
# from the language in C23, and GCC 15 compiles as C23 by default, so the
# target is pinned to C90 rather than patched. It is third-party code we do
# not otherwise modify.

add_library(mgf STATIC
    parser.c
    context.c
    xf.c
    object.c
    lookup.c
    badarg.c
    words.c
    fvect.c
)

target_include_directories(mgf PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

set_target_properties(mgf PROPERTIES
    C_STANDARD 90
    C_STANDARD_REQUIRED OFF   # MSVC has no C90 mode; it is permissive enough
    C_EXTENSIONS ON
)

if(MSVC)
    target_compile_definitions(mgf PRIVATE _CRT_SECURE_NO_WARNINGS)
    # C4113: K&R function pointer assignments.
    # C4005: NULL redefined in words.c / badarg.c.
    # C4273: parser.h redeclares malloc/calloc/realloc/free without
    #        prototypes, as was normal in 1994.
    # All expected for this code, none of them actionable.
    target_compile_options(mgf PRIVATE /wd4113 /wd4005 /wd4273)
else()
    target_compile_options(mgf PRIVATE -w)
endif()
