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
#ifndef TRANSLATIONSERVICE_H
#define TRANSLATIONSERVICE_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QStringList>

class QNetworkReply;
class AIAssistant;

/*! Multi-engine translation client (M-11).
 *
 *  Supports four engines selected by name:
 *    - "google": Google free web translator (no key required)
 *    - "deepl":  DeepL API (requires AI/deeplKey)
 *    - "baidu":  Baidu Translate API (requires AI/baiduAppId + AI/baiduKey)
 *    - "ai":     OpenAI-compatible LLM translation (reuses the AI config)
 *
 *  Results are delivered via translationReady(newsId, text, targetLang).
 *  The engine is read from the Settings "AI" group key translationEngine.
 */
class TranslationService : public QObject
{
  Q_OBJECT
public:
  explicit TranslationService(AIAssistant *ai, QObject *parent = 0);

  /*! Translate \a text into \a targetLang (e.g. "zh", "en") using the
   *  configured engine. Result delivered via translationReady.
   *  \a newsId is echoed back to let the caller update the DB row. */
  void translate(const QString &text, const QString &targetLang, int newsId);

  /*! The active engine name (google / deepl / baidu / ai). */
  QString engine() const;

  /*! True if the selected engine can be used (keys present when needed). */
  bool isConfigured() const;

signals:
  void translationReady(int newsId, const QString &text, const QString &targetLang);
  void requestFailed(const QString &error);

private slots:
  void slotReplyFinished();

private:
  void googleTranslate(const QString &text, const QString &targetLang);
  void deeplTranslate(const QString &text, const QString &targetLang);
  void baiduTranslate(const QString &text, const QString &targetLang);
  void aiTranslate(const QString &text, const QString &targetLang);

  QNetworkAccessManager *networkManager_;
  AIAssistant *ai_;
  QString pendingTargetLang_;
  int pendingNewsId_;
};

#endif // TRANSLATIONSERVICE_H
