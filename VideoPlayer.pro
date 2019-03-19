#-------------------------------------------------
#
# Project created by QtCreator 2016-09-01T16:10:47
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets


UI_DIR  = obj/Gui
MOC_DIR = obj/Moc
OBJECTS_DIR = obj/Obj


#将输出文件直接放到源码目录下的bin目录下，将dll都放在了次目录中，用以解决运行后找不到dll的问
#DESTDIR=$$OUT_PWD/bin/
DESTDIR=$$PWD/bin/
QMAKE_CXXFLAGS += -std=c++11

TARGET = VideoPlayer
TEMPLATE = app

#包含视频播放器的代码
include(module/VideoPlayer/VideoPlayer.pri)
#包含可拖动窗体的代码
include(module/DragAbleWidget/DragAbleWidget.pri)

SOURCES += src/main.cpp \
    src/Widget/VideoPlayerWidget.cpp \
    src/Widget/ShowVideoWidget.cpp \
    src/Widget/VideoSlider.cpp


HEADERS  += \
    src/Widget/VideoPlayerWidget.h \
    src/Widget/ShowVideoWidget.h \
    src/Widget/VideoSlider.h

FORMS    += \
    src/Widget/VideoPlayerWidget.ui \
    src/Widget/ShowVideoWidget.ui

RESOURCES += \
    resources/resources.qrc

INCLUDEPATH += $$PWD/src

win32:RC_FILE=$$PWD/resources/main.rc
