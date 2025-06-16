CONFIG += c++11
QMAKE_CXXFLAGS += -std=c++11

INCLUDEPATH += $$PWD/src \
               $$PWD/src/frame

SOURCES +=  \
    $$PWD/src/VideoPlayer/VideoPlayer.cpp \
    $$PWD/src/frame/AudioFrame/AACFrame.cpp \
    $$PWD/src/frame/VideoFrame/VideoEncodedFrame.cpp \
    $$PWD/src/frame/VideoFrame/VideoFrame.cpp \
    $$PWD/src/frame/AudioFrame/PCMFrame.cpp \
    $$PWD/src/VideoPlayer/VideoPlayer_VideoThread.cpp \
    $$PWD/src/VideoPlayer/VideoPlayer_AudioThread.cpp \
    $$PWD/src/PcmPlayer/PcmVolumeControl.cpp \
    $$PWD/src/PcmPlayer/PcmPlayer_SDL.cpp \
    $$PWD/src/PcmPlayer/PcmPlayer.cpp \
    $$PWD/src/frame/VideoFrame/VideoRawFrame.cpp \
    $$PWD/src/frame/nalu/nalu.cpp \
    $$PWD/src/util/thread.cpp \
    $$PWD/src/util/util.cpp

HEADERS  += \
    $$PWD/src/PcmPlayer/PcmPlayer.h \
    $$PWD/src/VideoPlayer/VideoPlayer.h \
    $$PWD/src/frame/AudioFrame/AACFrame.h \
    $$PWD/src/frame/VideoFrame/VideoEncodedFrame.h \
    $$PWD/src/frame/VideoFrame/VideoFrame.h \
    $$PWD/src/frame/AudioFrame/PCMFrame.h \
    $$PWD/src/PcmPlayer/PcmVolumeControl.h \
    $$PWD/src/PcmPlayer/PcmPlayer_SDL.h \
    $$PWD/src/PcmPlayer/PcmPlayer.cpp \
    $$PWD/src/frame/VideoFrame/VideoRawFrame.h \
    $$PWD/src/frame/nalu/h264.h \
    $$PWD/src/frame/nalu/h265.h \
    $$PWD/src/frame/nalu/nalu.h \
    $$PWD/src/util/thread.h \
    $$PWD/src/util/util.h

### lib ### Begin
    include($$PWD/lib/lib.pri)
### lib ### End

DEFINES += USE_PCM_PLAYER ENABLE_SDL
