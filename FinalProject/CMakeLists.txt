cmake_minimum_required(VERSION 3.10)
project(MAT201B_FinalProject)

# === 设置 AlloLib 和 al_ext 根路径 ===
set(ALLOLIB_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../../../../allolib")
set(AL_EXT_ROOT  "${CMAKE_CURRENT_SOURCE_DIR}/../../../../../al_ext")

# === 链接 AlloLib 构建目录中的库 ===
link_directories(
  ${ALLOLIB_ROOT}/build/lib
  ${AL_EXT_ROOT}/assets3d
)

# === 全局头文件路径设置 ===
include_directories(
  ${ALLOLIB_ROOT}
  ${ALLOLIB_ROOT}/include
  ${AL_EXT_ROOT}/assets3d
  ${ALLOLIB_ROOT}/..
  ${ALLOLIB_ROOT}/external/Gamma  # ✅ 添加 Gamma 的头文件路径
)

# === 默认目标：只链接 allolib 核心模块 al 和 Gamma（如果需要） ===
add_executable(voronoi voronoi.cpp)
target_link_libraries(voronoi PRIVATE al Gamma)

add_executable(voronoi3d voronoi3d.cpp)
target_link_libraries(voronoi3d PRIVATE al Gamma)

add_executable(testing2 testing2.cpp)
target_link_libraries(testing2 PRIVATE al Gamma)

add_executable(theredwire theredwire.cpp)
target_link_libraries(theredwire PRIVATE al Gamma)

add_executable(Finalproject Finalproject.cpp)
target_link_libraries(Finalproject PRIVATE al Gamma)

add_executable(voronoibreaking voronoibreaking.cpp)
target_link_libraries(voronoibreaking PRIVATE al Gamma)

add_executable(voronoibreaking2 voronoibreaking2.cpp)
target_link_libraries(voronoibreaking2 PRIVATE al Gamma)

# === 特殊目标：loadobj.cpp 需要 al_assets3d ===
add_executable(loadobj loadobj.cpp)
target_link_libraries(loadobj PRIVATE al al_assets3d)
target_include_directories(loadobj PRIVATE
  ${AL_EXT_ROOT}/assets3d
  ${ALLOLIB_ROOT}/..
)