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
#include "scriptfeedrunner.h"

#include <QFileInfo>
#include <QTextStream>

ScriptFeedRunner::ScriptFeedRunner(QObject *parent)
  : QObject(parent),
    timedOut_(false)
{
}

/*! Resolve the interpreter for a script by its file extension. */
static QString interpreterFor(const QString &scriptPath)
{
  QString ext = QFileInfo(scriptPath).suffix().toLower();
  if (ext == "py")  return QStringLiteral("python3");
  if (ext == "py2") return QStringLiteral("python2");
  if (ext == "js")  return QStringLiteral("node");
  if (ext == "rb")  return QStringLiteral("ruby");
  if (ext == "pl")  return QStringLiteral("perl");
  if (ext == "php") return QStringLiteral("php");
  if (ext == "sh")  return QStringLiteral("/bin/sh");
  return QString();
}

QByteArray ScriptFeedRunner::run(const QString &scriptPath,
                                 const QByteArray &payload, int timeoutMs)
{
  timedOut_ = false;
  lastError_.clear();

  QFileInfo fi(scriptPath);
  if (!fi.exists() || !fi.isFile()) {
    lastError_ = QStringLiteral("Script not found: %1").arg(scriptPath);
    return QByteArray();
  }
  if (!fi.isReadable()) {
    lastError_ = QStringLiteral("Script is not readable: %1").arg(scriptPath);
    return QByteArray();
  }

  QString interpreter = interpreterFor(scriptPath);
  if (interpreter.isEmpty()) {
    lastError_ = QStringLiteral("Unsupported script type: %1").arg(scriptPath);
    return QByteArray();
  }

  QProcess process(this);
  process.setProcessChannelMode(QProcess::SeparateChannels);

  QStringList args;
  if (interpreter == "/bin/sh") {
    args << scriptPath;
  } else {
    args << scriptPath;
  }

  process.start(interpreter, args);
  if (!process.waitForStarted(5000)) {
    lastError_ = QStringLiteral("Failed to start script: %1").
        arg(process.errorString());
    return QByteArray();
  }

  // Feed the payload (the raw feed HTML/XML) to the script on stdin.
  process.write(payload);
  process.closeWriteChannel();

  if (!process.waitForFinished(timeoutMs)) {
    process.kill();
    process.waitForFinished(3000);
    timedOut_ = true;
    lastError_ = QStringLiteral("Script timed out after %1 ms: %2")
        .arg(timeoutMs).arg(scriptPath);
    return QByteArray();
  }

  if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
    QString stderrText = QString::fromUtf8(process.readAllStandardError()).trimmed();
    lastError_ = QStringLiteral("Script error (exit %1): %2")
        .arg(process.exitCode())
        .arg(stderrText.isEmpty() ? scriptPath : stderrText);
    return QByteArray();
  }

  QByteArray out = process.readAllStandardOutput();
  if (out.trimmed().isEmpty()) {
    lastError_ = QStringLiteral("Script produced no output: %1").arg(scriptPath);
    return QByteArray();
  }
  return out;
}
