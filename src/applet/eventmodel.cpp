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
#include <akonadi/itemfetchscope.h>
#include <akonadi/entitydisplayattribute.h>
#include <akonadi/servermanager.h>


// qt headers
#include <QDate>
#include <QStandardItem>

// kde headers
#include <KIcon>
#include <KGlobal>
#include <KLocale>
#include <KGlobalSettings>
#include <KDateTime>

#include <Plasma/Theme>

#include <KDebug>

#if KDE_IS_VERSION(4,4,0)
#include <akonadi/kcal/incidencemimetypevisitor.h>
#endif

EventModel::EventModel(QObject *parent, int urgencyTime, int birthdayTime, QList<QColor> colorList) : QStandardItemModel(parent),
    parentItem(0),
    olderItem(0),
    todayItem(0),
    tomorrowItem(0),
    weekItem(0),
    monthItem(0),
    laterItem(0),
    somedayItem(0),
    m_monitor(0)
{
    parentItem = invisibleRootItem();
    settingsChanged(urgencyTime, birthdayTime, colorList);
    initModel();
    initMonitor();
}

EventModel::~EventModel()
{
}

void EventModel::initModel()
{
    if (!olderItem) {
        olderItem = new QStandardItem();
        initHeaderItem(olderItem, i18n("Earlier stuff"), i18n("Unfinished todos, still ongoing earlier events etc."), -28);
    }

    if (!todayItem) {
        todayItem = new QStandardItem();
        initHeaderItem(todayItem, i18n("Today"), i18n("Events of today"), 0);
    }

    if (!tomorrowItem) {
        tomorrowItem = new QStandardItem();
        initHeaderItem(tomorrowItem, i18n("Tomorrow"), i18n("Events for tomorrow"), 1);
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

    if (!somedayItem) {
        somedayItem = new QStandardItem();
        initHeaderItem(somedayItem, i18n("Some day"), i18n("Todos with no due date"), 366);
    }

    // this list should always be sorted by date
    sectionItems << olderItem << todayItem << tomorrowItem << weekItem << monthItem << laterItem << somedayItem;

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
                    } else if (item.hasPayload <KCal::Todo::Ptr>()) {
                        KCal::Todo *todo = item.payload<KCal::Todo::Ptr>().get();
                        if (todo) {
                            addTodoItem(todoDetails(item, todo, collection));
                        }
                    } // if hasPayload
                } // foreach
            }
        }
    }
}

void EventModel::initMonitor()
{
    m_monitor = new Akonadi::Monitor(this);
    Akonadi::ItemFetchScope scope;
    scope.fetchFullPayload(true);
    scope.fetchAllAttributes(true);
    m_monitor->fetchCollection(true);
    m_monitor->setItemFetchScope(scope);
    m_monitor->setCollectionMonitored(Akonadi::Collection::root());
#if KDE_IS_VERSION(4,4,0)
    m_monitor->setMimeTypeMonitored(Akonadi::IncidenceMimeTypeVisitor::eventMimeType(), true);
    m_monitor->setMimeTypeMonitored(Akonadi::IncidenceMimeTypeVisitor::todoMimeType(), true);
#else
    m_monitor->setMimeTypeMonitored("application/x-vnd.akonadi.calendar.event", true);
    m_monitor->setMimeTypeMonitored("application/x-vnd.akonadi.calendar.todo", true);
#endif

    connect(m_monitor, SIGNAL(itemAdded(const Akonadi::Item &, const Akonadi::Collection &)),
                       SLOT(itemAdded(const Akonadi::Item &, const Akonadi::Collection &)));
    connect(m_monitor, SIGNAL(itemRemoved(const Akonadi::Item &)),
                       SLOT(removeItem(const Akonadi::Item &)));
    connect(m_monitor, SIGNAL(itemChanged(const Akonadi::Item &, const QSet<QByteArray> &)),
                       SLOT(itemChanged(const Akonadi::Item &, const QSet<QByteArray> &)));
    connect(m_monitor, SIGNAL(itemMoved(const Akonadi::Item &, const Akonadi::Collection &, const Akonadi::Collection &)),
                       SLOT(itemMoved(const Akonadi::Item &, const Akonadi::Collection &, const Akonadi::Collection &)));
}

