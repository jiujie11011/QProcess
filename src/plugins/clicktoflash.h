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
#ifndef CLICKTOFLASH_H
#define CLICKTOFLASH_H

#include <QUrl>
#include <QWidget>
#include <QWebEnginePage>

class QToolButton;
class QHBoxLayout;
class QFrame;

class WebPage;

class ClickToFlash : public QObject
{
  Q_OBJECT

public:
  explicit ClickToFlash(WebPage *parentPage, QObject *parent = 0);

  static bool isAlreadyAccepted(const QUrl &url);
  void setUrl(const QUrl &url);
  QUrl url() const;

  static void addWhitelist(const QString &host);
  static bool isWhitelisted(const QString &host);

private slots:
  void load();
  void toWhitelist();

private:
  QUrl url_;
  WebPage* page_;

  static QUrl acceptedUrl;
  static QStringList whitelistHosts;
};

#endif // CLICKTOFLASH_H