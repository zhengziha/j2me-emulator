cmake_minimum_required(VERSION 3.15)
project(J2MEEmulator VERSION 1.0.0 LANGUAGES C CXX)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 编译选项
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_definitions(DEBUG)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -DNDEBUG")
endif()

# 查找SDL2
find_package(SDL2 REQUIRED)

# 查找SDL2_mixer (用于音频)
find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2_MIXER REQUIRED SDL2_mixer)

# 查找libcurl (用于网络)
pkg_check_modules(LIBCURL REQUIRED libcurl)

# 包含目录
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${SDL2_INCLUDE_DIRS})
include_directories(${SDL2_MIXER_INCLUDE_DIRS})
include_directories(${LIBCURL_INCLUDE_DIRS})

# 添加数学库链接 (用于图形计算)
find_library(MATH_LIBRARY m)

# 源文件
file(GLOB_RECURSE SOURCES 
    "src/*.cpp"
    "src/*.c"
)

# 排除不需要的文件
list(FILTER SOURCES EXCLUDE REGEX ".*test.*")

# 创建可执行文件
add_executable(${PROJECT_NAME} ${SOURCES})

# 链接库
target_link_libraries(${PROJECT_NAME} ${SDL2_LIBRARIES})
target_link_libraries(${PROJECT_NAME} ${SDL2_MIXER_LIBRARIES})
target_link_libraries(${PROJECT_NAME} ${LIBCURL_LIBRARIES})
target_link_directories(${PROJECT_NAME} PRIVATE ${SDL2_MIXER_LIBRARY_DIRS})
target_link_directories(${PROJECT_NAME} PRIVATE ${LIBCURL_LIBRARY_DIRS})

# 链接数学库 (如果找到)
if(MATH_LIBRARY)
    target_link_libraries(${PROJECT_NAME} ${MATH_LIBRARY})
endif()

# 平台特定设置
if(WIN32)
    target_link_libraries(${PROJECT_NAME} winmm)
elseif(APPLE)
    target_link_libraries(${PROJECT_NAME} "-framework CoreFoundation")
endif()

# 安装规则
install(TARGETS ${PROJECT_NAME} DESTINATION bin)