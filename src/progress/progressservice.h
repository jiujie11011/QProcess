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
#ifndef PROGRESSSERVICE_H
#define PROGRESSSERVICE_H

#include <QObject>
#include <QTimer>
#include <QSqlDatabase>

/** @brief Autosaves reading progress (scroll position, read state, last read
 * time) for the currently active news item.
 *
 * The progress is flushed periodically (every 5 s) while the context changes,
 * immediately on tab switch / window deactivation, and on application exit.
 */
class ProgressService : public QObject
{
  Q_OBJECT

public:
  explicit ProgressService(QObject *parent = 0);

  /** @brief Record the currently displayed news item and its scroll offset.
   * Starts or restarts the periodic autosave timer.
   */
  void updateContext(int feedId, int newsId, int scrollPos);

  /** @brief Update only the scroll offset. Keeps any active autosave timer
   * running (a continuously scrolling user does not keep resetting the save
   * deadline); starts the timer when idle.
   */
  void updateScrollPos(int scrollPos);

  /** @brief Record the current news item without restarting the timer. */
  void setContext(int feedId, int newsId, int scrollPos);

  /** @brief Write the current context to the database immediately. */
  void flush();

  /** @brief Placeholder for future progress cloud sync.
   *
   * Reserved for a later iteration (e.g. upload to a sync server and restore
   * on another device). Currently a no-op; keeps the interface stable.
   */
  void sync();

  /** @brief Current news id (0 when none). */
  int currentNewsId() const { return newsId_; }

  /** @brief Current feed id (0 when none). */
  int currentFeedId() const { return feedId_; }

private slots:
  void saveProgress();

private:
  QSqlDatabase db_;
  QTimer timer_;
  int feedId_;
  int newsId_;
  int scrollPos_;
  bool contextChanged_;
};

#endif // PROGRESSSERVICE_H
