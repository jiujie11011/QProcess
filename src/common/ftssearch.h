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
#ifndef FTSSEARCH_H
#define FTSSEARCH_H

#include <QString>

/** @brief Helpers for building SQLite FTS5 MATCH queries from user input.
 *
 * The database maintains an external-content FTS5 table (news_fts) over
 * news(title, content, description). FTS5's default unicode61 tokenizer
 * treats a run of CJK characters as a single token, so a Chinese substring
 * such as "新闻" would never match the token "新闻联播" - unlike LIKE.
 * Therefore the search entry point only uses MATCH for ASCII-only terms and
 * falls back to LIKE for everything else.
 *
 * Pure static helpers, no application dependencies -> unit-testable.
 *---------------------------------------------------------------------------*/
namespace FtsSearch
{
  /** True when the term contains only ASCII letters/digits/spaces - safe to
   *  hand to an FTS5 MATCH query without breaking Chinese matching. */
  bool isAsciiOnly(const QString &term);

  /** Escape a user term for use as a quoted phrase inside a MATCH clause.
   *  FTS5 phrases treat every character literally except the double quote,
   *  which must be doubled (""). A term with no characters yields "*". */
  QString matchTerm(const QString &term);
}

#endif // FTSSEARCH_H
