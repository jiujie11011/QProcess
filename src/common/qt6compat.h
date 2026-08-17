/* ============================================================
* QuiteRSS is a open-source cross-platform RSS/Atom news feeds reader
* Copyright (C) 2011-2020 QuiteRSS Team <quiterssteam@gmail.com>
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

#include <QtCore/qglobal.h>

// Qt 6 removed the `foreach` / Q_FOREACH macro from QtGlobal. The legacy
// code base (ported from Qt4) relies on it in many places, so restore the
// two-argument form for Qt 6 builds only. The header is force-included by
// QuiteRSS.pro (qmake -include / /FI), which is why this file lives apart
// from common.h. NOTE: this range-for based form does NOT copy the
// container (Qt 5's foreach did), so containers must not be mutated while
// being iterated.
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0) && !defined(foreach)
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

#endif // QT6COMPAT_H
