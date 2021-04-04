CONFIG += c++11
QMAKE_CXXFLAGS += -std=c++11

INCLUDEPATH += $$PWD/src

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

### lib ### Begin
    include($$PWD/lib/lib.pri)
### lib ### End

