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
#ifndef AIASSISTANT_H
#define AIASSISTANT_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QSqlDatabase>
#include <QJsonObject>
#include <QStringList>

class QNetworkReply;
class QNetworkRequest;

/*! Asynchronous OpenAI-compatible chat completion client.
 *
 * Reads configuration from the Settings "AI" group (apiKey/model/baseUrl
 * provided by the user on the settings page). Requests are asynchronous and
 * never block the news parsing main flow. Dialog turns are persisted to the
 * dialog table and counted in the stats table.
 */
class AIAssistant : public QObject
{
  Q_OBJECT
public:
  struct ArticleContext {
    int feedId = 0;
    int newsId = 0;
    QString title;
    QString content;
    QString category;
  };

  explicit AIAssistant(QObject *parent = 0);

  /*! True if the user configured an API key. */
  bool isConfigured() const;
  QString apiKey() const;
  QString model() const;
  QString baseUrl() const;

  /*! Send a chat turn. On success emits responseReady with the assistant
   *  text; on failure emits requestFailed. Never blocks the caller. */
  void sendMessage(const QString &prompt, const ArticleContext &context);

  /*! Request a summary of the current article context. */
  void requestSummary(const ArticleContext &context);

  /*! Request an automatic summary for \a context.newsId and deliver the
   *  result via summaryReady. Used by the read-auto-trigger feature. */
  void requestAutoSummary(const ArticleContext &context);

  /*! Translate the given text into \a targetLang (e.g. "zh", "en").
   *  Result is delivered via responseReady. */
  void translate(const QString &text, const QString &targetLang,
                 const ArticleContext &context);

  /*! Request related article suggestions. */
  void requestRecommendations(const ArticleContext &context);

  /*! Request automatic recommendations for \a context.newsId. The result is
   *  delivered via recommendationsReady and stored in the recommendations
   *  table. Used by the read-auto-recommend feature. */
  void requestAutoRecommendations(const ArticleContext &context);

  /*! Mark a news row as having an AI summary (aiSummary=1). */
  void markSummarized(int newsId);

  /*! Retry the last failed request. */
  void retryLast();

  /*! Test the connection using the given settings without touching the
   *  persisted configuration. Results are delivered via connectionTested. */
  void testConnection(const QString &baseUrl, const QString &apiKey,
                      const QString &model);

  /*! Fetch the model ids advertised by an OpenAI-compatible endpoint.
   *  The models list is derived from \a baseUrl (e.g. .../v1/chat/completions
   *  becomes .../v1/models). Results are delivered via modelsFetched. */
  void fetchModels(const QString &baseUrl, const QString &apiKey);

  /*! Return recent dialog records from the dialog table.
   *  -1 for newsId or feedId matches any. */
  QList<QJsonObject> loadHistory(int feedId = -1, int newsId = -1) const;

signals:
  void responseReady(const QString &text);
  void requestFailed(const QString &error);
  /*! Emitted when a translate() request completes.
   *  \a newsId is 0 when no article context was attached. */
  void translationReady(int newsId, const QString &text, const QString &targetLang);
  /*! Emitted when a requestAutoSummary() request completes. */
  void summaryReady(int newsId, const QString &text);
  /*! Emitted when a requestAutoRecommendations() request completes. */
  void recommendationsReady(int newsId, const QString &content);
  /*! Emitted when a testConnection() request completes. */
  void connectionTested(bool ok, const QString &message);
  /*! Emitted when a fetchModels() request completes (empty on failure). */
  void modelsFetched(const QStringList &models);

private slots:
  void slotReplyFinished();

private:
  QNetworkReply *sendRequest(const QString &url,
                             const QJsonObject &payload,
                             const QString &systemPrompt,
                             const QString &userPrompt);
  /*! Cache-aware request: returns cached response via responseReady when a hit
   *  occurs (and cache is enabled), otherwise performs the network request.
   *  \a eventType is the stats event to count for a live request. */
  void performRequest(const QString &systemPrompt,
                      const QString &userPrompt,
                      const QString &eventType);
  void storeMessage(const QString &role, const QString &content);
  QString buildSystemPrompt(const ArticleContext &context) const;
  QString buildUserPrompt(const QString &instruction, const ArticleContext &context) const;
  QString truncateContext(const QString &text, int maxChars) const;
  void countEvent(const QString &type);

  /*! True when AI/offlineCache is enabled. */
  bool offlineCacheEnabled() const;
  /*! Cache file path (dataDir/ai_cache.json). */
  QString cacheFilePath() const;
  /*! Return cached response for the given prompt, empty when absent. */
  QString cachedResponse(const QString &key) const;
  /*! Store a successful response in the offline cache. */
  void cacheResponse(const QString &key, const QString &text);

  QNetworkAccessManager *networkManager_;
  QSqlDatabase db_;
  QNetworkReply *currentReply_;
  /*! Replies for testConnection() / fetchModels() — they use their own
   *  settings arguments and are therefore handled outside currentReply_. */
  QNetworkReply *pendingTestReply_;
  QNetworkReply *pendingModelsReply_;
  QString lastPrompt_;
  QString currentCacheKey_;
  QString pendingTargetLang_;
  bool pendingIsTranslation_;
  int pendingSummaryNewsId_;
  int pendingRecommendationsNewsId_;
  ArticleContext lastContext_;
};

#endif // AIASSISTANT_H
