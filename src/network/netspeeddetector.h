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
#ifndef NETSPEEDDETECTOR_H
#define NETSPEEDDETECTOR_H

#include <QObject>
#include <QElapsedTimer>
#include <QNetworkRequest>
#include <QQueue>
#include <QStringList>

class NetworkManager;
class QNetworkReply;
class QTimer;

/** @brief Measures network latency/bandwidth and suggests a download
 *         concurrency, mirroring MrRSS's speed detection approach.
 *
 *  Phase 1 (latency): sends HEAD requests to up to three distinct hosts.
 *  Phase 2 (bandwidth): downloads the first user feed URL (when available)
 *  and measures the achieved throughput.
 *
 *  The result is mapped to a concurrency value: slow -> 3, medium -> 6,
 *  fast -> 10 (QuiteRSS caps the download pool at 10).
 *----------------------------------------------------------------------------*/
class NetworkSpeedDetector : public QObject
{
  Q_OBJECT
public:
  explicit NetworkSpeedDetector(QObject *parent = 0);
  ~NetworkSpeedDetector();

  void startDetection(const QStringList &probeUrls);
  bool isRunning() const;
  int suggestedConcurrency() const;

  static int concurrencyFromSpeed(int latencyMs, double bandwidthMbps);

signals:
  void signalDetectionFinished(bool success, int latencyMs,
                               double bandwidthMbps, int concurrency);

private slots:
  void slotReplyFinished(QNetworkReply *reply);
  void slotTimeout();

private:
  QNetworkRequest makeRequest(const QUrl &url) const;
  void sendNextHeadRequest();
  void startBandwidthTest();
  void finishDetection(bool success);
  void abortCurrentReply();

  NetworkManager *networkManager_;
  QNetworkReply *currentReply_;
  QTimer *timeoutTimer_;
  QQueue<QString> headQueue_;
  QString bandwidthUrl_;
  QElapsedTimer latencyElapsed_;
  QElapsedTimer bandwidthElapsed_;
  int latencyMs_;
  double bandwidthMbps_;
  int suggestedConcurrency_;
  bool running_;
  bool bandwidthTest_;

};

#endif // NETSPEEDDETECTOR_H
