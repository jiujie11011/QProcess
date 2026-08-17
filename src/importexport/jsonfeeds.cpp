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
#include "jsonfeeds.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSqlQuery>
#include <QDateTime>
#include <QTreeView>

#include "feedsmodel.h"

// ----------------------------------------------------------------------------
QString exportFeedsToJson(FeedsModel *model, QSqlDatabase db)
{
  Q_UNUSED(db);
  if (!model) return QString();

  // Walk the whole tree using a copy of the model as the OPML exporter does
  FeedsModel exportTreeModel(NULL);
  QTreeView exportTreeView;
  exportTreeView.setModel(&exportTreeModel);
  exportTreeModel.setView(model->view());

  QJsonArray feedArray;
  QModelIndex index = exportTreeModel.index(0, 0);
  while (index.isValid()) {
    QJsonObject obj;
    obj.insert("id", exportTreeModel.idByIndex(index));
    obj.insert("text", exportTreeModel.dataField(index, "text").toString());
    obj.insert("title", exportTreeModel.dataField(index, "title").toString());
    obj.insert("htmlUrl", exportTreeModel.dataField(index, "htmlUrl").toString());
    obj.insert("xmlUrl", exportTreeModel.dataField(index, "xmlUrl").toString());
    obj.insert("parentId", exportTreeModel.paridByIndex(index));
    obj.insert("isFolder", exportTreeModel.isFolder(index));

    if (!exportTreeModel.isFolder(index)) {
      obj.insert("language", exportTreeModel.dataField(index, "language").toString());
      obj.insert("description", exportTreeModel.dataField(index, "description").toString());
      obj.insert("updateIntervalEnable",
                 exportTreeModel.dataField(index, "updateIntervalEnable").toInt());
      obj.insert("updateInterval",
                 exportTreeModel.dataField(index, "updateInterval").toInt());
      obj.insert("updateIntervalType",
                 exportTreeModel.dataField(index, "updateIntervalType").toString());
      obj.insert("updateOnStartup",
                 exportTreeModel.dataField(index, "updateOnStartup").toInt());
      obj.insert("proxyEnabled",
                 exportTreeModel.dataField(index, "proxyEnabled").toInt());
      obj.insert("proxyURL",
                 exportTreeModel.dataField(index, "proxyURL").toString());
    }
    feedArray.append(obj);
    index = exportTreeView.indexBelow(index);
  }

  QJsonObject root;
  root.insert("type", "quill-feeds");
  root.insert("version", 1);
  root.insert("exported", QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
  root.insert("feeds", feedArray);

  return QString::fromUtf8(QJsonDocument(root).toJson(QJsonDocument::Indented));
}

// ----------------------------------------------------------------------------
int importFeedsFromJson(const QString &jsonData, QSqlDatabase db)
{
  if (jsonData.isEmpty()) return -1;

  QJsonParseError parseError;
  QJsonDocument doc = QJsonDocument::fromJson(jsonData.toUtf8(), &parseError);
  if (parseError.error != QJsonParseError::NoError) return -1;

  QJsonObject root = doc.object();
  if (root.value("type").toString() != "quill-feeds") return -1;

  QJsonArray feedArray = root.value("feeds").toArray();
  if (feedArray.isEmpty()) return 0;

  int addedCount = 0;
  db.transaction();

  QSqlQuery q(db);
  QHash<int, int> idMap; // old id -> new id

  for (int i = 0; i < feedArray.count(); i++) {
    QJsonObject obj = feedArray.at(i).toObject();
    bool isFolder = obj.value("isFolder").toBool();
    int oldId = obj.value("id").toInt();
    int parentId = obj.value("parentId").toInt();

    int newParentId = idMap.value(parentId, 0);
    QString textString = obj.value("text").toString();
    if (textString.isEmpty()) textString = obj.value("title").toString();
    if (textString.isEmpty()) textString = QStringLiteral("(no title)");

    // For feeds - skip duplicates by xmlUrl
    bool isFeedDuplicated = false;
    QString xmlUrlString = obj.value("xmlUrl").toString();
    if (!isFolder && !xmlUrlString.isEmpty()) {
      q.prepare("SELECT id FROM feeds WHERE xmlUrl = ?");
      q.addBindValue(xmlUrlString);
      q.exec();
      if (q.next()) isFeedDuplicated = true;
    }
    if (isFeedDuplicated) continue;

    int rowToParent = 0;
    q.exec(QString("SELECT count(id) FROM feeds WHERE parentId='%1'").
           arg(newParentId));
    if (q.next()) rowToParent = q.value(0).toInt();

    if (isFolder) {
      q.prepare("INSERT INTO feeds(text, title, xmlUrl, created, f_Expanded, parentId, rowToParent) "
                "VALUES(?, ?, '', ?, 0, ?, ?)");
      q.addBindValue(textString);
      q.addBindValue(textString);
      q.addBindValue(QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
      q.addBindValue(newParentId);
      q.addBindValue(rowToParent);
      q.exec();
    } else {
      if (xmlUrlString.isEmpty()) continue;

      q.prepare("INSERT INTO feeds(text, title, description, xmlUrl, htmlUrl, created, "
                "parentId, rowToParent, updateIntervalEnable, updateInterval, "
                "updateIntervalType, updateOnStartup, proxyEnabled, proxyURL) "
                "VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
      q.addBindValue(textString);
      q.addBindValue(obj.value("title").toString());
      q.addBindValue(obj.value("description").toString());
      q.addBindValue(xmlUrlString);
      q.addBindValue(obj.value("htmlUrl").toString());
      q.addBindValue(QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
      q.addBindValue(newParentId);
      q.addBindValue(rowToParent);
      q.addBindValue(obj.value("updateIntervalEnable").toInt());
      q.addBindValue(obj.value("updateInterval").toInt());
      q.addBindValue(obj.value("updateIntervalType").toString());
      q.addBindValue(obj.value("updateOnStartup").toInt());
      q.addBindValue(obj.value("proxyEnabled").toInt());
      q.addBindValue(obj.value("proxyURL").toString());
      q.exec();
    }

    idMap.insert(oldId, q.lastInsertId().toInt());
    addedCount++;
  }

  db.commit();
  return addedCount;
}
