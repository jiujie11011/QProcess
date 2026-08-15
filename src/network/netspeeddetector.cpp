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
#include "netspeeddetector.h"

#include "networkmanager.h"

#include <QNetworkReply>
#include <QTimer>
#include <QUrl>

// ----------------------------------------------------------------------------
NetworkSpeedDetector::NetworkSpeedDetector(QObject *parent)
  : QObject(parent)
  , networkManager_(0)
  , currentReply_(0)
  , latencyMs_(0)
  , bandwidthMbps_(0.0)
  , suggestedConcurrency_(5)
  , running_(false)
  , bandwidthTest_(false)
{
  timeoutTimer_ = new QTimer(this);
  timeoutTimer_->setSingleShot(true);
  connect(timeoutTimer_, SIGNAL(timeout()), this, SLOT(slotTimeout()));
}
// ----------------------------------------------------------------------------
NetworkSpeedDetector::~NetworkSpeedDetector()
{
  abortCurrentReply();
  delete networkManager_;
}
// ----------------------------------------------------------------------------
void NetworkSpeedDetector::startDetection(const QStringList &probeUrls)
{
  if (running_)
    return;

  running_ = true;
  bandwidthTest_ = false;
  latencyMs_ = 0;
  bandwidthMbps_ = 0.0;

  if (!networkManager_) {
    networkManager_ = new NetworkManager(false);
    networkManager_->loadSettings();
    connect(networkManager_, SIGNAL(finished(QNetworkReply*)),
            this, SLOT(slotReplyFinished(QNetworkReply*)));
  }

  // Collect up to three distinct hosts from the user's own feeds, falling
  // back to a small set of well-known hosts when nothing is configured yet.
  QStringList hosts;
  foreach (const QString &url, probeUrls) {
    QString host = QUrl(url).host();
    if (host.isEmpty() || hosts.contains(host))
      continue;
    hosts << host;
    if (hosts.count() >= 3)
      break;
  }
  if (hosts.isEmpty())
    hosts << "www.baidu.com" << "www.github.com" << "www.google.com";

  bandwidthUrl_ = probeUrls.isEmpty() ? QString() : probeUrls.first();
  headQueue_.clear();
  foreach (const QString &host, hosts)
    headQueue_.enqueue(QString("http://%1/").arg(host));

  timeoutTimer_->start(20000);
  sendNextHeadRequest();
}
// ----------------------------------------------------------------------------
QNetworkRequest NetworkSpeedDetector::makeRequest(const QUrl &url) const
{
  QNetworkRequest request(url);
  request.setRawHeader("User-Agent", "QuiteRSS/0.19 (network speed detection)");
  request.setAttribute(QNetworkRequest::CacheLoadControlAttribute,
                       QNetworkRequest::AlwaysNetwork);
  return request;
}

void NetworkSpeedDetector::sendNextHeadRequest()
{
  if (headQueue_.isEmpty()) {
    startBandwidthTest();
    return;
  }

  latencyElapsed_.start();
  currentReply_ = networkManager_->head(makeRequest(QUrl(headQueue_.dequeue())));
}
// ----------------------------------------------------------------------------
void NetworkSpeedDetector::startBandwidthTest()
{
  if (bandwidthUrl_.isEmpty()) {
    // No feed URL to measure throughput on - classify from latency only.
    finishDetection(latencyMs_ > 0);
    return;
  }

  bandwidthTest_ = true;
  bandwidthElapsed_.start();
  currentReply_ = networkManager_->get(makeRequest(QUrl(bandwidthUrl_)));
}
// ----------------------------------------------------------------------------
void NetworkSpeedDetector::slotReplyFinished(QNetworkReply *reply)
{
  if (reply != currentReply_)
    return;
  currentReply_ = 0;
  reply->deleteLater();

  if (!bandwidthTest_) {
    // Latency phase: any HTTP response proves the host is reachable.
    int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (!reply->error() || status >= 100) {
      if (latencyMs_ == 0)
        latencyMs_ = latencyElapsed_.elapsed();
      startBandwidthTest();
      return;
    }
    // This host failed - try the next one.
    sendNextHeadRequest();
  } else {
    // Bandwidth phase: measure achieved throughput.
    qint64 bytes = reply->readAll().size();
    double duration = bandwidthElapsed_.elapsed() / 1000.0;
    if (duration > 0.0)
      bandwidthMbps_ = (bytes * 8.0) / (1024.0 * 1024.0) / duration;
    finishDetection(bytes > 0);
  }
}
// ----------------------------------------------------------------------------
void NetworkSpeedDetector::slotTimeout()
{
  abortCurrentReply();
  finishDetection(latencyMs_ > 0);
}
// ----------------------------------------------------------------------------
void NetworkSpeedDetector::finishDetection(bool success)
{
  if (!running_)
    return;
  running_ = false;
  timeoutTimer_->stop();

  if (success)
    suggestedConcurrency_ = concurrencyFromSpeed(latencyMs_, bandwidthMbps_);
  else
    suggestedConcurrency_ = 8; // safe default (do not stall feed refresh)

  emit signalDetectionFinished(success, latencyMs_,
                               bandwidthMbps_, suggestedConcurrency_);
}
// ----------------------------------------------------------------------------
void NetworkSpeedDetector::abortCurrentReply()
{
  if (currentReply_) {
    QNetworkReply *reply = currentReply_;
    currentReply_ = 0;
    reply->disconnect(this);
    reply->abort();
    reply->deleteLater();
  }
}
// ----------------------------------------------------------------------------
bool NetworkSpeedDetector::isRunning() const
{
  return running_;
}
// ----------------------------------------------------------------------------
int NetworkSpeedDetector::suggestedConcurrency() const
{
  return suggestedConcurrency_;
}
// ----------------------------------------------------------------------------
int NetworkSpeedDetector::concurrencyFromSpeed(int latencyMs, double bandwidthMbps)
{
  // Same thresholds as MrRSS's detector, scaled to QuiteRSS's pool cap (10).
  // The floor is kept at 6 so a misjudged "slow" link (or a short feed that
  // makes the bandwidth probe read low) cannot throttle feed refreshes
  // to 3 parallel requests.
  // slow   (latency > 300 ms or bandwidth < 2  Mbps) -> 6 concurrent
  // medium (latency > 100 ms or bandwidth < 20 Mbps) -> 8 concurrent
  // fast                                              -> 10 concurrent
  if (latencyMs > 300 || bandwidthMbps < 2.0)
    return 6;
  if (latencyMs > 100 || bandwidthMbps < 20.0)
    return 8;
  return 10;
}
// ----------------------------------------------------------------------------