void EventModel::initHeaderItem(QStandardItem *item, QString title, QString toolTip, int days)
{
    QMap<QString, QVariant> data;
    data["itemType"] = HeaderItem;
    data["title"] = title;
    item->setData(data, Qt::DisplayRole);
    item->setData(QVariant(QDateTime(QDate::currentDate().addDays(days))), SortRole);
    item->setData(QVariant(HeaderItem), ItemTypeRole);
    item->setData(QVariant(QString()), ResourceRole);
    item->setData(QVariant(QString()), UIDRole);
    item->setData(QVariant("<qt><b>" + toolTip + "</b></qt>"), TooltipRole);
    QFont bold = Plasma::Theme::defaultTheme()->font(Plasma::Theme::DefaultFont);
    bold.setBold(true);
    item->setFont(bold);
}

void EventModel::resetModel()
{
    clear();
    parentItem = invisibleRootItem();
    olderItem = 0;
    todayItem = 0;
    tomorrowItem = 0;
    weekItem = 0;
    monthItem = 0;
    laterItem = 0;
    somedayItem = 0;
    sectionItems.clear();
    delete m_monitor;
    m_monitor = 0;

    if (!Akonadi::ServerManager::isRunning()) {
        QStandardItem *errorItem = new QStandardItem();
        errorItem->setData(QVariant(i18n("The Akonadi server is not running!")), Qt::DisplayRole);
        parentItem->appendRow(errorItem);
    } else {
        initModel();
        initMonitor();
    }
}

void EventModel::settingsChanged(int urgencyTime, int birthdayTime, QList<QColor> itemColors)
{
    urgency = urgencyTime;
    birthdayUrgency = birthdayTime;
    urgentBg = itemColors.at(urgentColorPos);
    passedFg = itemColors.at(passedColorPos);
    birthdayBg = itemColors.at(birthdayColorPos);
    anniversariesBg = itemColors.at(anniversariesColorPos);
    todoBg = itemColors.at(todoColorPos);
    finishedTodoBg = itemColors.at(finishedTodoColorPos);
}

void EventModel::itemAdded(const Akonadi::Item &item, const Akonadi::Collection &collection)
{
#if KDE_IS_VERSION(4,5,3)
    kDebug() << "item added" << item.remoteId();
#else
    addItem(item, collection);
#endif
}

