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
#ifndef WEB_PLUGIN_FACTORY_H
#define WEB_PLUGIN_FACTORY_H

#include <QWebEngineUrlRequestInterceptor>
#include <QHash>

class WebPluginFactory : public QWebEngineUrlRequestInterceptor
{
  Q_OBJECT
public:
  explicit WebPluginFactory(QObject *parent = nullptr);

  void interceptRequest(QWebEngineUrlRequestInfo &info) override;

private:
  // Smart Referer cache: image host -> Referer header to use.
  QHash<QString, QByteArray> refererCache_;
};

#endif // WEB_PLUGIN_FACTORY_H