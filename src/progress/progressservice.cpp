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
#include "progressservice.h"

#include "database.h"

#include <QDateTime>
#include <QSqlQuery>
#include <QVariant>

ProgressService::ProgressService(QObject *parent)
  : QObject(parent)
  , feedId_(0)
  , newsId_(0)
  , scrollPos_(0)
  , contextChanged_(false)
{
  db_ = Database::connection("secondConnection");

  timer_.setSingleShot(true);
  timer_.setInterval(5000);
  connect(&timer_, SIGNAL(timeout()), this, SLOT(saveProgress()));
}

void ProgressService::updateContext(int feedId, int newsId, int scrollPos)
{
  if (feedId == feedId_ && newsId == newsId_ && scrollPos == scrollPos_)
    return;

  feedId_ = feedId;
  newsId_ = newsId;
  scrollPos_ = scrollPos;
  contextChanged_ = true;

  if (!timer_.isActive())
    timer_.start();
}

void ProgressService::updateScrollPos(int scrollPos)
{
  if (scrollPos == scrollPos_)
    return;

  scrollPos_ = scrollPos;
  contextChanged_ = true;

  if (!timer_.isActive())
    timer_.start();
}

void ProgressService::setContext(int feedId, int newsId, int scrollPos)
{
  feedId_ = feedId;
  newsId_ = newsId;
  scrollPos_ = scrollPos;
  contextChanged_ = true;
}

void ProgressService::saveProgress()
{
  if (!contextChanged_ || newsId_ <= 0)
    return;

  QSqlQuery q(db_);
  q.prepare("UPDATE news SET scrollPos = ?, lastReadTime = ? WHERE id = ?");
  q.addBindValue(scrollPos_);
  q.addBindValue(QDateTime::currentDateTime().toString(Qt::ISODate));
  q.addBindValue(newsId_);
  q.exec();

  contextChanged_ = false;
}

void ProgressService::flush()
{
  saveProgress();
}

void ProgressService::sync()
{
  // Placeholder for future progress cloud sync (reserved interface).
}
