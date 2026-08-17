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
#include "htmlsanitizer.h"

#include <qzregexp.h>

namespace {

/** Remove every occurrence of the given regex from the string. */
QString removeAll(const QString &html, const QString &pattern,
                  Qt::CaseSensitivity cs = Qt::CaseInsensitive)
{
  QString result = html;
  // The constructor already sets DotMatchesEverythingOption and (for
  // cs==CaseInsensitive) ORs in CaseInsensitiveOption. Do NOT call
  // setPatternOptions() here: it *replaces* the whole option set, silently
  // dropping CaseInsensitive and turning <SCRIPT> handling into a
  // case-sensitive match.
  QzRegExp re(pattern, cs);
  result.remove(re);
  return result;
}

} // namespace

QString HtmlSanitizer::sanitize(const QString &html)
{
  if (html.isEmpty())
    return html;

  QString result = html;

  // 1. Active/embed element blocks. The [\s\S] class together with the
  //    DotMatchesEverything option guarantees cross-line matching even when
  //    the source uses CRLF line endings.
  result = removeAll(result, "<script[^>]*>[\\s\\S]*?</script>");
  result = removeAll(result, "<style[^>]*>[\\s\\S]*?</style>");
  result = removeAll(result, "<iframe[^>]*>[\\s\\S]*?</iframe>");
  result = removeAll(result, "<object[^>]*>[\\s\\S]*?</object>");
  result = removeAll(result, "<applet[^>]*>[\\s\\S]*?</applet>");
  result = removeAll(result, "<frameset[^>]*>[\\s\\S]*?</frameset>");

  // Self-closing / tag-only vectors.
  result = removeAll(result, "<embed[^>]*>");
  result = removeAll(result, "<frame[^>]*>");
  result = removeAll(result, "<base[^>]*>");
  result = removeAll(result, "<link[^>]*>");
  // <meta http-equiv="refresh" ...> can redirect the reader; drop it.
  QzRegExp metaRefresh("<meta[^>]+http-equiv\\s*=\\s*[\"']refresh[\"'][^>]*>",
                       Qt::CaseInsensitive);
  result.remove(metaRefresh);

  // 2. Event handler attributes (onclick="...", onerror='...', onload=...).
  QzRegExp eventAttr("\\s+on[a-z]+\\s*=\\s*(\"[^\"]*\"|'[^']*'|[^\\s>]+)",
                     Qt::CaseInsensitive);
  result.remove(eventAttr);

  // 3. Dangerous URL schemes. Replace the whole attribute value with a safe
  //    fragment so the surrounding markup stays intact.
  //    Quoted form first (href="javascript:..." / src='vbscript:...'): the
  //    back-reference \2 must match the *closing* quote too, otherwise the
  //    replacement leaves a dangling quote behind (href="#""> instead of
  //    href="#">). The captured quote is echoed so the original quote style
  //    is preserved:  src='vbscript:...'  ->  src='#'  (not src="#").
  QzRegExp jsUrlQuoted("(href|src|action|xlink:href|background)\\s*=\\s*([\"'])\\s*"
                       "(javascript|vbscript|data:text/html)\\s*:[^\"'>\\s]*\\2",
                       Qt::CaseInsensitive);
  result.replace(jsUrlQuoted, "\\1=\\2#\\2");
  //    Bare form (href=javascript:...): no quote to preserve.
  QzRegExp jsUrlBare("(href|src|action|xlink:href|background)\\s*=\\s*"
                     "(javascript|vbscript|data:text/html)\\s*:[^\"'>\\s]*",
                     Qt::CaseInsensitive);
  result.replace(jsUrlBare, "\\1=\"#\"");

  return result;
}
