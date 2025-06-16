cmake_minimum_required(VERSION 3.13)

# set(PRODUCT_NAME VideoPlayer)

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
# set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/win64)
#set(LIBRARY_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/path)

if(UNIX)

    message("current platform: Linux ")

    set(CMAKE_PREFIX_PATH "/opt/Qt/Qt5.13.2")
    set(QT_QMAKE_EXECUTABLE "/opt/Qt/Qt5.13.2/bin/qmake")

    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin/linux)

elseif(WIN32)

    message("current platform: Windows ")

    # #指定Qt安装目录
    # set(QT_DIR_ROOT 
    #         C:/Qt/Qt5.13.2/5.13.2
    #         D:/Qt/Qt5.13.2/5.13.2)
    
    # if(CMAKE_CL_64)    #CMAKE的内建变量，如果是true，就说明编译器的64位的，自然可以编译64bit的程序
    #     set(QT_DIR_NAME msvc2017_64)
    #     set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin/win64)
    #     # set(CMAKE_PREFIX_PATH ${QT_DIR_ROOT}/msvc2017_64)
    #     # list(APPEND CMAKE_PREFIX_PATH ${QT_DIR_ROOT}/msvc2017_64)
    # else()
    #     set(QT_DIR_NAME msvc2017)
    #     set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin/win32)
    # endif()

    # foreach(item ${QT_DIR_ROOT})
    #     set(QT_DIR_PATH ${item}/${QT_DIR_NAME})
    #     message(STATUS "item is ${QT_DIR_PATH}")
    #     if(EXISTS ${QT_DIR_PATH})
    #         list(APPEND CMAKE_PREFIX_PATH ${QT_DIR_PATH})
    #     endif()
    # endforeach()

    if(CMAKE_CL_64)    #CMAKE的内建变量，如果是true，就说明编译器的64位的，自然可以编译64bit的程序
        list(APPEND CMAKE_PREFIX_PATH C:/Qt/Qt5.13.2/5.13.2/msvc2017_64)
        list(APPEND CMAKE_PREFIX_PATH C:/Qt/6.7.3/msvc2019_64)
        set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin/win64)
    else()
        set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin/win32)
        list(APPEND CMAKE_PREFIX_PATH C:/Qt/Qt5.13.2/5.13.2/msvc2017)
    endif()

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
find_package(Qt5 COMPONENTS Core Gui OpenGL OpenGLWidgets Widgets Network Concurrent)
find_package(Qt6 COMPONENTS Core Gui OpenGL OpenGLWidgets Widgets Network Concurrent Core5Compat)
# find_package(Qt6 REQUIRED COMPONENTS Core Gui OpenGL OpenGLWidgets Widgets Network Concurrent )

message("Qt5_VERSION:"${Qt5_VERSION})
message("Qt6_VERSION:"${Qt6_VERSION})

set(FOUND_QT5 FALSE)
set(FOUND_QT6 FALSE)

### 优先使用Qt6 ###
if(DEFINED Qt6_VERSION AND NOT ${Qt6_VERSION} STREQUAL "")
    message(STATUS "found Qt6")
    set(FOUND_QT6 TRUE)
else()
    message(STATUS "not found Qt6")
endif()

if(DEFINED Qt5_VERSION AND NOT ${Qt5_VERSION} STREQUAL "")
    message(STATUS "found Qt5")
    set(FOUND_QT5 TRUE)
else()
    message(STATUS "not found Qt5")
endif()

# if(Qt5_VERSION VERSION_GREATER_EQUAL "5.12.0")
#     message(STATUS "Qt version is 5.12.0 or higher")
# else()
#     message(FATAL_ERROR "Qt version is too low, need at least 5.12.0")
# endif()

if (NOT FOUND_QT5 AND NOT FOUND_QT6)
    message(FATAL_ERROR "\n!!!!! Qt5 or Qt6 not found!!!!!\n")
endif()

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
  
if (FOUND_QT6)
    qt6_add_resources(qrc_FILES ${CMAKE_CURRENT_SOURCE_DIR}/resources/resources.qrc)
elseif (FOUND_QT5)
    qt5_add_resources(qrc_FILES ${CMAKE_CURRENT_SOURCE_DIR}/resources/resources.qrc)
endif()

# message("ALL_SOURCE:"${ALL_SOURCE})
add_executable(${PROJECT_NAME} ${ALL_SOURCE} ${qrc_FILES})
# add_executable(${PROJECT_NAME} ${ALL_SOURCE})

#### add videoplayer module ###
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/module/VideoPlayer)

include_directories(${PRO_CODE_INC})

if (WIN32)
    target_link_libraries(${PROJECT_NAME}
        WS2_32.lib AdvAPI32.lib winmm.lib User32.lib GDI32.lib Strmiids.lib)
endif()

if (FOUND_QT6)
    target_link_libraries(${PROJECT_NAME} Qt6::Core Qt6::Widgets Qt6::Network Qt6::Concurrent Qt6::OpenGL Qt6::OpenGLWidgets Qt6::Core5Compat)
elseif (FOUND_QT5)
    target_link_libraries(${PROJECT_NAME} Qt5::Core Qt5::Widgets Qt5::Network Qt5::Concurrent)
endif()


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

