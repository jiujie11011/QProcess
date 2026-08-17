/* ============================================================
* Quill is a open-source cross-platform RSS/Atom news feeds reader
* Copyright (C) 2011-2020 Quill Team <quillteam@gmail.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <https://www.gnu.org/licenses/>.
* ============================================================ */
#ifndef QT6COMPAT_H
#define QT6COMPAT_H

// NOTE: must be <QtGlobal>, NOT <QtCore/qglobal.h>. Qt5's qmake INCPATH only
// lists module include dirs (-I.../include/QtCore, .../include/QtWidgets, ...)
// and has no Qt root include dir, so the module-prefixed form fails to resolve
// on Qt5 while the plain <QtGlobal> resolves from the QtCore module dir on both
// toolkits.
#include <QtGlobal>

// Qt 6 still ships `foreach`/Q_FOREACH (in qforeach.h, via QtGlobal) but
// marks it deprecated, and for non-implicitly-shared containers (QJsonArray,
// QJsonObject, ...) it emits -Wdeprecated-declarations warnings which break
// -Werror builds. The legacy code base (ported from Qt4) relies on foreach
// in many places, so override Qt's own macro on Qt 6 with a plain range-for
// expansion (semantically identical to Qt5's foreach except it does NOT copy
// the container, so containers must not be mutated while being iterated).
// The header is force-included by Quill.pro (qmake -include / /FI), which is
// why this file lives apart from common.h.
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#  ifdef foreach
#    undef foreach
#  endif
#  define foreach(variable, container) for (variable : container)
#endif

// Qt 6 moved QRegExp / QTextCodec out of QtCore into the Qt5Compat module.
// Many translation units rely on them without an explicit include (Qt5's
// <QtCore> umbrella header used to pull them in). Re-import them globally
// on Qt 6 so core5compat only has to be linked (already done in .pro).
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QRegExp>
#include <QTextCodec>
#endif

// QMouseEvent/QHoverEvent::pos() was removed in Qt6 in favour of position().
// This header is force-included for every translation unit (see Quill.pro),
// so a small macro is enough to keep the many event->pos() call sites working
// on both toolkits.
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#  define QEVENT_POS(e) ((e)->position().toPoint())
#  define QEVENT_POSF(e) ((e)->position())
#  define QEVENT_GLOBALPOS(e) ((e)->globalPosition().toPoint())
#  define QHOVEREVENT_NEW(type, pos, oldPos) \
     new QHoverEvent((type), (pos), (pos), (oldPos))
#else
#  define QEVENT_POS(e) ((e)->pos())
#  define QEVENT_POSF(e) ((e)->posF())
#  define QEVENT_GLOBALPOS(e) ((e)->globalPos())
#  define QHOVEREVENT_NEW(type, pos, oldPos) \
     new QHoverEvent((type), (pos), (oldPos))
#endif

// QModelIndex::child() was removed in Qt 6 -> model()->index(row, col, parent).
// Semantics are identical (Qt5's child() is just a thin wrapper over it).
// Callers must guarantee a valid model (they always do: they already called
// model()->rowCount() on the same index).
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#  define QMODELINDEX_CHILD(idx, row, col) ((idx).model()->index((row), (col), (idx)))
#else
#  define QMODELINDEX_CHILD(idx, row, col) ((idx).child((row), (col)))
#endif

// QWebEnginePage::createStandardContextMenu() was removed in Qt 6.2; the same
// API now lives on QWebEngineView::createStandardContextMenu().
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#  define QWEBENGINE_STD_CONTEXTMENU(view) ((view)->createStandardContextMenu())
#else
#  define QWEBENGINE_STD_CONTEXTMENU(view) ((view)->page()->createStandardContextMenu())
#endif

#endif // QT6COMPAT_H
