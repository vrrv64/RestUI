QT += core gui network xml
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
TARGET = RestUI
TEMPLATE = app
SOURCES += main.cpp mainwindow.cpp customtreewidget.cpp responsehighlighter.cpp
HEADERS += mainwindow.h customtreewidget.h responsehighlighter.h
#QMAKE_CXXFLAGS += -g
#QMAKE_CXXFLAGS += -O0

# Manually specify KSyntaxHighlighting
INCLUDEPATH += /usr/include/KF5/KSyntaxHighlighting
LIBS += -L/usr/lib/x86_64-linux-gnu -lKF5SyntaxHighlighting
