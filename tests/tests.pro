# ============================================================
# Quill - unit tests (QTest)
#
# Single target built around tests/main.cpp, which runs three
# suites sequentially (no CI step changes needed):
#   1. TestCommon           - HtmlSanitizer / FtsSearch (pure logic)
#   2. TestThemeManager     - theme tokens / persistence (needs QApplication)
#   3. TestNewsCardDelegate - card data serialization / sizeHint
#
# Requires QtWidgets because ThemeManager uses QApplication and
# NewsCardDelegate is a QStyledItemDelegate. Linux headless CI is
# handled inside main.cpp via the offscreen platform plugin.
# ============================================================
QT += core gui widgets testlib

TARGET = tst_common
CONFIG += console
CONFIG -= app_bundle
TEMPLATE = app

CONFIG += c++17

INCLUDEPATH += $$PWD/../src/common \
               $$PWD/../3rdparty/qupzilla \
               $$PWD/../src/theme \
               $$PWD/../src/newsview

SOURCES += main.cpp \
           tst_common.cpp \
           tst_thememanager.cpp \
           tst_newscarddelegate.cpp \
           ../src/common/htmlsanitizer.cpp \
           ../src/common/ftssearch.cpp \
           ../3rdparty/qupzilla/qzregexp.cpp \
           ../src/theme/thememanager.cpp \
           ../src/theme/tokens.cpp \
           ../src/newsview/newscarddelegate.cpp

HEADERS += tst_common.h \
           tst_thememanager.h \
           tst_newscarddelegate.h \
           ../src/common/htmlsanitizer.h \
           ../src/common/ftssearch.h \
           ../src/theme/thememanager.h \
           ../src/theme/tokens.h \
           ../src/newsview/newscarddelegate.h
