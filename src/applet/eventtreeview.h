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

#ifndef EVENTTREEVIEW_H
#define EVENTTREEVIEW_H

#include <QTreeView>
#include <QMouseEvent>

class QModelIndex;
class QPoint;

class EventTreeView : public QTreeView
{
    Q_OBJECT

public:
    EventTreeView(QWidget* parent = 0);
    ~EventTreeView();

    QModelIndex indexAtCursor();
    QString summaryAtCursor();

protected:
    void mouseMoveEvent(QMouseEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseDoubleClickEvent(QMouseEvent *event);

signals:
    void tooltipUpdated(QString);

private slots:
    void test();
    private:
    QModelIndex idx;
};

#endif
