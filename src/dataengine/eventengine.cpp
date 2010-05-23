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

#include "eventengine.h"
#include "korganizerengineutil.h"

static const char *CATEGORY_SOURCE = "Categories";
static const char *COLOR_SOURCE    = "Colors";
static const char *EVENT_SOURCE    = "Events";
static const char *SERVERSTATE_SOURCE = "ServerState";

EventEngine::EventEngine(QObject* parent, const QVariantList& args)
    : Plasma::DataEngine(parent)
{
    Q_UNUSED(args);

    m_util = new KOrganizerEngineUtil(this);
    connect(m_util, SIGNAL(calendarChanged()),this, SLOT(slotCalendarChanged()));
    connect(m_util, SIGNAL(serverStateChanged()),this, SLOT(slotServerStateChanged()));
}

EventEngine::~EventEngine()
{
    delete m_util;
}

QStringList EventEngine::sources() const
{
    QStringList sources;
    sources << CATEGORY_SOURCE;
    sources << COLOR_SOURCE;
    sources << EVENT_SOURCE;
    sources << SERVERSTATE_SOURCE;

    return sources;
}

bool EventEngine::sourceRequestEvent(const QString &name)
{
    return updateSourceEvent(name);
}

bool EventEngine::updateSourceEvent(const QString &name)
{
    if(QString::compare(name, CATEGORY_SOURCE) == 0) {
        setData(I18N_NOOP(name), I18N_NOOP("categories"), m_util->categories());
    }
    else if(QString::compare(name, COLOR_SOURCE) == 0) {
        setData(I18N_NOOP(name), I18N_NOOP("colors"), m_util->colors());
    }
    else if(QString::compare(name, EVENT_SOURCE) == 0) {
        setData(I18N_NOOP(name), I18N_NOOP("events"), m_util->events());
    }
    else if(QString::compare(name, SERVERSTATE_SOURCE) == 0) {
        setData(I18N_NOOP(name), I18N_NOOP("serverrunning"), m_util->serverstate());
    }
    return true;
}

void EventEngine::slotCalendarChanged()
{
    updateSourceEvent(COLOR_SOURCE);
    updateSourceEvent(EVENT_SOURCE);
}

void EventEngine::slotServerStateChanged()
{
    updateSourceEvent(SERVERSTATE_SOURCE);
    updateSourceEvent(EVENT_SOURCE);
}

#include "eventengine.moc"
