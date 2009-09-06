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
 */

#ifndef KORGANIZERENGINEUTIL_H
#define KORGANIZERENGINEUTIL_H

#include <akonadi/monitor.h>

// qt headers
#include <QMap>
#include <QObject>

class KConfig;

class KOrganizerEngineUtil : public QObject
{
    Q_OBJECT
public:
    KOrganizerEngineUtil(QObject *parent=0);
    ~KOrganizerEngineUtil();

    bool serverstate() const;
    QStringList categories() const;
    QMap <QString, QVariant> colors() const;
    QList <QVariant> events() const;

private slots:
    void akonadiStarted();
    void akonadiStopped();

private:
    void connectAkonadiSignals();

    KConfig *m_config;
    Akonadi::Monitor *m_akonadiMonitor;

signals:
    void calendarChanged();
    void serverStateChanged();
};

#endif
