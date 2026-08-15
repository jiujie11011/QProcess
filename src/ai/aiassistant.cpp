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
#include "aiassistant.h"

#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSqlQuery>
#include <QSqlError>
#include <QJsonDocument>
#include <QJsonArray>
#include <QDateTime>
#include <QFile>
#include <QCryptographicHash>
#include <QDebug>

#include "settings.h"
#include "statistics.h"
#include "database.h"
#include "mainapplication.h"

// ----------------------------------------------------------------------------
AIAssistant::AIAssistant(QObject *parent)
  : QObject(parent)
  , db_(Database::connection("secondConnection"))
  , currentReply_(0)
  , pendingIsTranslation_(false)
  , pendingSummaryNewsId_(0)
  , pendingRecommendationsNewsId_(0)
{
  if (!db_.isValid())
    db_ = QSqlDatabase::database();

  networkManager_ = new QNetworkAccessManager(this);
  connect(networkManager_, SIGNAL(finished(QNetworkReply*)),
          this, SLOT(slotReplyFinished()));
}

// ----------------------------------------------------------------------------
bool AIAssistant::isConfigured() const
{
  Settings settings;
  // The "Enable AI assistant" master switch in Settings -> AI must be on.
  if (!settings.value("AI/aiEnabled", false).toBool())
    return false;
  // Ollama runs locally and needs no API key.
  if (settings.value("AI/provider", "openai").toString() == "ollama")
    return !baseUrl().isEmpty();
  return !apiKey().isEmpty();
}

// ----------------------------------------------------------------------------
QString AIAssistant::apiKey() const
{
  Settings settings;
  return settings.value("AI/apiKey").toString();
}

// ----------------------------------------------------------------------------
QString AIAssistant::model() const
{
  Settings settings;
  return settings.value("AI/model", "gpt-4o-mini").toString();
}

// ----------------------------------------------------------------------------
QString AIAssistant::baseUrl() const
{
  Settings settings;
  return settings.value("AI/baseUrl",
                        "https://api.openai.com/v1/chat/completions").toString();
}

// ----------------------------------------------------------------------------
QString AIAssistant::truncateContext(const QString &text, int maxChars) const
{
  if (maxChars <= 0 || text.length() <= maxChars) return text;
  return text.left(maxChars) + "...";
}

// ----------------------------------------------------------------------------
QString AIAssistant::buildSystemPrompt(const ArticleContext &context) const
{
  if (context.newsId <= 0) return QString();

  QString system = "You are a helpful reading assistant integrated into an RSS reader. ";
  system += "Answer in the same language as the user. ";
  system += "When asked to summarize or analyze, base your answer on the provided article. ";
  if (!context.title.isEmpty())
    system += QString("Current article title: \"%1\". ").arg(context.title);
  return system;
}

// ----------------------------------------------------------------------------
QString AIAssistant::buildUserPrompt(const QString &instruction,
                                     const ArticleContext &context) const
{
  QString prompt;
  if (!instruction.isEmpty())
    prompt += instruction + "\n\n";
  if (context.newsId > 0) {
    if (!context.title.isEmpty())
      prompt += QString("Title: %1\n").arg(context.title);
    if (!context.content.isEmpty())
      prompt += QString("Content: %1\n").arg(
            truncateContext(context.content, 4000));
    if (!context.category.isEmpty())
      prompt += QString("Category: %1\n").arg(context.category);
  }
  return prompt.trimmed();
}

// ----------------------------------------------------------------------------
void AIAssistant::storeMessage(const QString &role, const QString &content)
{
  if (content.isEmpty()) return;

  QSqlQuery q(db_);
  q.prepare("INSERT INTO dialog(date, feedId, newsId, role, content) "
            "VALUES(?, ?, ?, ?, ?)");
  q.addBindValue(QDateTime::currentDateTime().toString(Qt::ISODate));
  q.addBindValue(lastContext_.feedId);
  q.addBindValue(lastContext_.newsId);
  q.addBindValue(role);
  q.addBindValue(content);
  if (!q.exec())
    qDebug() << "AIAssistant::storeMessage error:" << q.lastError().text();
}

