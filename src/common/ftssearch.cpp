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
#include "ftssearch.h"

bool FtsSearch::isAsciiOnly(const QString &term)
{
  const QChar *data = term.constData();
  const int len = term.length();
  for (int i = 0; i < len; ++i) {
    const ushort c = data[i].unicode();
    // Letters, digits, space and common punctuation are fine; anything in the
    // CJK/Fullwidth/Extension ranges forces the LIKE fallback.
    if (c > 0x007E)
      return false;
  }
  return true;
}

QString FtsSearch::matchTerm(const QString &term)
{
  if (term.trimmed().isEmpty())
    return QStringLiteral("*");

  QString escaped;
  escaped.reserve(term.length() + 2);
  escaped.append(QLatin1Char('"'));
  const QChar *data = term.constData();
  const int len = term.length();
  for (int i = 0; i < len; ++i) {
    const QChar c = data[i];
    if (c == QLatin1Char('"'))
      escaped.append(QLatin1String("\"\"")); // FTS5 phrase escape
    else
      escaped.append(c);
  }
  escaped.append(QLatin1Char('"'));
  return escaped;
}
