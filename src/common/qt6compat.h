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

// Version detection: Quill.pro injects QT5/QT6 at qmake time. This header is
// force-included into EVERY translation unit (-include / /FI), including the
// 3rdparty C++ sources whose qmake INCPATH does not necessarily contain the Qt
// include dirs. It therefore must NEVER #include a Qt header here -- even
// <QtGlobal> is not reliably resolvable on every Qt5 translation unit
// (Qt5 qmake INCPATH only lists module dirs like -I.../include/QtCore and has
// no Qt root include dir, and the QtGlobal forwarding header is not shipped by
// every Qt5 packaging, unlike Qt6 which always ships it).
#if defined(QT6)
#  define QUIL_QT6 1
#elif defined(QT5) || defined(HAVE_QT5)
#  define QUIL_QT6 0
#else
#  if defined(_MSC_VER)
#    pragma message("qt6compat.h: neither QT5 nor QT6 was injected by qmake; assuming Qt5 semantics")
#  else
#    warning "qt6compat.h: neither QT5 nor QT6 was injected by qmake; assuming Qt5 semantics"
#  endif
#  define QUIL_QT6 0
#endif

// Qt 6 still ships `foreach`/Q_FOREACH (in qforeach.h, via QtGlobal) but
// marks it deprecated, and for non-implicitly-shared containers (QJsonArray,
// QJsonObject, ...) it emits -Wdeprecated-declarations warnings which break
// -Werror builds. The legacy code base (ported from Qt4) relies on foreach
// in many places, so override Qt's own macro on Qt 6 with a plain range-for
// expansion (semantically identical to Qt5's foreach except it does NOT copy
// the container, so containers must not be mutated while being iterated).
#if QUIL_QT6
#  ifdef foreach
#    undef foreach
#  endif
#  define foreach(variable, container) for (variable : container)
#endif

// Qt 6 moved QRegExp / QTextCodec out of QtCore into the Qt5Compat module.
// Many translation units rely on them without an explicit include (Qt5's
// <QtCore> umbrella header used to pull them in). Re-import them globally
// on Qt 6 so core5compat only has to be linked (already done in .pro).
#if QUIL_QT6
#include <QRegExp>
#include <QTextCodec>
#endif

// QMouseEvent/QHoverEvent::pos() was removed in Qt6 in favour of position().
// This header is force-included for every translation unit (see Quill.pro),
// so a small macro is enough to keep the many event->pos() call sites working
// on both toolkits.
#if QUIL_QT6
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
#if QUIL_QT6
#  define QMODELINDEX_CHILD(idx, row, col) ((idx).model()->index((row), (col), (idx)))
#else
#  define QMODELINDEX_CHILD(idx, row, col) ((idx).child((row), (col)))
#endif

// QWebEnginePage::createStandardContextMenu() was removed in Qt 6.2; the same
// API now lives on QWebEngineView::createStandardContextMenu().
#if QUIL_QT6
#  define QWEBENGINE_STD_CONTEXTMENU(view) ((view)->createStandardContextMenu())
#else
#  define QWEBENGINE_STD_CONTEXTMENU(view) ((view)->page()->createStandardContextMenu())
#endif

// ============================================================
// Multimedia (QMediaPlayer / QAudioOutput) - Qt5 vs Qt6
// Qt5: QMediaPlayer::setMedia(), state(), setVolume(0-100), QMediaPlaylist
// Qt6: QMediaPlayer::setSource(), setAudioOutput(), playbackState(), QAudioOutput
// NOTE: translation units using these macros must #include <QMediaPlayer>
// (and <QAudioOutput> on Qt6) themselves -- mainwindow.h / playerbar.h do.
// This header must NEVER #include a Qt header (see note at the top).
// ============================================================
#if QUIL_QT6
#  define QMEDIAPLAYER_SET_SOURCE(player, url) ((player)->setSource(url))
#  define QMEDIAPLAYER_SET_AUDIO_OUTPUT(player, output) ((player)->setAudioOutput(output))
#  define QMEDIAPLAYER_PLAYBACK_STATE(player) ((player)->playbackState())
#  define QMEDIAPLAYER_SET_VOLUME(player, vol) do { \
    if ((player)->audioOutput()) (player)->audioOutput()->setVolume(vol); \
  } while(0)
#  define QMEDIAPLAYER_POSITION(player) ((player)->position())
#  define QMEDIAPLAYER_DURATION(player) ((player)->duration())
#else
// Qt5 multimedia
#  define QMEDIAPLAYER_SET_SOURCE(player, url) ((player)->setMedia(url))
#  define QMEDIAPLAYER_SET_AUDIO_OUTPUT(player, output) ((void)0)
#  define QMEDIAPLAYER_PLAYBACK_STATE(player) ((player)->state())
#  define QMEDIAPLAYER_SET_VOLUME(player, vol) ((player)->setVolume(vol))
#  define QMEDIAPLAYER_POSITION(player) ((player)->position())
#  define QMEDIAPLAYER_DURATION(player) ((player)->duration())
#endif

// QMediaPlayer::PlayingState enum value is the same in Qt5/Qt6
// QAudioOutput exists only in Qt6 - use pointer with #if QUIL_QT6

#endif // QT6COMPAT_H
