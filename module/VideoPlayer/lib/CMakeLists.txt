
cmake_minimum_required(VERSION 3.13)

set(FFMPEG_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/include CACHE INTERNAL "Test Str")

set(FFMPEG_BRANCH_NAME "V4.3.1")
set(SDL2_BRANCH_NAME   "V2.0.2")

if(UNIX)
    message("current platform: Linux ")
    set(FFMPEG_BRANCH_NAME "V4.3.1-linux")
    set(SDL2_BRANCH_NAME   "V2.0.12-linux")
elseif(WIN32)
    message("current platform: Windows ")
    set(FFMPEG_BRANCH_NAME "V4.3.1-windows")
    set(SDL2_BRANCH_NAME   "V2.0.2-windows")
endif() # WIN32

### 拉取用到的库文件 ###
if(EXISTS ${CMAKE_CURRENT_LIST_DIR}/ffmpeg/)
message("ffmpeg库文件文件存在")
else()
message("ffmpeg库文件文件不存在, 开始拉取...")
# 执行git命令，拉取ffmpeg库文件
execute_process(
    COMMAND git clone https://gitee.com/devlib/ffmpeg-dev.git ffmpeg -b ${FFMPEG_BRANCH_NAME}
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
)
endif()

if(EXISTS ${CMAKE_CURRENT_LIST_DIR}/SDL2/)
message("SDL2库文件文件存在")
else()
message("SDL2库文件文件不存在, 开始拉取...")

# 执行git命令，拉取SDL2库文件
execute_process(
    COMMAND git clone https://gitee.com/devlib/SDL2-dev.git SDL2 -b ${SDL2_BRANCH_NAME}
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
)
endif()

#---------------- ADD LIBS ---------------#
## FFMPEG
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/ffmpeg)

## SDL
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/SDL2)

#---------------- ADD LIBS ---------------#


#把变量传递给上层
set(LIB_INCLUDE  
    ${FFMPEG_INCLUDE} ${SDL2_INCLUDE}
    CACHE INTERNAL "Test Str")

set(LIB_FILES  
    ${FFMPEG_LIB} ${SDL2_LIB}
    CACHE INTERNAL "Test Str")