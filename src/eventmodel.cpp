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
#include <kcalcore/recurrence.h>
#include <kcalutils/incidenceformatter.h>

#include <akonadi/collectionfetchjob.h>
#include <akonadi/collectionfetchscope.h>
#include <akonadi/item.h>
#include <akonadi/itemfetchjob.h>
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

EventModel::EventModel(QObject *parent, int urgencyTime, int birthdayTime, QList<QColor> colorList, int count) : QStandardItemModel(parent),
    parentItem(0),
    m_monitor(0)
{
    parentItem = invisibleRootItem();
    settingsChanged(urgencyTime, birthdayTime, colorList, count);
//     initModel();
//     initMonitor();
}

EventModel::~EventModel()
{
}

void EventModel::initModel()
{
    createHeaderItems(m_headerPartsList);

    Akonadi::CollectionFetchScope scope;
    QStringList mimeTypes;
    mimeTypes << KCalCore::Event::eventMimeType();
    mimeTypes << KCalCore::Todo::todoMimeType();
    mimeTypes << "text/calendar";
    scope.setContentMimeTypes(mimeTypes);

    Akonadi::CollectionFetchJob *job = new Akonadi::CollectionFetchJob(Akonadi::Collection::root(),
                                                                       Akonadi::CollectionFetchJob::Recursive);
    job->setFetchScope(scope);
    connect(job, SIGNAL(result(KJob *)), this, SLOT(initialCollectionFetchFinished(KJob *)));
    job->start();
}

void EventModel::initialCollectionFetchFinished(KJob *job)
{
    if (job->error()) {
        kDebug() << "Initial collection fetch failed!";
    } else {
        Akonadi::CollectionFetchJob *cJob = qobject_cast<Akonadi::CollectionFetchJob *>(job);
        Akonadi::Collection::List collections = cJob->collections();
        foreach (const Akonadi::Collection &collection, collections) {
            m_collections.insert(collection.id(), collection);
            Akonadi::ItemFetchJob *job = new Akonadi::ItemFetchJob(collection);
            job->fetchScope().fetchFullPayload();
            job->fetchScope().setAncestorRetrieval( Akonadi::ItemFetchScope::Parent );
            
            connect(job, SIGNAL(result(KJob *)), this, SLOT(initialItemFetchFinished(KJob *)));
            job->start();
        }
    }
}

void EventModel::initialItemFetchFinished(KJob *job)
{
    if (job->error()) {
        kDebug() << "Initial item fetch failed!";
    } else {
        Akonadi::ItemFetchJob *iJob = qobject_cast<Akonadi::ItemFetchJob *>(job);
        Akonadi::Item::List items = iJob->items();
        foreach (const Akonadi::Item &item, items) {
            if (item.hasPayload <KCalCore::Event::Ptr>()) {
                KCalCore::Event::Ptr event = item.payload <KCalCore::Event::Ptr>();
                if (event) {
                    addEventItem(eventDetails(item, event));
                } // if event
            } else if (item.hasPayload <KCalCore::Todo::Ptr>()) {
                KCalCore::Todo::Ptr todo = item.payload<KCalCore::Todo::Ptr>();
                if (todo) {
                    addTodoItem(todoDetails(item, todo));
                }
            } // if hasPayload
        } // foreach
    }
    
    setSortRole(EventModel::SortRole);
    sort(0, Qt::AscendingOrder);
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
    m_monitor->setMimeTypeMonitored(KCalCore::Event::eventMimeType(), true);
    m_monitor->setMimeTypeMonitored(KCalCore::Todo::todoMimeType(), true);
    m_monitor->setMimeTypeMonitored("text/calendar", true);

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
    data["title"] = QString("<b>" + title + "</b>");
    item->setData(data, Qt::DisplayRole);
    QColor textColor = Plasma::Theme::defaultTheme()->color(Plasma::Theme::TextColor);
    item->setForeground(QBrush(textColor));
    item->setData(QVariant(QDateTime(QDate::currentDate().addDays(days))), SortRole);
    item->setData(QVariant(HeaderItem), ItemTypeRole);
    item->setData(QVariant(QString()), ResourceRole);
    item->setData(QVariant(QString()), UIDRole);
    item->setData(QVariant("<qt><b>" + toolTip + "</b></qt>"), TooltipRole);
}

