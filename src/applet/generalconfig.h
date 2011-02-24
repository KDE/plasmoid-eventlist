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
 *   Copyright (C) 2011 by gerdfleischer <gerdfleischer@web.de>
 */

#ifndef GENERALCONFIG_H
#define GENERALCONFIG_H

#include "ui_eventappletgeneralconfig.h"

class GeneralConfig : public QWidget, public Ui::EventAppletGeneralConfig
{
    Q_OBJECT

public:
    GeneralConfig(QWidget *parent = 0);
    ~GeneralConfig();
    void setupConnections();

private slots:
    void slotAddHeader();
    void slotRemoveHeader();
    void checkItem(QTreeWidgetItem *item, int column);

private:
    QList<int> usedDays();
};

#endif

