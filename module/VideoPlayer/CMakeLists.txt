
cmake_minimum_required(VERSION 3.13)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/lib)

file(GLOB_RECURSE ALL_SOURCE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/EventHandle/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/LogWriter/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Mutex/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/VideoPlayer/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/VideoPlayer/Audio/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/VideoPlayer/Video/*.cpp
        )

message("ALL_SOURCE:"${ALL_SOURCE})
#排除main.cpp
list(REMOVE_ITEM ALL_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)

# 后来你决定添加新的源代码
# 使用target_sources添加文件
target_sources(${PROJECT_NAME} PRIVATE ${ALL_SOURCE})

#把变量传递给上层
set(VIDEOPLAYER_INCLUDE 
    ${CMAKE_CURRENT_SOURCE_DIR} 
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${LIB_INCLUDE}
    CACHE INTERNAL "Test Str")

set(VIDEOPLAYER_LIB  
    ${LIB_FILES}
    CACHE INTERNAL "Test Str")


target_include_directories(${PROJECT_NAME} PUBLIC ${VIDEOPLAYER_INCLUDE})
