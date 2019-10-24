CONFIG += c++11
QMAKE_CXXFLAGS += -std=c++11

SOURCES +=  \
    $$PWD/src/Mutex/Cond.cpp \
    $$PWD/src/Mutex/Mutex.cpp \
    $$PWD/src/LogWriter/LogWriter.cpp \
    $$PWD/src/VideoPlayer/VideoPlayer.cpp \
    $$PWD/src/VideoPlayer/Video/VideoFrame.cpp \
    $$PWD/src/VideoPlayer/Video/VideoPlayer_VideoThread.cpp \
    $$PWD/src/VideoPlayer/Audio/VideoPlayer_AudioThread.cpp \
    $$PWD/src/VideoPlayer/Audio/PcmVolumeControl.cpp \
    $$PWD/src/EventHandle/VideoPlayerEventHandle.cpp \
    $$PWD/src/types.cpp

HEADERS  += \
    $$PWD/src/Mutex/Cond.h \
    $$PWD/src/Mutex/Mutex.h \
    $$PWD/src/LogWriter/LogWriter.h \
    $$PWD/src/VideoPlayer/VideoPlayer.h \
    $$PWD/src/VideoPlayer/Video/VideoFrame.h \
    $$PWD/src/VideoPlayer/Audio/PcmVolumeControl.h \
    $$PWD/src/EventHandle/VideoPlayerEventHandle.h \
    $$PWD/src/types.h


win32{

    contains(QT_ARCH, i386) {
        message("32-bit")
        INCLUDEPATH += $$PWD/lib/win32/ffmpeg/include \
                       $$PWD/lib/win32/SDL2/include \
                       $$PWD/src

        LIBS += -L$$PWD/lib/win32/ffmpeg/lib -lavcodec -lavdevice -lavfilter -lavformat -lavutil -lpostproc -lswresample -lswscale
        LIBS += -L$$PWD/lib/win32/SDL2/lib -lSDL2
    } else {
        message("64-bit")
        INCLUDEPATH += $$PWD/lib/win64/ffmpeg/include \
                       $$PWD/lib/win64/SDL2/include \
                       $$PWD/src

        LIBS += -L$$PWD/lib/win64/ffmpeg/lib -lavcodec -lavdevice -lavfilter -lavformat -lavutil -lpostproc -lswresample -lswscale
        LIBS += -L$$PWD/lib/win64/SDL2/lib -lSDL2
    }

}

unix{
    contains(QT_ARCH, i386) {
        message("32-bit, 请自行编译32位库!")
    } else {
        message("64-bit")
        INCLUDEPATH += $$PWD/lib/linux/ffmpeg/include \
                       $$PWD/lib/linux/SDL2/include/SDL2 \
                       $$PWD/src

        LIBS += -L$$PWD/lib/linux/ffmpeg/lib  -lavformat  -lavcodec -lavdevice -lavfilter -lavutil -lswresample -lswscale
        LIBS += -L$$PWD/lib/linux/SDL2/lib -lSDL2

        LIBS += -lpthread -ldl
    }

#QMAKE_POST_LINK 表示编译后执行内容
#QMAKE_PRE_LINK 表示编译前执行内容

#解压库文件
#QMAKE_PRE_LINK += "cd $$PWD/lib/linux && tar xvzf ffmpeg.tar.gz "
system("cd $$PWD/lib/linux && tar xvzf ffmpeg.tar.gz")
system("cd $$PWD/lib/linux && tar xvzf SDL2.tar.gz")


}


