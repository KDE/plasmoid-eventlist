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

#include "korganizerengineutil.h"

// qt headers
#include <QColor>
#include <QList>
#include <QString>

// kde headers
#include <KConfig>
#include <KConfigGroup>
#include <KDebug>
#include <KStandardDirs>
#include <KDateTime>

// kdepim headers
#include <kcal/incidenceformatter.h>
#include <kcal/event.h>
#include <kcal/recurrence.h>

// Akonadi
#include <akonadi/servermanager.h>
#include <akonadi/control.h>
#include <akonadi/collection.h>
#include <akonadi/collectionfetchjob.h>
#include <akonadi/item.h>
#include <akonadi/itemfetchjob.h>
#include <akonadi/itemfetchscope.h>

KOrganizerEngineUtil::KOrganizerEngineUtil(QObject *parent)
    : QObject(parent),
    m_akonadiMonitor(0)
{
    m_config = new KConfig("korganizerrc");

    if (Akonadi::Control::start()) {
        m_akonadiMonitor = new Akonadi::Monitor();
        m_akonadiMonitor->setAllMonitored(true);
        connectAkonadiSignals();
    }

    connect(Akonadi::ServerManager::self(), SIGNAL(started()), this, SLOT(akonadiStarted()));
    connect(Akonadi::ServerManager::self(), SIGNAL(stopped()), this, SLOT(akonadiStopped()));
}

KOrganizerEngineUtil::~KOrganizerEngineUtil()
{
    delete m_config;
    delete m_akonadiMonitor;
}

bool KOrganizerEngineUtil::serverstate() const
{
    return Akonadi::ServerManager::isRunning();
}

QStringList KOrganizerEngineUtil::categories() const
{
    KConfigGroup general(m_config, "General");
    return general.readEntry("Custom Categories", QStringList());
}

QMap <QString, QVariant> KOrganizerEngineUtil::colors() const
{
    KConfigGroup general(m_config, "Category Colors2");

    QMap <QString, QVariant> colors;
    foreach(const QString &category, categories()) {
        colors.insert(category, QVariant(general.readEntry(category, QColor())));
    }

    return colors;
}

QList <QVariant> KOrganizerEngineUtil::events() const
{
    QList <QVariant> events;

    if (!m_akonadiMonitor)
        return events;

    Akonadi::CollectionFetchJob *job = new Akonadi::CollectionFetchJob(Akonadi::Collection::root(),
                                                                       Akonadi::CollectionFetchJob::Recursive);
    if (job->exec()) {
        Akonadi::Collection::List collections = job->collections();
        foreach(const Akonadi::Collection &collection, collections) {
            Akonadi::ItemFetchJob *ijob = new Akonadi::ItemFetchJob(collection);
            ijob->fetchScope().fetchFullPayload();

            if (ijob->exec()) {
                Akonadi::Item::List items = ijob->items();
                foreach(const Akonadi::Item &item, items) {
                    if (item.hasPayload <KCal::Event::Ptr>()) {
                        KCal::Event *event = item.payload <KCal::Event::Ptr>().get();
                        if (event) {
                            QMap <QString, QVariant> values;
                            values ["summary"] = event->summary();
                            values ["categories"] = event->categories();
                            values ["status"] = event->status();
                            values ["startDate"] = event->dtStart().dateTime();
                            values ["endDate"] = event->dtEnd().dateTime();
                            values ["uid"] = event->uid();
                            values ["description"] = event->description();
                            values ["location"] = event->location();
                            bool recurs = event->recurs();
                            values ["recurs"] = recurs;
                            QList<QVariant> recurDates;
                            if (recurs) {
                                KCal::Recurrence *r = event->recurrence();
                                KCal::DateTimeList dtTimes = r->timesInInterval(KDateTime(QDate::currentDate()), KDateTime(QDate::currentDate()).addDays(366));
                                dtTimes.sortUnique();
                                foreach (KDateTime t, dtTimes) {
                                    recurDates << QVariant(t.dateTime());
                                }
                            }
                            values ["recurDates"] = recurDates;
                            event->customProperty("KABC", "BIRTHDAY") == QString("YES") ? values ["isBirthday"] = QVariant(TRUE) : QVariant(FALSE);
                            event->customProperty("KABC", "ANNIVERSARY") == QString("YES") ? values ["isAnniversary"] = QVariant(TRUE) : QVariant(FALSE);
                            values ["tooltip"] = KCal::IncidenceFormatter::toolTipStr(event);

                            events << values;
                        } // if event
                    } // if hasPayload
                } // foreach
            }
        }
    }

    return events;
}

void KOrganizerEngineUtil::akonadiStarted()
{
    m_akonadiMonitor = new Akonadi::Monitor();
    m_akonadiMonitor->setAllMonitored(true);
    connectAkonadiSignals();
    emit serverStateChanged();
}

void KOrganizerEngineUtil::akonadiStopped()
{
    delete m_akonadiMonitor;
    m_akonadiMonitor = 0;
    emit serverStateChanged();
}

void KOrganizerEngineUtil::connectAkonadiSignals()
{
    connect(m_akonadiMonitor, SIGNAL(itemChanged (const Akonadi::Item &, const QSet< QByteArray > &)),
                              SIGNAL(calendarChanged()));

    connect(m_akonadiMonitor, SIGNAL(itemAdded(const Akonadi::Item &, const Akonadi::Collection &)),
                              SIGNAL(calendarChanged()));

    connect(m_akonadiMonitor, SIGNAL(itemRemoved(const Akonadi::Item &)),
                              SIGNAL(calendarChanged()));
}    
