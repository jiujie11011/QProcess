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
#ifndef SCRIPTFEEDRUNNER_H
#define SCRIPTFEEDRUNNER_H

#include <QObject>
#include <QProcess>
#include <QByteArray>
#include <QString>

/*! Run a user-supplied feed script and capture its stdout as feed XML.
 *
 *  The script is invoked directly (no shell), so no command injection is
 *  possible. The feed HTML/XML payload is passed to the script on stdin;
 *  the script writes the RSS/Atom XML to stdout. Execution is bounded by a
 *  timeout (default 30s) after which the process is killed.
 *
 *  Security: the script path comes from the user's own feed configuration
 *  (feeds.fetchScript). Only the interpreter chosen from the file
 *  extension is used; arguments are passed as separate argv entries.
 */
class ScriptFeedRunner : public QObject
{
  Q_OBJECT
public:
  explicit ScriptFeedRunner(QObject *parent = 0);

  /*! Run scriptPath feeding it the raw payload on stdin.
   *  Returns the script stdout on success, empty QByteArray on failure
   *  (error available via lastError() / timedOut()). */
  QByteArray run(const QString &scriptPath, const QByteArray &payload,
                 int timeoutMs = 30000);

  bool timedOut() const { return timedOut_; }
  QString lastError() const { return lastError_; }

private:
  bool timedOut_;
  QString lastError_;
};

#endif // SCRIPTFEEDRUNNER_H
