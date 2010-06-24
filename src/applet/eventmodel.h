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
 *   Copyright (C) 2008 by Javier Goday <jgoday@gmail.com>
 *   Copyright (C) 2009 by gerdfleischer <gerdfleischer@web.de>
 */

#ifndef EVENTMODEL_H
#define EVENTMODEL_H

#include <akonadi/monitor.h>
#include <akonadi/collection.h>

#include <kcal/event.h>
#include <kcal/todo.h>

// qt headers
#include <QStandardItemModel>
#include <QColor>
#include <QHash>
#include <QString>

class QStandardItem;

static const int ShortDateFormat = 0;
static const int LongDateFormat = 1;
static const int CustomDateFormat = 2;

static const int urgentColorPos = 0;
static const int passedColorPos = 1;
static const int birthdayColorPos = 2;
static const int anniversariesColorPos = 3;
static const int todoColorPos = 4;

/**
* Model of the view
* Categorizes the events using the startDate property
*/
class EventModel : public QStandardItemModel
{
    Q_OBJECT
public:
    enum EventRole {
        SortRole = Qt::UserRole + 1,
        UIDRole,
        ItemTypeRole,
        TooltipRole,
        ItemIDRole,
        ResourceRole
    };

    enum ItemType {
        HeaderItem = 0,
        NormalItem,
        BirthdayItem,
        AnniversaryItem,
        TodoItem
    };

    EventModel(QObject *parent = 0, int urgencyTime = 15, QList<QColor> colorList = QList<QColor>());
    ~EventModel();

public:
    void setDateFormat(int format, QString string);
    void initModel();
    void initMonitor();
    void resetModel();
    void settingsChanged(int urgencyTime, QList<QColor> itemColors);

private slots:
    void addEventItem(const QMap <QString, QVariant> &values);
    void addTodoItem(const QMap <QString, QVariant> &values);
    void itemAdded(const Akonadi::Item &, const Akonadi::Collection &);
    void removeItem(const Akonadi::Item &);
    void itemChanged(const Akonadi::Item &, const QSet<QByteArray> &);
    void itemMoved(const Akonadi::Item &, const Akonadi::Collection &, const Akonadi::Collection &);

private:
    void initHeaderItem(QStandardItem *item, QString title, QString toolTip, int days);
    void addItem(const Akonadi::Item &item, const Akonadi::Collection &collection);
    void addItemRow(QDate eventDate, QStandardItem *items);
    void addItemToHeader(QStandardItem *headerItem, QStandardItem *item);
    int figureRow(QStandardItem *headerItem);
    QMap<QString, QVariant> eventDetails(const Akonadi::Item &, KCal::Event *, const Akonadi::Collection &);
    QMap<QString, QVariant> todoDetails(const Akonadi::Item &, KCal::Todo *, const Akonadi::Collection &);

private:
    QStandardItem *parentItem, *todayItem, *tomorrowItem, *weekItem, *monthItem, *laterItem, *somedayItem;
    QList<QStandardItem *> sectionItems;
    int urgency;
    QColor urgentBg, passedFg, birthdayBg, anniversariesBg, todoBg;
    Akonadi::Monitor *m_monitor;

signals:
    void modelNeedsExpanding();
};

#endif
