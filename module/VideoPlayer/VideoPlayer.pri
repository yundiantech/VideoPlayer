QMAKE_CXXFLAGS += -std=c++11

SOURCES +=  \
    $$PWD/src/AppConfig.cpp \
    $$PWD/src/Mutex/Cond.cpp \
    $$PWD/src/Mutex/Mutex.cpp \
    $$PWD/src/LogWriter/LogWriter.cpp \
    $$PWD/src/VideoPlayer/VideoPlayer.cpp \
    $$PWD/src/VideoPlayer/Video/VideoPlayer_VideoThread.cpp \
    $$PWD/src/VideoPlayer/Audio/VideoPlayer_AudioThread.cpp \
    $$PWD/src/VideoPlayer/Audio/PcmVolumeControl.cpp \
    $$PWD/src/EventHandle/VideoPlayerEventHandle.cpp

HEADERS  += \
    $$PWD/src/AppConfig.h \
    $$PWD/src/Mutex/Cond.h \
    $$PWD/src/Mutex/Mutex.h \
    $$PWD/src/LogWriter/LogWriter.h \
    $$PWD/src/VideoPlayer/VideoPlayer.h \
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