void EventModel::resetModel()
{
    clear();
    parentItem = invisibleRootItem();
    delete m_monitor;
    m_monitor = 0;

    if (!Akonadi::ServerManager::isRunning()) {
        QStandardItem *errorItem = new QStandardItem();
        errorItem->setData(QVariant(i18n("The Akonadi server is not running.")), Qt::DisplayRole);
        parentItem->appendRow(errorItem);
    } else {
        initModel();
        initMonitor();
    }
}

void EventModel::settingsChanged(int urgencyTime, int birthdayTime, QList<QColor> itemColors, int count)
{
    urgency = urgencyTime;
    birthdayUrgency = birthdayTime;
    urgentBg = itemColors.at(urgentColorPos);
    passedFg = itemColors.at(passedColorPos);
    todoBg = itemColors.at(todoColorPos);
    finishedTodoBg = itemColors.at(finishedTodoColorPos);
    recurringCount = count;
}

void EventModel::setCategoryColors(QHash<QString, QColor> categoryColors)
{
    m_categoryColors = categoryColors;
}

void EventModel::setHeaderItems(QStringList headerParts)
{
    m_headerPartsList = headerParts;
}

void EventModel::createHeaderItems(QStringList headerParts)
{
    m_sectionItemsMap.clear();

    QStandardItem *olderItem = new QStandardItem();
    initHeaderItem(olderItem, i18n("Earlier stuff"), i18n("Unfinished todos, still ongoing earlier events, etc."), -28);
    m_sectionItemsMap.insert(olderItem->data(SortRole).toDate(), olderItem);

    QStandardItem *somedayItem = new QStandardItem();
    initHeaderItem(somedayItem, i18n("Some day"), i18n("Todos with no due date"), 366);
    m_sectionItemsMap.insert(somedayItem->data(SortRole).toDate(), somedayItem);

    for (int i = 0; i < headerParts.size(); i += 3) {
        QStandardItem *item = new QStandardItem();
        initHeaderItem(item, headerParts.value(i), headerParts.value(i + 1), headerParts.value(i + 2).toInt());
        m_sectionItemsMap.insert(item->data(SortRole).toDate(), item);
    }
}

void EventModel::itemAdded(const Akonadi::Item &item, const Akonadi::Collection &collection)
{
    m_collections.insert(collection.id(), collection);
    addItem(item, collection);
}

