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

#include "eventmodel.h"

// kdepim headers
#include <kcal/incidenceformatter.h>
#include <kcal/recurrence.h>

#include <akonadi/collectionfetchjob.h>
#include <akonadi/item.h>
#include <akonadi/itemfetchjob.h>
#include <akonadi/itemfetchscope.h>
#include <akonadi/kcal/incidencemimetypevisitor.h>
#include <akonadi/itemfetchscope.h>
#include <akonadi/entitydisplayattribute.h>


// qt headers
#include <QDate>
#include <QStandardItem>

// kde headers
#include <KIcon>
#include <KGlobal>
#include <KLocale>
#include <KGlobalSettings>

#include <Plasma/Theme>

#include <KDebug>

EventModel::EventModel(QObject *parent, int urgencyTime, QList<QColor> colorList, int days) : QStandardItemModel(parent),
    parentItem(0),
    todayItem(0),
    tomorrowItem(0),
    weekItem(0),
    monthItem(0),
    laterItem(0),
    m_monitor(0)
{
    parentItem = invisibleRootItem();
    settingsChanged(urgencyTime, colorList, days);
    initModel();

    m_monitor = new Akonadi::Monitor(this);
    Akonadi::ItemFetchScope scope;
    scope.fetchFullPayload(true);
    scope.fetchAllAttributes(true);;
    m_monitor->fetchCollection(true);
    m_monitor->setItemFetchScope(scope);
    m_monitor->setCollectionMonitored(Akonadi::Collection::root());
    m_monitor->setMimeTypeMonitored(Akonadi::IncidenceMimeTypeVisitor::eventMimeType(), true);

    connect(m_monitor, SIGNAL(itemAdded(const Akonadi::Item &, const Akonadi::Collection &)),
                       SLOT(eventAdded(const Akonadi::Item &, const Akonadi::Collection &)));
    connect(m_monitor, SIGNAL(itemRemoved(const Akonadi::Item &)),
                       SLOT(eventRemoved(const Akonadi::Item &)));
}

EventModel::~EventModel()
{
}

void EventModel::initModel()
{
    if (!todayItem) {
        todayItem = new QStandardItem();
        initHeaderItem(todayItem, i18n("Today"), i18n("Events of today"), 0);
    }

    if (!tomorrowItem) {
        tomorrowItem = new QStandardItem();
        initHeaderItem(tomorrowItem, i18n("Tomorrow"), i18n("Events for tomrrow"), 1);
    }

    if (!weekItem) {
        weekItem = new QStandardItem();
        initHeaderItem(weekItem, i18n("Week"), i18n("Events of the next week"), 2);
    }

    if (!monthItem) {
        monthItem = new QStandardItem();
        initHeaderItem(monthItem, i18n("Next 4 weeks"), i18n("Events for the next 4 weeks"), 8);
    }

    if (!laterItem) {
        laterItem = new QStandardItem();
        initHeaderItem(laterItem, i18n("Later"), i18n("Events later than 4 weeks"), 29);
    }

    sectionItems << todayItem << tomorrowItem << weekItem << monthItem << laterItem;

//     if (!m_akonadiMonitor)
//         return events;

    Akonadi::CollectionFetchJob *job = new Akonadi::CollectionFetchJob(Akonadi::Collection::root(),
                                                                       Akonadi::CollectionFetchJob::Recursive);
    if (job->exec()) {
        Akonadi::Collection::List collections = job->collections();
        foreach (const Akonadi::Collection &collection, collections) {
            Akonadi::ItemFetchJob *ijob = new Akonadi::ItemFetchJob(collection);
            ijob->fetchScope().fetchFullPayload();

            if (ijob->exec()) {
                Akonadi::Item::List items = ijob->items();
                foreach (const Akonadi::Item &item, items) {
                    if (item.hasPayload <KCal::Event::Ptr>()) {
                        KCal::Event *event = item.payload <KCal::Event::Ptr>().get();
                        if (event) {
                            addEventItem(eventDetails(item, event, collection));
                        } // if event
                    } // if hasPayload
                } // foreach
            }
        }
    }
}

void EventModel::initHeaderItem(QStandardItem *item, QString title, QString toolTip, int days)
{
        item->setData(QVariant(title), Qt::DisplayRole);
        item->setData(QVariant(QDateTime(QDate::currentDate().addDays(days))), EventModel::SortRole);
        item->setData(QVariant(HeaderItem), EventModel::ItemRole);
        item->setData(QVariant(QString()), EventModel::UIDRole);
        item->setData(QVariant("<qt><b>" + toolTip + "</b></qt>"), EventModel::TooltipRole);
        QFont bold = Plasma::Theme::defaultTheme()->font(Plasma::Theme::DefaultFont);
        bold.setBold(true);
        item->setFont(bold);
}