void EventModel::removeItem(const Akonadi::Item &item)
{
    foreach (QStandardItem *i, sectionItems) {
        QModelIndexList l;
        if (i->hasChildren())
            l = match(i->child(0, 0)->index(), EventModel::ItemIDRole, item.remoteId());
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

void EventModel::itemChanged(const Akonadi::Item &item, const QSet<QByteArray> &)
{
    kDebug() << "item changed";
#if KDE_IS_VERSION(4,5,3)
    removeItem(item);
    addItem(item, item.parentCollection());
#else
    Q_UNUSED(item);
#endif
}

void EventModel::itemMoved(const Akonadi::Item &item, const Akonadi::Collection &, const Akonadi::Collection &)
{
    kDebug() << "item moved";
#if KDE_IS_VERSION(4,5,3)
    removeItem(item);
    addItem(item, item.parentCollection());
#else
    Q_UNUSED(item);
#endif
}

void EventModel::addItem(const Akonadi::Item &item, const Akonadi::Collection &collection)
{
    if (item.hasPayload<KCal::Event::Ptr>()) {
        KCal::Event *event = item.payload <KCal::Event::Ptr>().get();
        if (event) {
            addEventItem(eventDetails(item, event, collection));
        } // if event
    } else if (item.hasPayload <KCal::Todo::Ptr>()) {
        KCal::Todo *todo = item.payload<KCal::Todo::Ptr>().get();
        if (todo) {
            addTodoItem(todoDetails(item, todo, collection));
        }
    }
}

void EventModel::addEventItem(const QMap<QString, QVariant> &values)
{
    QMap<QString, QVariant> data = values;
    if (values["recurs"].toBool()) {
        QList<QVariant> dtTimes = values["recurDates"].toList();
        foreach (QVariant eventDtTime, dtTimes) {
            QStandardItem *eventItem;
            eventItem = new QStandardItem();
            data["startDate"] = eventDtTime;

            QDate itemDt = eventDtTime.toDate();
            if (values["isBirthday"].toBool()) {
                data["itemType"] = BirthdayItem;
                int n = eventDtTime.toDate().year() - values["startDate"].toDate().year();
                data["yearsSince"] = QString::number(n);
                if (itemDt >= QDate::currentDate() && QDate::currentDate().daysTo(itemDt) < birthdayUrgency) {
                    eventItem->setBackground(QBrush(urgentBg));
                } else {
                    eventItem->setBackground(QBrush(birthdayBg));
                }
                eventItem->setData(QVariant(BirthdayItem), ItemTypeRole);
            } else if (values["isAnniversary"].toBool()) {
                data["itemType"] = AnniversaryItem;
                int n = eventDtTime.toDate().year() - values["startDate"].toDate().year();
                data["yearsSince"] = QString::number(n);
                if (itemDt >= QDate::currentDate() && QDate::currentDate().daysTo(itemDt) < birthdayUrgency) {
                    eventItem->setBackground(QBrush(urgentBg));
                } else {
                    eventItem->setBackground(QBrush(anniversariesBg));
                }
                eventItem->setData(QVariant(AnniversaryItem), ItemTypeRole);
            } else {
                data["itemType"] = NormalItem;
                eventItem->setData(QVariant(NormalItem), ItemTypeRole);
                QDateTime itemDtTime = data["startDate"].toDateTime();
                if (itemDtTime > QDateTime::currentDateTime() && QDateTime::currentDateTime().secsTo(itemDtTime) < urgency * 60) {
                    eventItem->setBackground(QBrush(urgentBg));
                } else if (QDateTime::currentDateTime() > itemDtTime) {
                    eventItem->setForeground(QBrush(passedFg));
                }
            }

            eventItem->setData(data, Qt::DisplayRole);
            eventItem->setData(eventDtTime, SortRole);
            eventItem->setData(values["uid"], UIDRole);
            eventItem->setData(values["itemid"], ItemIDRole);
            eventItem->setData(values["resource"], ResourceRole);
            eventItem->setData(values["tooltip"], TooltipRole);

            addItemRow(eventDtTime.toDate(), eventItem);
        }
    } else {
        QStandardItem *eventItem;
        eventItem = new QStandardItem();
        data["itemType"] = NormalItem;
        eventItem->setData(QVariant(NormalItem), ItemTypeRole);
        eventItem->setData(data, Qt::DisplayRole);
        eventItem->setData(values["startDate"].toDateTime(), SortRole);
        eventItem->setData(values["uid"], EventModel::UIDRole);
        eventItem->setData(values["itemid"], ItemIDRole);
        eventItem->setData(values["resource"], ResourceRole);
        eventItem->setData(values["tooltip"], TooltipRole);
        QDateTime itemDtTime = values["startDate"].toDateTime();
        if (itemDtTime > QDateTime::currentDateTime() && QDateTime::currentDateTime().secsTo(itemDtTime) < urgency * 60) {
            eventItem->setBackground(QBrush(urgentBg));
        } else if (QDateTime::currentDateTime() > itemDtTime) {
            eventItem->setForeground(QBrush(passedFg));
        }

        addItemRow(values["startDate"].toDate(), eventItem);
    }
}

void EventModel::addTodoItem(const QMap <QString, QVariant> &values)
{
    QMap<QString, QVariant> data = values;
    QStandardItem *todoItem = new QStandardItem();
    data["itemType"] = TodoItem;
    todoItem->setData(QVariant(TodoItem), ItemTypeRole);
    todoItem->setData(data, Qt::DisplayRole);
    todoItem->setData(values["dueDate"].toDateTime(), SortRole);
    todoItem->setData(values["uid"], EventModel::UIDRole);
    todoItem->setData(values["itemid"], ItemIDRole);
    todoItem->setData(values["resource"], ResourceRole);
    todoItem->setData(values["tooltip"], TooltipRole);
    if (values["completed"].toBool() == TRUE) {
        todoItem->setBackground(QBrush(finishedTodoBg));
    } else {
        todoItem->setBackground(QBrush(todoBg));
    }
    
    addItemRow(values["dueDate"].toDate(), todoItem);
}

void EventModel::addItemRow(QDate eventDate, QStandardItem *incidenceItem)
{
    QStandardItem *headerItem = 0;

    foreach (QStandardItem *item, sectionItems) {
        if (eventDate < item->data(SortRole).toDate())
            break;
        else
            headerItem = item;
    }

    if (headerItem) {
        headerItem->appendRow(incidenceItem);
        headerItem->sortChildren(0, Qt::AscendingOrder);
        if (headerItem->row() == -1) {
            parentItem->insertRow(figureRow(headerItem), headerItem);
        }
        
        emit modelNeedsExpanding();
    }
}

int EventModel::figureRow(QStandardItem *headerItem)
{
    int row = 0;

    foreach (QStandardItem *item, sectionItems) {
        if (item->data(SortRole).toDateTime() >= headerItem->data(SortRole).toDateTime())
            break;
        if (item->row() != -1)
            row = item->row() + 1;
    }

    return row;
}

QMap<QString, QVariant> EventModel::eventDetails(const Akonadi::Item &item, KCal::Event *event, const Akonadi::Collection &collection)
{
    QMap <QString, QVariant> values;
    values["resource"] = collection.resource();
    values["resourceName"] = collection.name();
    values["uid"] = event->uid();
    values["itemid"] = item.remoteId();
    values["summary"] = event->summary();
    values["description"] = event->description();
    values["location"] = event->location();
    values["categories"] = event->categoriesStr();
    values["status"] = event->status();
    values["startDate"] = event->dtStart().dateTime().toLocalTime();
    values["endDate"] = event->dtEnd().dateTime().toLocalTime();
    bool recurs = event->recurs();
    values["recurs"] = recurs;
    QList<QVariant> recurDates;
    if (recurs) {
        KCal::Recurrence *r = event->recurrence();
        KCal::DateTimeList dtTimes = r->timesInInterval(KDateTime(QDate::currentDate()), KDateTime(QDate::currentDate()).addDays(365));
        dtTimes.sortUnique();
        foreach (KDateTime t, dtTimes) {
            recurDates << QVariant(t.dateTime().toLocalTime());
        }
    }
    values["recurDates"] = recurDates;
    event->customProperty("KABC", "BIRTHDAY") == QString("YES") ? values ["isBirthday"] = QVariant(TRUE) : QVariant(FALSE);
    event->customProperty("KABC", "ANNIVERSARY") == QString("YES") ? values ["isAnniversary"] = QVariant(TRUE) : QVariant(FALSE);
#if KDE_IS_VERSION(4,4,60)
    values["tooltip"] = KCal::IncidenceFormatter::toolTipStr(collection.resource(), event, event->dtStart().date(), TRUE, KDateTime::Spec::LocalZone());
#else
    values["tooltip"] = KCal::IncidenceFormatter::toolTipStr(event, TRUE, KDateTime::Spec::LocalZone());
#endif

    return values;
}

QMap<QString, QVariant> EventModel::todoDetails(const Akonadi::Item &item, KCal::Todo *todo, const Akonadi::Collection &collection)
{
    QMap <QString, QVariant> values;
    values["resource"] = collection.resource();
    values["resourceName"] = collection.name();
    values["uid"] = todo->uid();
    values["itemid"] = item.remoteId();
    values["summary"] = todo->summary();
    values["description"] = todo->description();
    values["location"] = todo->location();
    values["categories"] = todo->categoriesStr();
    values["completed"] = todo->isCompleted();
    values["percent"] = todo->percentComplete();
    if (todo->hasStartDate()) {
        values["startDate"] = todo->dtStart(FALSE).dateTime().toLocalTime();
        values["hasStartDate"] = TRUE;
    } else {
        values["startDate"] = QDateTime();
        values["hasStartDate"] = FALSE;
    }
    values["completedDate"] = todo->completed().dateTime().toLocalTime();
#if KDE_IS_VERSION(4,4,0)
    values["inProgress"] = todo->isInProgress(FALSE);
#endif
    values["isOverdue"] = todo->isOverdue();
    if (todo->hasDueDate()) {
        values["dueDate"] = todo->dtDue().dateTime().toLocalTime();
        values["hasDueDate"] = TRUE;
    } else {
        values["dueDate"] = QDateTime::currentDateTime().addDays(366);
        values["hasDueDate"] = FALSE;
    }
#if KDE_IS_VERSION(4,4,60)
    values["tooltip"] = KCal::IncidenceFormatter::toolTipStr(collection.resource(), todo, todo->dtStart().date(), TRUE, KDateTime::Spec::LocalZone());
#else
    values["tooltip"] = KCal::IncidenceFormatter::toolTipStr(todo, TRUE, KDateTime::Spec::LocalZone());
#endif

    return values;
}

#include "eventmodel.moc"
