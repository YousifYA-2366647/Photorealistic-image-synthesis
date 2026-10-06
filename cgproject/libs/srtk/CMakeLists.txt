# srtk: Simple Ray Tracing Kernel
#
# Scene description (materials, textures, triangle sets) plus two
# interchangeable ray traversal backends -- see USE_EMBREE in the top-level
# CMakeLists.txt.

add_library(srtk STATIC
    Bounds.cpp
    Cluster.cpp
    Grid.cpp
    Hit.cpp
    Material.cpp
    OpenGL.cpp
    State.cpp
    Texture.cpp
    Triangle.cpp
)

target_include_directories(srtk PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(srtk PUBLIC OpenGL::GL)

if(USE_EMBREE)
    target_sources(srtk PRIVATE EmbreeScene.cpp)
    target_link_libraries(srtk PUBLIC embree)
    target_compile_definitions(srtk PUBLIC SRTK_USE_EMBREE)
endif()

if(MSVC)
    target_compile_definitions(srtk PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()