void EventModel::resetModel(bool isRunning)
{
    clear();
    parentItem = invisibleRootItem();
    todayItem = 0;
    tomorrowItem = 0;
    weekItem = 0;
    monthItem = 0;
    laterItem = 0;
    sectionItems.clear();

    if (!isRunning) {
        QStandardItem *errorItem = new QStandardItem();
        errorItem->setData(QVariant(i18n("The Akonadi server is not running!")), Qt::DisplayRole);
        parentItem->appendRow(errorItem);
    } else {
        initModel();
    }
}

void EventModel::settingsChanged(int urgencyTime, QList<QColor> itemColors, int period)
{
    urgency = urgencyTime;
    urgentBg = itemColors.at(urgentColorPos);
    passedFg = itemColors.at(passedColorPos);
    birthdayBg = itemColors.at(birthdayColorPos);
    anniversariesBg = itemColors.at(anniversariesColorPos);
}

void EventModel::eventAdded(const Akonadi::Item &item, const Akonadi::Collection &collection)
{
    kDebug() << "item added" << item.remoteId();
    if (item.hasPayload <KCal::Event::Ptr>()) {
        KCal::Event *event = item.payload <KCal::Event::Ptr>().get();
        if (event) {
            addEventItem(eventDetails(item, event, collection));
        } // if event
    }
}

void EventModel::eventRemoved(const Akonadi::Item &item)
{
    kDebug() << "eventRemoved" << item.remoteId();
    foreach (QStandardItem *i, sectionItems) {
        QModelIndexList l;
        if (i->hasChildren())
            l = match(i->child(0, 0)->index(), EventModel::ItemIDRole, item.remoteId());
        kDebug() << l;
        if (!l.isEmpty())
            i->removeRow(l[0].row());
        int r = i->row();
        if (r != -1 && !i->hasChildren()) {
            takeItem(r);
            removeRow(r);
            emit modelNeedsExpanding();
        }
    }
}

void EventModel::addEventItem(const QMap <QString, QVariant> &values)
{
    QList<QVariant> data;
    if (values["recurs"].toBool()) {
        QList<QVariant> dtTimes = values["recurDates"].toList();
        foreach (QVariant eventDtTime, dtTimes) {
            QStandardItem *eventItem;
            eventItem = new QStandardItem();
            data.insert(StartDtTimePos, eventDtTime);
            data.insert(EndDtTimePos, values["endDate"]);
            data.insert(SummaryPos, values["summary"]);
            data.insert(DescriptionPos, values["description"]);
            data.insert(LocationPos, values["location"]);
            int n = eventDtTime.toDate().year() - values["startDate"].toDate().year();
            data.insert(YearsSincePos, QVariant(QString::number(n)));
            data.insert(resourceNamePos, values["resourceName"]);
            data.insert(BirthdayOrAnniversaryPos, QVariant(values["isBirthday"].toBool() || values["isAnniversary"].toBool()));

            if (values["isBirthday"].toBool()) {
                eventItem->setBackground(QBrush(birthdayBg));
                eventItem->setData(QVariant(BirthdayItem), EventModel::ItemRole);
            } else if (values["isAnniversary"].toBool()) {
                eventItem->setBackground(QBrush(anniversariesBg));
                eventItem->setData(QVariant(AnniversaryItem), EventModel::ItemRole);
            }

            eventItem->setData(data, Qt::DisplayRole);
            eventItem->setData(eventDtTime, EventModel::SortRole);
            eventItem->setData(values["uid"], EventModel::UIDRole);
            eventItem->setData(values["itemid"], ItemIDRole);
            eventItem->setData(values["tooltip"], EventModel::TooltipRole);

            addItemRow(eventDtTime.toDate(), eventItem);
        }
    } else {
            QStandardItem *eventItem;
            eventItem = new QStandardItem();
            data.insert(StartDtTimePos, values["startDate"]);
            data.insert(EndDtTimePos, values["endDate"]);
            data.insert(SummaryPos, values["summary"]);
            data.insert(DescriptionPos, values["description"]);
            data.insert(LocationPos, values["location"]);
            data.insert(YearsSincePos, QVariant(QString()));
            data.insert(resourceNamePos, values["resourceName"]);
            data.insert(BirthdayOrAnniversaryPos, QVariant(FALSE));

            eventItem->setData(QVariant(NormalItem), EventModel::ItemRole);
            eventItem->setData(data, Qt::DisplayRole);
            eventItem->setData(values["startDate"].toDateTime(), EventModel::SortRole);
            eventItem->setData(values["uid"], EventModel::UIDRole);
            eventItem->setData(values["itemid"], ItemIDRole);
            eventItem->setData(values["tooltip"], EventModel::TooltipRole);
            QDateTime itemDtTime = values["startDate"].toDateTime();
            if (itemDtTime > QDateTime::currentDateTime() && QDateTime::currentDateTime().secsTo(itemDtTime) < urgency * 60) {
                eventItem->setBackground(QBrush(urgentBg));
            } else if (QDateTime::currentDateTime() > itemDtTime) {
                eventItem->setForeground(QBrush(passedFg));
            }

            addItemRow(values["startDate"].toDate(), eventItem);
    }
}

