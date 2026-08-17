# ============================================================
# Quill - unit tests (QTest)
#
# Standalone test target: only depends on QtCore/QtTest and the
# pure-logic modules (HtmlSanitizer, FtsSearch) that have no
# application singletons behind them. Run it in CI after the
# main build to keep the build+test loop closed.
# ============================================================
QT += core testlib
QT -= gui

TARGET = tst_common
CONFIG += console
CONFIG -= app_bundle
TEMPLATE = app

INCLUDEPATH += $$PWD/../src/common \
               $$PWD/../3rdparty/qupzilla

SOURCES += tst_common.cpp \
           ../src/common/htmlsanitizer.cpp \
           ../src/common/ftssearch.cpp \
           ../3rdparty/qupzilla/qzregexp.cpp

HEADERS += ../src/common/htmlsanitizer.h \
           ../src/common/ftssearch.h
