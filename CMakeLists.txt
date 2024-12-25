cmake_minimum_required(VERSION 3.13)

# set(PRODUCT_NAME TransCodeIpcAudio)

#直接获取cmakelists.txt所在目录名 赋值给PRODUCT_NAME
get_filename_component(PRODUCT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)

project(${PRODUCT_NAME})

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

#CMAKE_ARCHIVE_OUTPUT_DIRECTORY：默认存放静态库的文件夹位置；
#CMAKE_LIBRARY_OUTPUT_DIRECTORY：默认存放动态库的文件夹位置；
#LIBRARY_OUTPUT_PATH：默认存放库文件的位置，如果产生的是静态库并且没有指定
#CMAKE_ARCHIVE_OUTPUT_DIRECTORY 则存放在该目录下，动态库也类似；
#CMAKE_RUNTIME_OUTPUT_DIRECTORY：存放可执行软件的目录；
#set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/archive)
#set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/library)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/win64)
#set(LIBRARY_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/path)

if(UNIX)

    message("current platform: Linux ")

    set(CMAKE_PREFIX_PATH "/opt/Qt/Qt5.13.2")
    set(QT_QMAKE_EXECUTABLE "/opt/Qt/Qt5.13.2/bin/qmake")

elseif(WIN32)

    message("current platform: Windows ")

    #指定Qt安装目录
    set(QT_DIR_ROOT 
            C:/Qt/Qt5.13.2/5.13.2
            D:/Qt/Qt5.13.2/5.13.2)
    
    if(CMAKE_CL_64)    #CMAKE的内建变量，如果是true，就说明编译器的64位的，自然可以编译64bit的程序
        set(QT_DIR_NAME msvc2017_64)

        # set(CMAKE_PREFIX_PATH ${QT_DIR_ROOT}/msvc2017_64)
        # list(APPEND CMAKE_PREFIX_PATH ${QT_DIR_ROOT}/msvc2017_64)
    else()
        set(QT_DIR_NAME msvc2017)
    endif()

    foreach(item ${QT_DIR_ROOT})
        set(QT_DIR_PATH ${item}/${QT_DIR_NAME})
        message(STATUS "item is ${QT_DIR_PATH}")
        if(EXISTS ${QT_DIR_PATH})
            list(APPEND CMAKE_PREFIX_PATH ${QT_DIR_PATH})
        endif()
    endforeach()

else()

    message("current platform: unkonw ")

endif()


##为每个模块提供*.cmake文件搜索路径
#set(Qt5_DIR "${CMAKE_PREFIX_PATH}/lib/cmake/Qt5")
#set(Qt5Widgets_DIR "${CMAKE_PREFIX_PATH}/lib/cmake/Qt5Widgets")
#set(Qt5Network_DIR "${CMAKE_PREFIX_PATH}/lib/cmake/Qt5Network")
#set(Qt5LinguistTools_DIR "${CMAKE_PREFIX_PATH}/lib/cmake/Qt5LinguistTools")
#然后查找
#find_package(Qt5 COMPONENTS Widgets Network LinguistTools)
#find_package(QT NAMES Qt6 Qt5 COMPONENTS Widgets Network REQUIRED)
# find_package(Qt5 COMPONENTS Widgets Network WebSockets REQUIRED)


# find_package(<库名> <版本号> EXACT COMPONENTS <组件名1> ... <组件名n> REQUIRED)
find_package(Qt5 5.13.2 COMPONENTS Widgets Network Concurrent REQUIRED)
 
#CMAKE_ARCHIVE_OUTPUT_DIRECTORY：默认存放静态库的文件夹位置；
#CMAKE_LIBRARY_OUTPUT_DIRECTORY：默认存放动态库的文件夹位置；
#LIBRARY_OUTPUT_PATH：默认存放库文件的位置，如果产生的是静态库并且没有指定
#CMAKE_ARCHIVE_OUTPUT_DIRECTORY 则存放在该目录下，动态库也类似；
#CMAKE_RUNTIME_OUTPUT_DIRECTORY：存放可执行软件的目录；
#set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/archive)
#set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/library)
# set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/win64)
#set(LIBRARY_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/path)

add_definitions(-DUNICODE)


if(MSVC)
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SAFESEH:NO")
endif()


# set the include files
set(PRO_CODE_INC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/src/module
        ${CMAKE_CURRENT_SOURCE_DIR}/module/DragAbleWidget
        )
include_directories(${PRO_CODE_INC})


file(GLOB_RECURSE ALL_SOURCE
        ${CMAKE_CURRENT_SOURCE_DIR}/module/DragAbleWidget/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Widget/*.cpp
        )
  
# file(GLOB_RECURSE TMP_SOURCE
#     ${CMAKE_CURRENT_SOURCE_DIR}/module/VideoPlayer/src/*.c
#     ${CMAKE_CURRENT_SOURCE_DIR}/module/VideoPlayer/src/*.cpp)
# list (APPEND ALL_SOURCE ${TMP_SOURCE})

# file(GLOB_RECURSE TMP_SOURCE
#     ${CMAKE_CURRENT_SOURCE_DIR}/module/DragAbleWidget/*.c
#     ${CMAKE_CURRENT_SOURCE_DIR}/module/DragAbleWidget/*.cpp)
# list (APPEND ALL_SOURCE ${TMP_SOURCE})

# file(GLOB_RECURSE ALL_HEAD
#         src/*.hpp
#         src/*.h)

qt5_add_resources(qrc_FILES ${CMAKE_CURRENT_SOURCE_DIR}/resources/resources.qrc)
# message("ALL_SOURCE:"${ALL_SOURCE})
add_executable(${PROJECT_NAME} ${ALL_SOURCE} ${qrc_FILES})
# add_executable(${PROJECT_NAME} ${ALL_SOURCE})

#### add videoplayer module ###
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/module/VideoPlayer)

include_directories(${PRO_CODE_INC})

# target_link_libraries(${PROJECT_NAME}
#         winmm.lib
#         Qt5::Core Qt5::Widgets Qt5::Network Qt5::WebSockets Qt5::Concurrent
#         core gui network sql concurrent xml axcontainer)

target_link_libraries(${PROJECT_NAME}
        WS2_32.lib AdvAPI32.lib winmm.lib User32.lib GDI32.lib Strmiids.lib
        Qt5::Core Qt5::Widgets Qt5::Network  Qt5::Concurrent)
#target_link_libraries(${PROJECT_NAME}
#        -lavformat
#        -lavcodec
#        -lavutil
#        -lswscale
#        -lavfilter
#        -lswresample
#        -lnvmpi
#        -lpthread -llzma -lssl -lcrypt -lz
#        Qt5::Core Qt5::Widgets Qt5::Network Qt5::WebSockets)

# message(${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

# #编译成功后，执行自定义命令
# #拷贝有可能会升级的lib文件到system/lib
# add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
#     # COMMAND cp ${CMAKE_CURRENT_SOURCE_DIR}/module/VideoPlayer/lib/ffmpeg/bin/win64/* ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/
#     # COMMAND ${CMAKE_COMMAND} -E copy module/VideoPlayer/lib/ffmpeg/bin/win64/avcodec.lib bin/win64
#     COMMAND copy module/VideoPlayer/lib/ffmpeg/bin/win64/* bin/win64/
#     )


# #编译成功后，执行自定义命令
# #拷贝dll到bin目录下
# add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
#     COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/copy_dll.bat
#     )