// ----------------------------------------------------------------------------
void AIAssistant::countEvent(const QString &type)
{
  StatisticsService stats(db_);
  stats.addEvent(type);
}

// ----------------------------------------------------------------------------
bool AIAssistant::offlineCacheEnabled() const
{
  return Settings().value("AI/offlineCache", false).toBool();
}

// ----------------------------------------------------------------------------
QString AIAssistant::cacheFilePath() const
{
  return mainApp->dataDir() + "/ai_cache.json";
}

// ----------------------------------------------------------------------------
QString AIAssistant::cachedResponse(const QString &key) const
{
  QFile file(cacheFilePath());
  if (!file.open(QIODevice::ReadOnly))
    return QString();
  QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
  file.close();
  return doc.object().value(key).toString();
}

// ----------------------------------------------------------------------------
void AIAssistant::cacheResponse(const QString &key, const QString &text)
{
  if (!offlineCacheEnabled() || text.isEmpty()) return;

  QFile file(cacheFilePath());
  QJsonObject cache;
  if (file.open(QIODevice::ReadOnly)) {
    cache = QJsonDocument::fromJson(file.readAll()).object();
    file.close();
  }
  cache.insert(key, text);
  if (file.open(QIODevice::WriteOnly | QIODevice::Truncate))
    file.write(QJsonDocument(cache).toJson(QJsonDocument::Compact));
  file.close();
}

// ----------------------------------------------------------------------------
QNetworkReply *AIAssistant::sendRequest(const QString &url,
                                        const QJsonObject &payload,
                                        const QString &systemPrompt,
                                        const QString &userPrompt)
{
  if (!isConfigured()) {
    emit requestFailed(tr("AI API key is not configured. "
                          "Open Settings -> AI to set it up."));
    return 0;
  }

  QNetworkRequest request{QUrl(url)};
  request.setHeader(QNetworkRequest::ContentTypeHeader,
                    "application/json");
  request.setRawHeader("Authorization",
                       QString("Bearer %1").arg(apiKey()).toUtf8());

  QJsonObject body = payload;
  body.insert("model", model());
  int maxTokens = Settings().value("AI/maxTokens", 0).toInt();
  if (maxTokens > 0)
    body.insert("max_tokens", maxTokens);
  QJsonArray messages;
  if (!systemPrompt.isEmpty())
    messages.append(QJsonObject{{"role", "system"}, {"content", systemPrompt}});
  messages.append(QJsonObject{{"role", "user"}, {"content", userPrompt}});
  body.insert("messages", messages);

  QNetworkReply *reply = networkManager_->post(
        request, QJsonDocument(body).toJson(QJsonDocument::Compact));
  return reply;
}

// ----------------------------------------------------------------------------
void AIAssistant::performRequest(const QString &systemPrompt,
                                 const QString &userPrompt,
                                 const QString &eventType)
{
  QString key = QCryptographicHash::hash(
        (model() + "\n" + systemPrompt + "\n" + userPrompt).toUtf8(),
        QCryptographicHash::Md5).toHex();

  if (offlineCacheEnabled()) {
    QString cached = cachedResponse(key);
    if (!cached.isEmpty()) {
      storeMessage("assistant", cached);
      emit responseReady(cached);
      return;
    }
  }

  if (currentReply_) {
    currentReply_->abort();
    currentReply_ = 0;
  }

  QNetworkReply *reply = sendRequest(baseUrl(), QJsonObject(), systemPrompt, userPrompt);
  if (!reply) return;
  currentReply_ = reply;
  if (!eventType.isEmpty())
    countEvent(eventType);
  currentCacheKey_ = key;
}

