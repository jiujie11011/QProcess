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
#ifndef LOCALSUMMARY_H
#define LOCALSUMMARY_H

#include <QString>
#include <QStringList>
#include <QVector>

/*! Pure-local extractive summarizer (no API required).
 *
 * Implements a TF-IDF weighted sentence scoring combined with a TextRank
 * graph ranking pass. Used as an offline fallback for the AI summarizer.
 */
class LocalSummarizer
{
public:
  /*! Produce an extractive summary of \a text of roughly \a sentenceCount
   *  sentences. Returns empty when there is nothing to summarize. */
  static QString summarize(const QString &text, int sentenceCount = 3);

private:
  /*! Split text into sentences on punctuation/newlines. */
  static QStringList splitSentences(const QString &text);
  /*! Normalize a sentence to its word tokens (lowercase, alphanumeric). */
  static QStringList tokenize(const QString &text);
  /*! Return the idf weight of a term given the term->docFrequency map and
   *  the total sentence count. */
  static double idf(int docFrequency, int totalDocs);
};

#endif // LOCALSUMMARY_H
