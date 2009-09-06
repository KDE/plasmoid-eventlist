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

#ifndef EVENTENGINE_H
#define EVENTENGINE_H

// plasma headers
#include <plasma/dataengine.h>

class QTimer;
class KOrganizerEngineUtil;

class EventEngine : public Plasma::DataEngine
{
    Q_OBJECT

public:
    EventEngine(QObject* parent, const QVariantList& args);
    ~EventEngine();

    QStringList sources() const;

protected:
    bool sourceRequestEvent(const QString &name);
    bool updateSourceEvent(const QString& source);

private slots:
    void slotCalendarChanged();
    void slotServerStateChanged();
    void midnightTimerExp();

private:
    KOrganizerEngineUtil *m_util;
    QTimer *m_midnightTimer;
};

K_EXPORT_PLASMA_DATAENGINE(events, EventEngine)

#endif
