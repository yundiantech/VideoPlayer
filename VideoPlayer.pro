#-------------------------------------------------
#
# Project created by QtCreator 2016-09-01T16:10:47
#
#-------------------------------------------------

QT       += core gui opengl openglwidgets

greaterThan(QT_MAJOR_VERSION, 5): QT += core5compat
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

UI_DIR  = obj/Gui
MOC_DIR = obj/Moc
OBJECTS_DIR = obj/Obj


#将输出文件直接放到源码目录下的bin目录下，将dll都放在了此目录中，用以解决运行后找不到dll的问
#DESTDIR=$$PWD/bin/
contains(QT_ARCH, i386) {
    message("32-bit")
    DESTDIR = $${PWD}/bin/win32
} else {
    message("64-bit")
    DESTDIR = $${PWD}/bin/win64
}
QMAKE_CXXFLAGS += -std=c++11

TARGET = VideoPlayer
TEMPLATE = app

#引入VideoPlayer模块代码
include(module/VideoPlayer/VideoPlayer.pri)
#引入DragAbleWidget模块代码
include(module/DragAbleWidget/DragAbleWidget.pri)

SOURCES += \
    src/Widget/SetVideoUrlDialog.cpp \
    src/Widget/mymessagebox_withTitle.cpp \
    src/main.cpp \
    src/AppConfig.cpp \
    src/MainWindow.cpp \
    src/Widget/ShowVideoWidget.cpp \
    src/Widget/VideoSlider.cpp

HEADERS  += \
    src/AppConfig.h \
    src/MainWindow.h \
    src/Widget/SetVideoUrlDialog.h \
    src/Widget/ShowVideoWidget.h \
    src/Widget/VideoSlider.h \
    src/Widget/mymessagebox_withTitle.h


FORMS    += \
    src/MainWindow.ui \
    src/Widget/SetVideoUrlDialog.ui \
    src/Widget/ShowVideoWidget.ui \
    src/Widget/mymessagebox_withTitle.ui

RESOURCES += \
    resources/resources.qrc

INCLUDEPATH += $$PWD/src

win32:RC_FILE=$$PWD/resources/main.rc
