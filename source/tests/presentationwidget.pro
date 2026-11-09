QT += widgets concurrent testlib
CONFIG += testcase console c++11
CONFIG -= app_bundle
TEMPLATE = app
TARGET = presentationwidget_test
INCLUDEPATH += ..
SOURCES += presentationwidget_test.cpp ../slidezoom.cpp ../presentationwidget.cpp ../presentation.cpp ../pdfrenderer.cpp
HEADERS += ../slidezoom.hpp ../presentationwidget.hpp ../presentation.hpp ../pdfrenderer.hpp