void EventModel::addItemRow(QDate eventDate, QStandardItem *item)
{
    if (eventDate == QDate::currentDate()) {
        todayItem->appendRow(item);
        todayItem->sortChildren(0, Qt::AscendingOrder);
        if (todayItem->row() == -1)
            parentItem->insertRow(figureRow(todayItem), todayItem);
    } else if (eventDate > QDate::currentDate() && eventDate <= QDate::currentDate().addDays(1)) {
        tomorrowItem->appendRow(item);
        tomorrowItem->sortChildren(0, Qt::AscendingOrder);
        if (tomorrowItem->row() == -1)
            parentItem->insertRow(figureRow(tomorrowItem), tomorrowItem);
    } else if (eventDate > QDate::currentDate().addDays(1) && eventDate <= QDate::currentDate().addDays(7)) {
        weekItem->appendRow(item);
        weekItem->sortChildren(0, Qt::AscendingOrder);
        if (weekItem->row() == -1)
            parentItem->insertRow(figureRow(weekItem), weekItem);
    } else if (eventDate > QDate::currentDate().addDays(7) && eventDate <= QDate::currentDate().addDays(28)) {
        monthItem->appendRow(item);
        monthItem->sortChildren(0, Qt::AscendingOrder);
        if (monthItem->row() == -1)
            parentItem->insertRow(figureRow(monthItem), monthItem);
    } else if (eventDate > QDate::currentDate().addDays(28) && eventDate <= QDate::currentDate().addDays(365)) {
        laterItem->appendRow(item);
        laterItem->sortChildren(0, Qt::AscendingOrder);
        if (laterItem->row() == -1)
            parentItem->appendRow(laterItem);
    }

    emit modelNeedsExpanding();
}

int EventModel::figureRow(QStandardItem *headerItem)
{
    if (headerItem == todayItem) {
        return 0;
    } else if (headerItem == tomorrowItem) {
        if (todayItem->row() > -1)
            return 1;
        else
            return 0;
    } else if (headerItem == weekItem) {
        if (todayItem->row() > -1 && tomorrowItem->row() > -1)
            return 2;
        else if (todayItem->row() > -1 || tomorrowItem->row() > -1)
            return 1;
        else
            return 0;
    } else if (headerItem == monthItem) {
        if (todayItem->row() > -1 && tomorrowItem->row() > -1 && weekItem->row() > -1)
            return 3;
        else if (weekItem->row() > -1)
            return weekItem->row() + 1;
        else if (todayItem->row() > -1 || tomorrowItem->row() > -1)
            return 1;
        else
            return 0;
    }

    return -1;
}

QMap<QString, QVariant> EventModel::eventDetails(const Akonadi::Item &item, KCal::Event *event, const Akonadi::Collection &collection)
{
    QMap <QString, QVariant> values;
    values["resource"] = collection.resource();
    values["resourceName"] = collection.name();
    values["summary"] = event->summary();
    values["categories"] = event->categories();
    values["status"] = event->status();
    values["startDate"] = event->dtStart().dateTime();
    values["endDate"] = event->dtEnd().dateTime();
    values["uid"] = event->uid();
    values["itemid"] = item.remoteId();
    values["description"] = event->description();
    values["location"] = event->location();
    bool recurs = event->recurs();
    values["recurs"] = recurs;
    QList<QVariant> recurDates;
    if (recurs) {
        KCal::Recurrence *r = event->recurrence();
        KCal::DateTimeList dtTimes = r->timesInInterval(KDateTime(QDate::currentDate()), KDateTime(QDate::currentDate()).addDays(366));
        dtTimes.sortUnique();
        foreach (KDateTime t, dtTimes) {
            recurDates << QVariant(t.dateTime());
        }
    }
    values["recurDates"] = recurDates;
    event->customProperty("KABC", "BIRTHDAY") == QString("YES") ? values ["isBirthday"] = QVariant(TRUE) : QVariant(FALSE);
    event->customProperty("KABC", "ANNIVERSARY") == QString("YES") ? values ["isAnniversary"] = QVariant(TRUE) : QVariant(FALSE);
#if KDE_IS_VERSION(4,4,60)
    values["tooltip"] = KCal::IncidenceFormatter::toolTipStr(resource, event);
#else
    values["tooltip"] = KCal::IncidenceFormatter::toolTipStr(event);
#endif
    return values;
}

#include "eventmodel.moc"
