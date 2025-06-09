
cmake_minimum_required(VERSION 3.13)

set(USE_PCM_PLAYER 1)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/lib)

file(GLOB_RECURSE ALL_SOURCE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/VideoPlayer/*.cpp
        )

#排除main.cpp
list(REMOVE_ITEM ALL_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)

if (USE_PCM_PLAYER)
    # add_definitions(-DUSE_PCM_PLAYER -DENABLE_SDL)
    target_compile_definitions (${PROJECT_NAME} PRIVATE USE_PCM_PLAYER ENABLE_SDL)
else()
    #排除PcmPlayer
    list(REMOVE_ITEM ALL_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/src/PcmPlayer/PcmPlayer_SDL.cpp)
endif()

message("ALL_SOURCE:"${ALL_SOURCE})

# 后来你决定添加新的源代码
# 使用target_sources添加文件
target_sources(${PROJECT_NAME} PRIVATE ${ALL_SOURCE})

#把变量传递给上层
set(VIDEOPLAYER_INCLUDE 
    ${CMAKE_CURRENT_SOURCE_DIR} 
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/frame
    ${LIB_INCLUDE}
    CACHE INTERNAL "Test Str")

set(VIDEOPLAYER_LIB  
    ${LIB_FILES}
    CACHE INTERNAL "Test Str")


target_include_directories(${PROJECT_NAME} PUBLIC ${VIDEOPLAYER_INCLUDE})
