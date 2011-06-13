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

#ifndef FORMATCONFIG_H
#define FORMATCONFIG_H

#include "ui_eventappletformatconfig.h"

class FormatConfig : public QWidget, public Ui::EventAppletFormatConfig
{
    Q_OBJECT

public:
    FormatConfig(QWidget *parent = 0);
    ~FormatConfig();
	void setupConnections();

signals:
    void categoryItemCountChanged();


private slots:
	void slotAddCategory();
	void slotCopyCategory();
	void slotRemoveCategory();
};

#endif

