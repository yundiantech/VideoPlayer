INCLUDEPATH += $$PWD

win32{

    INCLUDEPATH += $$PWD/win/ffmpeg/include \
                   $$PWD/win/SDL2/include

    contains(QT_ARCH, i386) {
        message("32-bit")

        LIBS += -L$$PWD/win/ffmpeg/lib/x86 -lavcodec -lavdevice -lavfilter -lavformat -lavutil -lpostproc -lswresample -lswscale
        LIBS += -L$$PWD/win/SDL2/lib/x86 -lSDL2
    } else {
        message("64-bit")

        LIBS += -L$$PWD/win/ffmpeg/lib/x64 -lavcodec -lavdevice -lavfilter -lavformat -lavutil -lpostproc -lswresample -lswscale
        LIBS += -L$$PWD/win/SDL2/lib/x64 -lSDL2
    }

}

unix{

    INCLUDEPATH += $$PWD/linux/ffmpeg/include \
                   $$PWD/linux/SDL2/include/SDL2

    contains(QT_ARCH, i386) {
        message("32-bit, 请自行编译32位库!")
    } else {
        message("64-bit")

        LIBS += -L$$PWD/linux/ffmpeg/lib  -lavformat  -lavcodec -lavdevice -lavfilter -lavutil -lswresample -lswscale
        LIBS += -L$$PWD/linux/SDL2/lib -lSDL2

        LIBS += -lpthread -ldl
    }

#QMAKE_POST_LINK 表示编译后执行内容
#QMAKE_PRE_LINK 表示编译前执行内容

#解压库文件
#QMAKE_PRE_LINK += "cd $$PWD/lib/linux && tar xvzf ffmpeg.tar.gz "
system("cd $$PWD/lib/linux && tar xvzf ffmpeg.tar.gz")
system("cd $$PWD/lib/linux && tar xvzf SDL2.tar.gz")


}

