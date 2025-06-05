INCLUDEPATH += $$PWD

FFMPEG_BRANCH_NAME="V4.3.1"
SDL2_BRANCH_NAME="V2.0.2"

win32{
    message("current platform: Windows ")
    FFMPEG_BRANCH_NAME="V4.3.1-windows"
    SDL2_BRANCH_NAME="V2.0.2-windows"
}

unix{
    message("current platform: Linux ")
    FFMPEG_BRANCH_NAME="V4.3.1-linux"
    SDL2_BRANCH_NAME="V2.0.12-linux"
}
    

#自动下载ffmpeg库文件
FILE_TO_CHECK=$$PWD/ffmpeg
!exists($$FILE_TO_CHECK) {
    message("File $$FILE_TO_CHECK does not exist. Cloning from Git repository...")
    system(git clone https://gitee.com/devlib/ffmpeg-dev.git ffmpeg -b $$FFMPEG_BRANCH_NAME)
} else {
    message("File $$FILE_TO_CHECK exists.")
}


#自动下载SDL库文件
FILE_TO_CHECK=$$PWD/SDL2
!exists($$FILE_TO_CHECK) {
    message("File $$FILE_TO_CHECK does not exist. Cloning from Git repository...")
    system(git clone https://gitee.com/devlib/SDL2-dev.git SDL2 -b $$SDL2_BRANCH_NAME)
} else {
    message("File $$FILE_TO_CHECK exists.")
}


include($$PWD/ffmpeg/ffmpeg.pri)
include($$PWD/SDL2/SDL2.pri)
