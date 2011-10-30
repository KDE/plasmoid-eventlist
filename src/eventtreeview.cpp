/*
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>
 *
 *   Copyright (C) 2009 by gerdfleischer <gerdfleischer@web.de>
 */

#include "eventtreeview.h"
#include "eventmodel.h"

#include <QModelIndex>
#include <QMouseEvent>

EventTreeView::EventTreeView(QWidget* parent)
    : QTreeView(parent)
{
    setAttribute(Qt::WA_NoSystemBackground);
    setAutoFillBackground(false);
    setMouseTracking(true);
    setAlternatingRowColors(true);
    setHeaderHidden(true);
    setRootIsDecorated(false);
    setAnimated(true);
    setWordWrap(true);
    setFrameShape(QFrame::NoFrame);
    setSelectionMode(QAbstractItemView::NoSelection);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setEditTriggers(QAbstractItemView::NoEditTriggers);
}

EventTreeView::~EventTreeView()
{
}

void EventTreeView::mouseMoveEvent(QMouseEvent *event)
{
    QString oldTip = tip;

    idx = indexAt(event->pos());
    if (idx.isValid()) {
        tip = idx.data(EventModel::TooltipRole).toString();
    } else {
        tip.clear();
    }
    
    if (tip != oldTip)
        emit tooltipUpdated(tip);
}

void EventTreeView::mousePressEvent(QMouseEvent *event)
{
    QString oldTip = tip;

    idx = indexAt(event->pos());
    if (idx.isValid()) {
        tip = idx.data(EventModel::TooltipRole).toString();
    } else {
        tip.clear();
    }

    if (tip != oldTip)
        emit tooltipUpdated(tip);
}

QModelIndex EventTreeView::indexAtCursor()
{
    return idx;
}

QString EventTreeView::summaryAtCursor()
{
    const QVariant v = idx.data(Qt::DisplayRole);
    QMap<QString, QVariant> values = v.toMap();
    return values["summary"].toString();
}

#include "eventtreeview.moc"
