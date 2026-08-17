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
#ifndef JSONFEEDS_H
#define JSONFEEDS_H

#include <QString>
#include <QSqlDatabase>

class FeedsModel;

/*! Export the whole feed tree to a JSON document string.
 *  Preserves hierarchy (parentId), feed metadata and update settings.
 *  Returns empty string on failure. */
QString exportFeedsToJson(FeedsModel *model, QSqlDatabase db);

/*! Import feeds from a JSON document string.
 *  Adds feeds into the database preserving hierarchy. Duplicate feeds
 *  (by lowercase xmlUrl) are skipped. Returns the number of feeds added,
 *  or -1 if the document is invalid. */
int importFeedsFromJson(const QString &jsonData, QSqlDatabase db);

#endif // JSONFEEDS_H
