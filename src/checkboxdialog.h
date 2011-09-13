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

#ifndef CHECKBOXDIALOG_H
#define CHECKBOXDIALOG_H

#include <KDialog>

class CheckBoxDialog : public KDialog
{
    Q_OBJECT

public:
    CheckBoxDialog(QWidget* parent, QStringList disabledProperties, QMap<QString, QString> properties);
    ~CheckBoxDialog();

    QStringList disabledProperties();

private slots:
    void slotUncheckAll();
    void slotCheckAll();

private:
    QWidget *m_checkBoxWidget;
};

#endif
