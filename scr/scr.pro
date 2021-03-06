#-------------------------------------------------
#
# Project created by QtCreator 2018-01-29T20:45:15
#
#-------------------------------------------------

QT       += core gui
QT       += sql

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = scr
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


SOURCES += main.cpp\
        mainwindow.cpp \
    environmentdialog.cpp \
    logindialog.cpp \
    securefiledialog.cpp \
    addentry.cpp \
    deleteconfirmation.cpp \
    zxcvbn.cpp \
    mysortfilterproxymodel.cpp \
    helpwindow.cpp \
    newtable.cpp

HEADERS  += mainwindow.h \
    environmentdialog.h \
    logindialog.h \
    securefiledialog.h \
    addentry.h \
    deleteconfirmation.h \
    dict-src.h \
    zxcvbn.h \
    mysortfilterproxymodel.h \
    helpwindow.h \
    newtable.h

FORMS    += mainwindow.ui \
    addentry.ui \
    deleteconfirmation.ui \
    helpwindow.ui \
    newtable.ui

win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../SEfile/release/ -lSEfile
else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../SEfile/debug/ -lSEfile
else:unix: LIBS += -L$$OUT_PWD/../SEfile/ -lSEfile

INCLUDEPATH += $$PWD/../SEfile
DEPENDPATH += $$PWD/../SEfile

win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../SEfile/release/libSEfile.a
else:win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../SEfile/debug/libSEfile.a
else:win32:!win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../SEfile/release/SEfile.lib
else:win32:!win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../SEfile/debug/SEfile.lib
else:unix: PRE_TARGETDEPS += $$OUT_PWD/../SEfile/libSEfile.a
