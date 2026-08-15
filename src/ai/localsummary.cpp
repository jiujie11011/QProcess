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
#include "localsummary.h"

#include <QHash>
#include <QSet>
#include <QtAlgorithms>
#include <QtMath>

namespace {

/*! English stop words removed before scoring. */
const QSet<QString> &stopWords()
{
  static const QSet<QString> words = QSet<QString>()
      << "a" << "an" << "the" << "and" << "or" << "but" << "if" << "then"
      << "of" << "in" << "on" << "at" << "to" << "for" << "with" << "by"
      << "from" << "as" << "is" << "are" << "was" << "were" << "be" << "been"
      << "being" << "it" << "its" << "this" << "that" << "these" << "those"
      << "i" << "you" << "he" << "she" << "we" << "they" << "them" << "his"
      << "her" << "their" << "my" << "our" << "your" << "not" << "no" << "so"
      << "just" << "very" << "about" << "into" << "over" << "also" << "than"
      << "which" << "who" << "whom" << "what" << "when" << "where" << "how"
      << "why" << "will" << "would" << "can" << "could" << "should" << "may"
      << "might" << "has" << "have" << "had" << "do" << "does" << "did" << "s"
      << "t" << "re" << "ve" << "ll" << "the" << "and";
  return words;
}

/*! Similarity between two sentences: overlap of unique tokens / sqrt(n*m). */
double similarity(const QStringList &a, const QStringList &b)
{
  if (a.isEmpty() || b.isEmpty()) return 0.0;
  QSet<QString> setB = b.toSet();
  int overlap = 0;
  foreach (const QString &tok, a.toSet()) {
    if (setB.contains(tok)) ++overlap;
  }
  return overlap / qSqrt((double)a.size() * (double)b.size());
}

} // namespace

// ----------------------------------------------------------------------------
QString LocalSummarizer::summarize(const QString &text, int sentenceCount)
{
  QStringList sentences = splitSentences(text);
  if (sentences.size() <= sentenceCount)
    return text;

  // Build token lists per sentence.
  QVector<QStringList> tokens(sentences.size());
  QHash<QString, int> docFreq; // term -> number of sentences containing it
  for (int i = 0; i < sentences.size(); ++i) {
    tokens[i] = tokenize(sentences[i]);
    foreach (const QString &tok, tokens[i].toSet())
      ++docFreq[tok];
  }

  int total = sentences.size();
  double d = 0.85; // TextRank damping factor

  // Iterative TextRank over sentence similarity graph.
  QVector<double> score(total, 1.0);
  for (int iter = 0; iter < 30; ++iter) {
    QVector<double> next(total, 0.0);
    for (int i = 0; i < total; ++i) {
      double incoming = 0.0;
      double outgoingSum = 0.0;
      for (int j = 0; j < total; ++j) {
        if (i == j) continue;
        double sim = similarity(tokens[i], tokens[j]);
        if (sim <= 0.0) continue;
        // Outgoing strength of sentence j.
        double out = 0.0;
        for (int k = 0; k < total; ++k) {
          if (j == k) continue;
          out += similarity(tokens[j], tokens[k]);
        }
        if (out > 0.0)
          incoming += sim * score[j] / out;
      }
      next[i] = (1.0 - d) + d * incoming;
      outgoingSum = 0.0; // keep unused-variable check satisfied
      Q_UNUSED(outgoingSum)
    }
    score = next;
  }

  // TF-IDF position bonus.
  QVector<double> tfidf(total, 0.0);
  for (int i = 0; i < total; ++i) {
    QHash<QString, int> tf;
    foreach (const QString &tok, tokens[i])
      ++tf[tok];
    double s = 0.0;
    QHash<QString, int>::const_iterator it = tf.constBegin();
    for (; it != tf.constEnd(); ++it) {
      if (stopWords().contains(it.key())) continue;
      int df = docFreq.value(it.key(), 1);
      s += (double)it.value() * idf(df, total);
    }
    // Position prior: earlier sentences slightly favored.
    tfidf[i] = s * (1.0 - 0.2 * ((double)i / total));
  }

  // Combined score.
  QVector<double> combined(total);
  for (int i = 0; i < total; ++i)
    combined[i] = 0.6 * score[i] + 0.4 * tfidf[i];

  // Pick top sentenceCount sentence indices preserving original order.
  QVector<int> order(total);
  for (int i = 0; i < total; ++i) order[i] = i;
  for (int i = 0; i < total; ++i) {
    for (int j = i + 1; j < total; ++j) {
      if (combined[order[j]] > combined[order[i]])
        qSwap(order[i], order[j]);
    }
  }

  QVector<int> picked;
  for (int i = 0; i < total && picked.size() < sentenceCount; ++i) {
    if (combined[order[i]] > 0.0)
      picked.append(order[i]);
  }
  if (picked.isEmpty()) {
    // Fallback: first sentenceCount sentences.
    for (int i = 0; i < sentenceCount && i < total; ++i)
      picked.append(i);
  }
  qSort(picked);

  QStringList result;
  foreach (int idx, picked)
    result << sentences[idx];
  return result.join(" ");
}

// ----------------------------------------------------------------------------
QStringList LocalSummarizer::splitSentences(const QString &text)
{
  QStringList out;
  QString current;
  QChar last = QChar();
  for (int i = 0; i < text.size(); ++i) {
    QChar c = text.at(i);
    current.append(c);
    if (c == '.' || c == '!' || c == '?' || c == '\n' || c == '\r') {
      QString s = current.simplified();
      if (!s.isEmpty()) out << s;
      current.clear();
    }
    last = c;
  }
  Q_UNUSED(last)
  QString s = current.simplified();
  if (!s.isEmpty()) out << s;
  return out;
}

// ----------------------------------------------------------------------------
QStringList LocalSummarizer::tokenize(const QString &text)
{
  QStringList out;
  QString word;
  for (int i = 0; i < text.size(); ++i) {
    QChar c = text.at(i).toLower();
    if (c.isLetterOrNumber()) {
      word.append(c);
    } else {
      if (!word.isEmpty()) {
        out << word;
        word.clear();
      }
    }
  }
  if (!word.isEmpty()) out << word;
  return out;
}

// ----------------------------------------------------------------------------
double LocalSummarizer::idf(int docFrequency, int totalDocs)
{
  if (docFrequency <= 0) docFrequency = 1;
  return qLn((double)totalDocs / docFrequency + 1.0);
}
