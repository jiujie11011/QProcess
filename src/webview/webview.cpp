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
#include "webview.h"
#include "webpage.h"

#include <QApplication>
#include <QDebug>
#include <QWebEngineHistory>
#include <QContextMenuEvent>

WebView::WebView(QWidget *parent)
  : QWebEngineView(parent)
  , buttonClick_(0)
  , isLoading_(false)
  , rssChecked_(false)
  , hasRss_(false)
  , posX_(0)
  , webPage_(nullptr)
{
  setContextMenuPolicy(Qt::CustomContextMenu);
  webPage_ = new WebPage(this);
  setPage(webPage_);
  QPalette pal(qApp->palette());
  pal.setColor(QPalette::Base, Qt::white);
  setPalette(pal);

  connect(this, SIGNAL(loadStarted()), this, SLOT(slotLoadStarted()));
  connect(this, SIGNAL(loadProgress(int)), this, SLOT(slotLoadProgress(int)));
  connect(this, SIGNAL(loadFinished(bool)), this, SLOT(slotLoadFinished()));
  connect(this, SIGNAL(customContextMenuRequested(const QPoint&)),
          this, SLOT(contextMenuRequested(const QPoint&)));
}

void WebView::disconnectObjects()
{
  disconnect(this);
}

/*virtual*/ void WebView::mousePressEvent(QMouseEvent *event)
{
  buttonClick_ = 0;

  if (event->buttons() == Qt::RightButton) {
    posX_ = QEVENT_POS(event).x();
  }

  QWebEngineView::mousePressEvent(event);
}

/*virtual*/ void WebView::mouseReleaseEvent(QMouseEvent *event)
{
  if (event->button() & Qt::RightButton) {
    int posX2 = QEVENT_POS(event).x();
    if (posX_ > posX2+5) {
      if (history()->canGoBack())
        back();
      else
        emit signalGoHome();
    } else if (posX_+5 < posX2) {
      forward();
    } else {
      emit showContextMenu(QEVENT_POS(event));
    }
  } else if (event->button() & Qt::MiddleButton) {
    if (event->modifiers() == Qt::NoModifier) {
      buttonClick_ = MIDDLE_BUTTON;
    } else {
      buttonClick_ = MIDDLE_BUTTON_MOD;
    }
  } else if (event->button() & Qt::LeftButton) {
    if (event->modifiers() == Qt::ControlModifier) {
      buttonClick_ = LEFT_BUTTON_CTRL;
    } else if ((event->modifiers() == Qt::ShiftModifier) ||
               (event->modifiers() == (Qt::ControlModifier | Qt::ShiftModifier))) {
      buttonClick_ = LEFT_BUTTON_SHIFT;
    } else if (event->modifiers() == Qt::AltModifier) {
      buttonClick_ = LEFT_BUTTON_ALT;
    } else {
      buttonClick_ = LEFT_BUTTON;
    }
  }

  QWebEngineView::mouseReleaseEvent(event);
}

/*virtual*/ void WebView::wheelEvent(QWheelEvent *event)
{
  if (event->modifiers() == Qt::ControlModifier) {
    if (event->angleDelta().y() > 0) {
      if (zoomFactor() < 5.0)
        setZoomFactor(zoomFactor()+0.1);
    }
    else {
      if (zoomFactor() > 0.3)
        setZoomFactor(zoomFactor()-0.1);
    }
    event->accept();
    return;
  }
  QWebEngineView::wheelEvent(event);
}

void WebView::contextMenuEvent(QContextMenuEvent *event)
{
  // QContextMenuEvent is not a QSinglePointEvent in Qt6, so it has no
  // position(); pos() works on both Qt5 and Qt6.
  emit showContextMenu(event->pos());
}

void WebView::mouseMoveEvent(QMouseEvent* event)
{
  if (event->buttons() != Qt::LeftButton) {
    QWebEngineView::mouseMoveEvent(event);
    return;
  }

  // WebEngine: use runJavaScript for hit testing
  // For now, skip the drag detection that requires mainFrame()
  QWebEngineView::mouseMoveEvent(event);
}

void WebView::slotLoadStarted()
{
  isLoading_ = true;

  rssChecked_ = false;
  emit rssChanged(false);
}

void WebView::slotLoadProgress(int value)
{
  if (value > 60) {
    checkRss();
  }
}

void WebView::slotLoadFinished()
{
  isLoading_ = false;
}

void WebView::checkRss()
{
  if (rssChecked_) {
    return;
  }

  rssChecked_ = true;
  // WebEngine: use runJavaScript to find RSS links
  page()->runJavaScript(
    "document.querySelectorAll('link[type=\"application/rss+xml\"]').length",
    [this](const QVariant& result) {
      hasRss_ = result.toInt() != 0;
      emit rssChanged(hasRss_);
    });
}

void WebView::contextMenuRequested(const QPoint& pos)
{
  emit showContextMenu(pos);
}