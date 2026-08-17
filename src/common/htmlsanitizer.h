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
#ifndef HTMLSANITIZER_H
#define HTMLSANITIZER_H

#include <QString>

/** @brief Strip active content from untrusted article HTML before rendering.
 *
 * Pure static helpers, no dependency on the application singletons, so the
 * class can be unit-tested in isolation (tests/tests.pro).
 *
 * What is removed:
 *  - <script> / <style> blocks (any attribute casing, nested not possible)
 *  - <iframe>, <object>, <embed>, <applet>, <frameset>, <frame>, <base>,
 *    <link>, <meta http-equiv="refresh"> - tracking / hijack / embed vectors
 *  - every on* event attribute (onclick, onerror, onload, ...)
 *  - javascript: / vbscript: URLs in href/src/action/xlink:href/background
 *
 * Everything else (text, images, links, tables, code blocks) is preserved,
 * including file:// and data: URLs used by the offline image cache.
 *---------------------------------------------------------------------------*/
namespace HtmlSanitizer
{
  QString sanitize(const QString &html);
}

#endif // HTMLSANITIZER_H
