CONFIG += c++11
QMAKE_CXXFLAGS += -std=c++11

INCLUDEPATH += $$PWD/src

SOURCES +=  \
    $$PWD/src/VideoPlayer/VideoPlayer.cpp \
    $$PWD/src/frame/VideoFrame/VideoFrame.cpp \
    $$PWD/src/frame/AudioFrame/PCMFrame.cpp \
    $$PWD/src/VideoPlayer/VideoPlayer_VideoThread.cpp \
    $$PWD/src/VideoPlayer/VideoPlayer_AudioThread.cpp \
    $$PWD/src/PcmPlayer/PcmVolumeControl.cpp \
    $$PWD/src/PcmPlayer/PcmPlayer_SDL.cpp \
    $$PWD/src/PcmPlayer/PcmPlayer.cpp \
    $$PWD/src/util/thread.cpp \
    $$PWD/src/types.cpp

HEADERS  += \
    $$PWD/src/VideoPlayer/VideoPlayer.h \
    $$PWD/src/frame/VideoFrame/VideoFrame.h \
    $$PWD/src/frame/AudioFrame/PCMFrame.h \
    $$PWD/src/PcmPlayer/PcmVolumeControl.h \
    $$PWD/src/PcmPlayer/PcmPlayer_SDL.h \
    $$PWD/src/PcmPlayer/PcmPlayer.cpp \
    $$PWD/src/util/thread.h \
    $$PWD/src/types.h

### lib ### Begin
    include($$PWD/lib/lib.pri)
### lib ### End