void EventModel::removeItem(const Akonadi::Item &item)
{
    foreach (QStandardItem *i, m_sectionItemsMap) {
        QModelIndexList l;
        if (i->hasChildren())
            l = match(i->child(0, 0)->index(), EventModel::ItemIDRole, item.id());
        while (!l.isEmpty()) {
            i->removeRow(l[0].row());
            if (!i->hasChildren()) break;
            l = match(i->child(0, 0)->index(), EventModel::ItemIDRole, item.id());
        }
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
    removeItem(item);
    addItem(item, item.parentCollection());
}

void EventModel::itemMoved(const Akonadi::Item &item, const Akonadi::Collection &, const Akonadi::Collection &)
{
    kDebug() << "item moved";
    removeItem(item);
    addItem(item, item.parentCollection());
}

void EventModel::addItem(const Akonadi::Item &item, const Akonadi::Collection &collection)
{
    Q_UNUSED(collection);

    if (item.hasPayload<KCalCore::Event::Ptr>()) {
        KCalCore::Event::Ptr event = item.payload <KCalCore::Event::Ptr>();
        if (event) {
            addEventItem(eventDetails(item, event));
        } // if event
    } else if (item.hasPayload <KCalCore::Todo::Ptr>()) {
        KCalCore::Todo::Ptr todo = item.payload<KCalCore::Todo::Ptr>();
        if (todo) {
            addTodoItem(todoDetails(item, todo));
        }
    }
}

void EventModel::addEventItem(const QMap<QString, QVariant> &values)
{
    QMap<QString, QVariant> data = values;
    QString category = values["mainCategory"].toString();
    QColor textColor = Plasma::Theme::defaultTheme()->color(Plasma::Theme::TextColor);

    // dont add events starting later than a year
    if (values["startDate"].toDate() > QDate::currentDate().addDays(365)) {
        return;
    }

    if (values["recurs"].toBool()) {
        int c = 0;
        QList<QVariant> dtTimes = values["recurDates"].toList();
        foreach (QVariant eventDtTime, dtTimes) {
            if (recurringCount != 0 && c >= recurringCount)
                break;

            QStandardItem *eventItem = new QStandardItem();
            eventItem->setForeground(QBrush(textColor));
            data["startDate"] = eventDtTime;

            int d = values["startDate"].toDateTime().daysTo(values["endDate"].toDateTime());
            data["endDate"] = eventDtTime.toDateTime().addDays(d);

            QDate itemDt = eventDtTime.toDate();
            if (values["isBirthday"].toBool()) {
                data["itemType"] = BirthdayItem;
                int n = eventDtTime.toDate().year() - values["startDate"].toDate().year();
                n > 2000 ? data["yearsSince"] = "XX" : data["yearsSince"] = QString::number(n); // workaround missing facebook birthdays
                if (itemDt >= QDate::currentDate() && QDate::currentDate().daysTo(itemDt) < birthdayUrgency) {
                    eventItem->setBackground(QBrush(urgentBg));
                } else {
                    if (m_categoryColors.contains(i18n("Birthday")) || m_categoryColors.contains("Birthday")) {
                        QString b = "Birthday";
                        if (m_categoryColors.contains(i18n("Birthday"))) {
                            b = i18n("Birthday");
                        }
                        eventItem->setBackground(QBrush(m_categoryColors.value(b)));
                    }
                }
                eventItem->setData(QVariant(BirthdayItem), ItemTypeRole);
            } else if (values["isAnniversary"].toBool()) {
                data["itemType"] = AnniversaryItem;
                int n = eventDtTime.toDate().year() - values["startDate"].toDate().year();
                data["yearsSince"] = QString::number(n);
                if (itemDt >= QDate::currentDate() && QDate::currentDate().daysTo(itemDt) < birthdayUrgency) {
                    eventItem->setBackground(QBrush(urgentBg));
                } else {
                    if (m_categoryColors.contains(category)) {
                        eventItem->setBackground(QBrush(m_categoryColors.value(category)));
                    }
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
                } else if (m_categoryColors.contains(category)) {
                    eventItem->setBackground(QBrush(m_categoryColors.value(category)));
                }
            }

            eventItem->setData(data, Qt::DisplayRole);
            eventItem->setData(eventDtTime, SortRole);
            eventItem->setData(values["uid"], UIDRole);
            eventItem->setData(values["itemid"], ItemIDRole);
            eventItem->setData(values["resource"], ResourceRole);
            eventItem->setData(values["tooltip"], TooltipRole);

            addItemRow(eventDtTime.toDate(), eventItem);

            ++c;
        }
    } else {
        QStandardItem *eventItem;
        eventItem = new QStandardItem();
        data["itemType"] = NormalItem;
        eventItem->setData(QVariant(NormalItem), ItemTypeRole);
        eventItem->setForeground(QBrush(textColor));
        eventItem->setData(data, Qt::DisplayRole);
        eventItem->setData(values["startDate"], SortRole);
        eventItem->setData(values["uid"], EventModel::UIDRole);
        eventItem->setData(values["itemid"], ItemIDRole);
        eventItem->setData(values["resource"], ResourceRole);
        eventItem->setData(values["tooltip"], TooltipRole);
        QDateTime itemDtTime = values["startDate"].toDateTime();
        if (itemDtTime > QDateTime::currentDateTime() && QDateTime::currentDateTime().secsTo(itemDtTime) < urgency * 60) {
            eventItem->setBackground(QBrush(urgentBg));
        } else if (QDateTime::currentDateTime() > itemDtTime) {
            eventItem->setForeground(QBrush(passedFg));
        } else if (m_categoryColors.contains(category)) {
            eventItem->setBackground(QBrush(m_categoryColors.value(category)));
        }

        addItemRow(values["startDate"].toDate(), eventItem);
    }
}

void EventModel::addTodoItem(const QMap <QString, QVariant> &values)
{
    QColor textColor = Plasma::Theme::defaultTheme()->color(Plasma::Theme::TextColor);
    QMap<QString, QVariant> data = values;
    QString category = values["mainCategory"].toString();

    // dont add todos starting later than a year
    if (values["hasDueDate"].toBool() == TRUE && values["dueDate"].toDate() > QDate::currentDate().addDays(365)) {
        return;
    }

    if (values["recurs"].toBool()) {
        int c = 0;
        QList<QVariant> dtTimes = values["recurDates"].toList();
        foreach (QVariant eventDtTime, dtTimes) {
            if (recurringCount != 0 && c >= recurringCount)
                break;

            QStandardItem *todoItem = new QStandardItem();
            data["dueDate"] = eventDtTime;
            data["itemType"] = TodoItem;
            todoItem->setData(QVariant(TodoItem), ItemTypeRole);
            todoItem->setForeground(QBrush(textColor));
            todoItem->setData(data, Qt::DisplayRole);
            todoItem->setData(eventDtTime, SortRole);
            todoItem->setData(values["uid"], EventModel::UIDRole);
            todoItem->setData(values["itemid"], ItemIDRole);
            todoItem->setData(values["resource"], ResourceRole);
            todoItem->setData(values["tooltip"], TooltipRole);
            if (values["completed"].toBool() == TRUE) {
                todoItem->setBackground(QBrush(finishedTodoBg));
            } else if (m_categoryColors.contains(category)) {
                todoItem->setBackground(QBrush(m_categoryColors.value(category)));
            } else {
                todoItem->setBackground(QBrush(todoBg));
            }

            addItemRow(eventDtTime.toDate(), todoItem);

            ++c;
        }
    } else {
        QStandardItem *todoItem = new QStandardItem();
        data["itemType"] = TodoItem;
        todoItem->setData(QVariant(TodoItem), ItemTypeRole);
        todoItem->setForeground(QBrush(textColor));
        todoItem->setData(data, Qt::DisplayRole);
        todoItem->setData(values["dueDate"], SortRole);
        todoItem->setData(values["uid"], EventModel::UIDRole);
        todoItem->setData(values["itemid"], ItemIDRole);
        todoItem->setData(values["resource"], ResourceRole);
        todoItem->setData(values["tooltip"], TooltipRole);
        if (values["completed"].toBool() == TRUE) {
            todoItem->setBackground(QBrush(finishedTodoBg));
        } else if (m_categoryColors.contains(category)) {
            todoItem->setBackground(QBrush(m_categoryColors.value(category)));
        } else {
            todoItem->setBackground(QBrush(todoBg));
        }

        addItemRow(values["dueDate"].toDate(), todoItem);
    }
}

void EventModel::addItemRow(QDate eventDate, QStandardItem *incidenceItem)
{
    QStandardItem *headerItem = 0;

    foreach (QStandardItem *item, m_sectionItemsMap) {
        if (eventDate < item->data(SortRole).toDate())
            break;
        else
            headerItem = item;
    }

    if (headerItem) {
        headerItem->appendRow(incidenceItem);
        headerItem->sortChildren(0, Qt::AscendingOrder);
        if (headerItem->row() == -1) {
            parentItem->appendRow(headerItem);
            parentItem->sortChildren(0, Qt::AscendingOrder);
        }
        
        emit modelNeedsExpanding();
    }
}

QMap<QString, QVariant> EventModel::eventDetails(const Akonadi::Item &item, KCalCore::Event::Ptr event)
{
    QMap <QString, QVariant> values;
    Akonadi::Collection itemCollection = m_collections.value(item.storageCollectionId());
    values["resource"] = itemCollection.resource();
    values["resourceName"] = itemCollection.name();
    values["uid"] = event->uid();
    values["itemid"] = item.id();
    values["remoteid"] = item.remoteId();
    values["summary"] = event->summary();
    values["description"] = event->description();
    values["location"] = event->location();
    QStringList categories = event->categories();
    if (categories.isEmpty()) {
        values["categories"] = i18n("Unspecified");
        values["mainCategory"] = i18n("Unspecified");
    } else {
        values["categories"] = event->categoriesStr();
        values["mainCategory"] = categories.first();
    }

    values["status"] = event->status();
    values["startDate"] = event->dtStart().dateTime().toLocalTime();
    values["endDate"] = event->dtEnd().dateTime().toLocalTime();

    bool recurs = event->recurs();
    values["recurs"] = recurs;
    QList<QVariant> recurDates;
    if (recurs) {
        KCalCore::Recurrence *r = event->recurrence();
        KCalCore::DateTimeList dtTimes = r->timesInInterval(KDateTime(QDate::currentDate()), KDateTime(QDate::currentDate()).addDays(365));
        dtTimes.sortUnique();
        foreach (KDateTime t, dtTimes) {
            recurDates << QVariant(t.dateTime().toLocalTime());
        }
    }
    values["recurDates"] = recurDates;

    if (event->customProperty("KABC", "BIRTHDAY") == QString("YES") || categories.contains(i18n("Birthday")) || categories.contains("Birthday")) {
        values["isBirthday"] = QVariant(TRUE);
    } else {
        values["isBirthday"] = QVariant(FALSE);
    }

    event->customProperty("KABC", "ANNIVERSARY") == QString("YES") ? values ["isAnniversary"] = QVariant(TRUE) : QVariant(FALSE);
    values["tooltip"] = KCalUtils::IncidenceFormatter::toolTipStr(itemCollection.name(), event, event->dtStart().date(), TRUE, KDateTime::Spec::LocalZone());

    return values;
}

QMap<QString, QVariant> EventModel::todoDetails(const Akonadi::Item &item, KCalCore::Todo::Ptr todo)
{
    QMap <QString, QVariant> values;
    Akonadi::Collection itemCollection = m_collections.value(item.storageCollectionId());
    values["resource"] = itemCollection.resource();
    values["resourceName"] = itemCollection.name();
    values["uid"] = todo->uid();
    values["itemid"] = item.id();
    values["remoteid"] = item.remoteId();
    values["summary"] = todo->summary();
    values["description"] = todo->description();
    values["location"] = todo->location();
    QStringList categories = todo->categories();
    if (categories.isEmpty()) {
        values["categories"] = i18n("Unspecified");
        values["mainCategory"] = i18n("Unspecified");
    } else {
        values["categories"] = todo->categoriesStr();
        values["mainCategory"] = categories.first();
    }

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
    values["inProgress"] = todo->isInProgress(FALSE);
    values["isOverdue"] = todo->isOverdue();
    if (todo->hasDueDate()) {
        values["dueDate"] = todo->dtDue().dateTime().toLocalTime();
        values["hasDueDate"] = TRUE;
    } else {
        values["dueDate"] = QDateTime::currentDateTime().addDays(366);
        values["hasDueDate"] = FALSE;
    }

    bool recurs = todo->recurs();
    values["recurs"] = recurs;
    QList<QVariant> recurDates;
    if (recurs) {
        KCalCore::Recurrence *r = todo->recurrence();
        KCalCore::DateTimeList dtTimes = r->timesInInterval(KDateTime(QDate::currentDate()), KDateTime(QDate::currentDate()).addDays(365));
        dtTimes.sortUnique();
        foreach (KDateTime t, dtTimes) {
            recurDates << QVariant(t.dateTime().toLocalTime());
        }
    }
    values["recurDates"] = recurDates;

    values["tooltip"] = KCalUtils::IncidenceFormatter::toolTipStr(itemCollection.name(), todo, todo->dtStart().date(), TRUE, KDateTime::Spec::LocalZone());

    return values;
}

#include "eventmodel.moc"