// ----------------------------------------------------------------------------
void AIAssistant::sendMessage(const QString &prompt, const ArticleContext &context)
{
  if (prompt.trimmed().isEmpty()) return;

  lastPrompt_ = prompt;
  lastContext_ = context;
  pendingIsTranslation_ = false;

  QString system = buildSystemPrompt(context);
  QString user = buildUserPrompt(prompt, context);

  storeMessage("user", prompt);
  performRequest(system, user, StatType::AIChat);
}

// ----------------------------------------------------------------------------
void AIAssistant::requestSummary(const ArticleContext &context)
{
  lastContext_ = context;
  pendingIsTranslation_ = false;
  QString system = buildSystemPrompt(context);
  QString instruction = QString("Please summarize this article in about %1 words.").
      arg(Settings().value("AI/summaryLength", 200).toInt());
  QString user = buildUserPrompt(instruction, context);

  performRequest(system, user, StatType::AISummary);
}

// ----------------------------------------------------------------------------
void AIAssistant::requestAutoSummary(const ArticleContext &context)
{
  if (context.newsId <= 0) return;

  lastContext_ = context;
  pendingIsTranslation_ = false;
  pendingSummaryNewsId_ = context.newsId;

  QString system = buildSystemPrompt(context);
  QString instruction = QString("Please summarize this article in about %1 words.").
      arg(Settings().value("AI/summaryLength", 200).toInt());
  QString user = buildUserPrompt(instruction, context);

  performRequest(system, user, StatType::AISummary);
}

// ----------------------------------------------------------------------------
void AIAssistant::translate(const QString &text, const QString &targetLang,
                            const ArticleContext &context)
{
  if (text.trimmed().isEmpty()) return;

  lastContext_ = context;
  pendingTargetLang_ = targetLang;
  pendingIsTranslation_ = true;

  QString system = QString(
        "You are a professional translator. Translate the given text into %1. "
        "Preserve formatting and meaning. Output only the translation.")
      .arg(targetLang);
  QString user = text;

  performRequest(system, user, QString());
}

// ----------------------------------------------------------------------------
void AIAssistant::requestRecommendations(const ArticleContext &context)
{
  lastContext_ = context;
  pendingIsTranslation_ = false;
  QString system = buildSystemPrompt(context);
  QString instruction = QString(
        "Based on this article, recommend a few related topics or articles "
        "that the reader may find interesting. List them briefly.");
  QString user = buildUserPrompt(instruction, context);

  performRequest(system, user, StatType::AIChat);
}

// ----------------------------------------------------------------------------
void AIAssistant::requestAutoRecommendations(const ArticleContext &context)
{
  if (context.newsId <= 0) return;

  lastContext_ = context;
  pendingIsTranslation_ = false;
  pendingSummaryNewsId_ = 0;
  pendingRecommendationsNewsId_ = context.newsId;

  QString system = buildSystemPrompt(context);
  QString instruction = QString(
        "Based on this article, recommend a few related articles. "
        "Return a JSON array where each item has 'title' and 'link' fields. "
        "Example: [{\"title\":\"...\",\"link\":\"...\"}]");
  QString user = buildUserPrompt(instruction, context);

  performRequest(system, user, StatType::AIChat);
}

// ----------------------------------------------------------------------------
void AIAssistant::markSummarized(int newsId)
{
  if (newsId <= 0) return;
  QSqlQuery q(db_);
  q.prepare("UPDATE news SET aiSummary=1 WHERE id=?");
  q.addBindValue(newsId);
  q.exec();
}

// ----------------------------------------------------------------------------
void AIAssistant::retryLast()
{
  if (lastPrompt_.isEmpty()) return;
  pendingIsTranslation_ = false;
  QString system = buildSystemPrompt(lastContext_);
  QString user = buildUserPrompt(lastPrompt_, lastContext_);
  storeMessage("user", lastPrompt_);
  performRequest(system, user, StatType::AIChat);
}

