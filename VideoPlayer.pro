#-------------------------------------------------
#
# Project created by QtCreator 2016-09-01T16:10:47
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = VideoPlayer
TEMPLATE = app

SOURCES += src/main.cpp \
    src/videoplayer/videoplayer_thread.cpp \
    src/videoplayer/videoplayer.cpp \
    src/videoplayer/videoplayer_showvideowidget.cpp \
    src/videoplayer/widget/VideoSlider.cpp

HEADERS  += \
    src/videoplayer/videoplayer_thread.h \
    src/videoplayer/videoplayer.h \
    src/videoplayer/videoplayer_showvideowidget.h \
    src/videoplayer/widget/VideoSlider.h

FORMS    += \
    src/videoplayer/videoplayer.ui \
    src/videoplayer/videoplayer_showvideowidget.ui


INCLUDEPATH += $$PWD/ffmpeg/include \
               $$PWD/SDL2/include \
               $$PWD/src

#LIBS += $$PWD/ffmpeg/lib/avcodec.lib \
#        $$PWD/ffmpeg/lib/avdevice.lib \
#        $$PWD/ffmpeg/lib/avfilter.lib \
#        $$PWD/ffmpeg/lib/avformat.lib \
#        $$PWD/ffmpeg/lib/avutil.lib \
#        $$PWD/ffmpeg/lib/postproc.lib \
#        $$PWD/ffmpeg/lib/swresample.lib \
#        $$PWD/ffmpeg/lib/swscale.lib \
#        $$PWD/SDL2/lib/x86/SDL2.lib

LIBS += -L$$PWD/ffmpeg/lib -lavcodec -lavdevice -lavfilter -lavformat -lavutil -lpostproc -lswresample -lswscale
LIBS += -L$$PWD/SDL2/lib/x86 -lSDL2


RESOURCES += \
    resources.qrc

win32:RC_FILE=$$PWD/main.rc
