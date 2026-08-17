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
#include "translationservice.h"

#include "mainapplication.h"
#include "settings.h"
#include "aiassistant.h"

#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrl>
#include <QCryptographicHash>
#include <QDateTime>
#include <QDebug>

TranslationService::TranslationService(AIAssistant *ai, QObject *parent)
  : QObject(parent),
    ai_(ai)
{
  networkManager_ = new QNetworkAccessManager(this);
  connect(networkManager_, SIGNAL(finished(QNetworkReply*)),
          this, SLOT(slotReplyFinished()));
}

QString TranslationService::engine() const
{
  return Settings().value("AI/translationEngine", "google").toString();
}

bool TranslationService::isConfigured() const
{
  QString e = engine();
  if (e == "deepl") {
    return !Settings().value("AI/deeplKey").toString().isEmpty();
  } else if (e == "baidu") {
    return !Settings().value("AI/baiduAppId").toString().isEmpty() &&
           !Settings().value("AI/baiduKey").toString().isEmpty();
  } else if (e == "ai") {
    AIAssistant ai;
    return ai.isConfigured();
  }
  return true; // google: no key required
}

void TranslationService::translate(const QString &text, const QString &targetLang,
                                   int newsId)
{
  if (text.trimmed().isEmpty()) return;

  pendingTargetLang_ = targetLang;
  pendingNewsId_ = newsId;

  QString e = engine();
  if (e == "deepl")
    deeplTranslate(text, targetLang);
  else if (e == "baidu")
    baiduTranslate(text, targetLang);
  else if (e == "ai")
    aiTranslate(text, targetLang);
  else
    googleTranslate(text, targetLang);
}

// ----------------------------------------------------------------------------
void TranslationService::googleTranslate(const QString &text,
                                         const QString &targetLang)
{
  QUrl url("https://translate.googleapis.com/translate_a/single");
  QUrlQuery query;
  query.addQueryItem("client", "gtx");
  query.addQueryItem("sl", "auto");
  query.addQueryItem("tl", targetLang);
  query.addQueryItem("dt", "t");
  query.addQueryItem("q", text);
  url.setQuery(query);

  QNetworkRequest request{url};
  request.setRawHeader("User-Agent", "Mozilla/5.0");
  QNetworkReply *reply = networkManager_->get(request);
  Q_UNUSED(reply)
}

// ----------------------------------------------------------------------------
void TranslationService::deeplTranslate(const QString &text,
                                        const QString &targetLang)
{
  Settings settings;
  QString apiKey = settings.value("AI/deeplKey").toString();
  if (apiKey.isEmpty()) {
    emit requestFailed(tr("DeepL API key is not configured."));
    return;
  }

  QString target = targetLang.toUpper();
  if (target == "ZH")
    target = "ZH-HANS";

  QUrl url("https://api-free.deepl.com/v2/translate");
  QNetworkRequest request{url};
  request.setHeader(QNetworkRequest::ContentTypeHeader,
                    "application/x-www-form-urlencoded");
  request.setRawHeader("Authorization",
                       QString("DeepL-Auth-Key %1").arg(apiKey).toUtf8());

  QUrlQuery query;
  query.addQueryItem("text", text);
  query.addQueryItem("target_lang", target);
  QNetworkReply *reply = networkManager_->post(
        request, query.toString(QUrl::FullyEncoded).toUtf8());
  Q_UNUSED(reply)
}

// ----------------------------------------------------------------------------
void TranslationService::baiduTranslate(const QString &text,
                                        const QString &targetLang)
{
  Settings settings;
  QString appId = settings.value("AI/baiduAppId").toString();
  QString key = settings.value("AI/baiduKey").toString();
  if (appId.isEmpty() || key.isEmpty()) {
    emit requestFailed(tr("Baidu Translate is not configured."));
    return;
  }

  QString salt = QString::number(QDateTime::currentMSecsSinceEpoch());
  QString signSource = appId + text + salt + key;
  QString sign = QString(QCryptographicHash::hash(
        signSource.toUtf8(), QCryptographicHash::Md5).toHex());

  QUrl url("https://fanyi-api.baidu.com/api/trans/vip/translate");
  QNetworkRequest request{url};
  request.setHeader(QNetworkRequest::ContentTypeHeader,
                    "application/x-www-form-urlencoded");

  QUrlQuery query;
  query.addQueryItem("q", text);
  query.addQueryItem("from", "auto");
  query.addQueryItem("to", targetLang);
  query.addQueryItem("appid", appId);
  query.addQueryItem("salt", salt);
  query.addQueryItem("sign", sign);
  QNetworkReply *reply = networkManager_->post(
        request, query.toString(QUrl::FullyEncoded).toUtf8());
  Q_UNUSED(reply)
}

// ----------------------------------------------------------------------------
void TranslationService::aiTranslate(const QString &text,
                                     const QString &targetLang)
{
  if (!ai_) {
    emit requestFailed(tr("AI assistant is not available."));
    return;
  }
  AIAssistant::ArticleContext context;
  context.newsId = pendingNewsId_;
  ai_->translate(text, targetLang, context);
}

// ----------------------------------------------------------------------------
void TranslationService::slotReplyFinished()
{
  QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
  if (!reply) return;

  reply->deleteLater();

  if (reply->error() != QNetworkReply::NoError) {
    emit requestFailed(reply->errorString());
    return;
  }

  QByteArray data = reply->readAll();
  QString e = engine();
  QString translated;
  QString targetLang = pendingTargetLang_;
  int newsId = pendingNewsId_;

  if (e == "google") {
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonArray arr = doc.array();
    if (arr.isEmpty()) return;
    QJsonArray sentences = arr.at(0).toArray();
    foreach (const QJsonValue &v, sentences) {
      if (v.isArray() && v.toArray().count() > 0)
        translated += v.toArray().at(0).toString();
    }
  } else if (e == "deepl") {
    QJsonObject obj = QJsonDocument::fromJson(data).object();
    QJsonArray translations = obj.value("translations").toArray();
    if (!translations.isEmpty())
      translated = translations.at(0).toObject().value("text").toString();
  } else if (e == "baidu") {
    QJsonObject obj = QJsonDocument::fromJson(data).object();
    if (obj.contains("error_code")) {
      emit requestFailed(obj.value("error_msg").toString());
      return;
    }
    QJsonArray trans = obj.value("trans_result").toArray();
    foreach (const QJsonValue &v, trans) {
      translated += v.toObject().value("dst").toString();
      translated += "\n";
    }
    translated = translated.trimmed();
  }

  if (translated.trimmed().isEmpty()) {
    emit requestFailed(tr("Translation produced empty result."));
    return;
  }

  emit translationReady(newsId, translated.trimmed(), targetLang);
}
