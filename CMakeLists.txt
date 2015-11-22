project(abcrender)
cmake_minimum_required(VERSION 2.8)
set(CMAKE_BUILD_TYPE Debug)
find_package(ImageMagick COMPONENTS Magick++ MagickCore REQUIRED)

find_path(GLM_INCLUDE_DIR glm/glm.hpp)
message(STATUS "glm ${GLM_INCLUDE_DIR}")

find_path(ALEMBIC_INCLUDE_DIR Alembic/Abc/All.h)
message(STATUS "alembic include ${ALEMBIC_INCLUDE_DIR}")

find_path(EXR_INCLUDE_DIR OpenEXR/half.h)
message(STATUS "openexr include ${EXR_INCLUDE_DIR}")

include_directories(
${GLM_INCLUDE_DIR}
${ImageMagick_INCLUDE_DIRS}
${ALEMBIC_INCLUDE_DIR}
${EXR_INCLUDE_DIR}/OpenEXR
)

set(SCENE_LIBRARIES "")

foreach (LIB Alembic Imath Half Iex hdf5 hdf5_hl)
    find_library(FOUND${LIB} ${LIB})
    message(STATUS "   ${LIB} ${FOUND${LIB}}")
    set(SCENE_LIBRARIES ${SCENE_LIBRARIES} ${FOUND${LIB}})
endforeach(LIB)

add_executable(abcrender
main.cpp
rendercontext.cpp
vertex.cpp
edge.cpp
gradient.cpp
abcrender.cpp
)

set_property(TARGET abcrender PROPERTY CXX_STANDARD 11)
set_property(TARGET abcrender PROPERTY CXX_STANDARD_REQUIRED ON)

target_link_libraries(abcrender
${ImageMagick_LIBRARIES}
${SCENE_LIBRARIES}
)

install(TARGETS abcrender DESTINATION bin)