// ----------------------------------------------------------------------------
void AIAssistant::slotReplyFinished()
{
  QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
  if (!reply) return;

  if (reply != currentReply_) {
    reply->deleteLater();
    return;
  }
  currentReply_ = 0;

  if (reply->error() != QNetworkReply::NoError) {
    emit requestFailed(reply->errorString());
    reply->deleteLater();
    return;
  }

  QByteArray data = reply->readAll();
  reply->deleteLater();

  QJsonDocument doc = QJsonDocument::fromJson(data);
  QJsonArray choices = doc.object().value("choices").toArray();
  if (choices.isEmpty()) {
    emit requestFailed(tr("AI response parse error."));
    return;
  }
  QString text = choices.at(0).toObject().value("message").
      toObject().value("content").toString();

  if (text.isEmpty()) {
    emit requestFailed(tr("AI returned an empty response."));
    return;
  }

  bool isTranslation = pendingIsTranslation_;
  QString targetLang = pendingTargetLang_;
  int newsId = lastContext_.newsId;
  int summaryNewsId = pendingSummaryNewsId_;
  int recommendationsNewsId = pendingRecommendationsNewsId_;
  pendingIsTranslation_ = false;
  pendingTargetLang_.clear();
  pendingSummaryNewsId_ = 0;
  pendingRecommendationsNewsId_ = 0;

  if (recommendationsNewsId > 0) {
    if (!currentCacheKey_.isEmpty())
      cacheResponse(currentCacheKey_, text);
    currentCacheKey_.clear();

    QSqlQuery recQ(db_);
    recQ.prepare("INSERT INTO recommendations(newsId, date, content) "
                 "VALUES(?, ?, ?)");
    recQ.addBindValue(recommendationsNewsId);
    recQ.addBindValue(QDateTime::currentDateTime().toString(Qt::ISODate));
    recQ.addBindValue(text);
    recQ.exec();

    emit recommendationsReady(recommendationsNewsId, text);
    return;
  }

  if (summaryNewsId > 0) {
    if (!currentCacheKey_.isEmpty())
      cacheResponse(currentCacheKey_, text);
    currentCacheKey_.clear();
    emit summaryReady(summaryNewsId, text);
    return;
  }

  if (isTranslation) {
    if (!currentCacheKey_.isEmpty())
      cacheResponse(currentCacheKey_, text);
    currentCacheKey_.clear();
    emit translationReady(newsId, text, targetLang);
    return;
  }

  storeMessage("assistant", text);
  if (!currentCacheKey_.isEmpty())
    cacheResponse(currentCacheKey_, text);
  currentCacheKey_.clear();
  emit responseReady(text);
}

// ----------------------------------------------------------------------------
QList<QJsonObject> AIAssistant::loadHistory(int feedId, int newsId) const
{
  QList<QJsonObject> list;
  QSqlQuery q(db_);
  QString qStr = "SELECT date, feedId, newsId, role, content "
                 "FROM dialog ORDER BY id";
  if (feedId > 0 || newsId > 0) {
    QStringList cond;
    if (feedId > 0) cond << "feedId=?";
    if (newsId > 0) cond << "newsId=?";
    qStr += " WHERE " + cond.join(" AND ");
  }
  q.prepare(qStr);
  if (feedId > 0) q.addBindValue(feedId);
  if (newsId > 0) q.addBindValue(newsId);
  if (!q.exec()) {
    qDebug() << "AIAssistant::loadHistory error:" << q.lastError().text();
    return list;
  }
  while (q.next()) {
    QJsonObject item;
    item.insert("date", q.value(0).toString());
    item.insert("feedId", q.value(1).toInt());
    item.insert("newsId", q.value(2).toInt());
    item.insert("role", q.value(3).toString());
    item.insert("content", q.value(4).toString());
    list.append(item);
  }
  return list;
}
