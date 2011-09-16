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

#include "generalconfig.h"

#include <QTreeWidgetItemIterator>

#include <KDebug>

GeneralConfig::GeneralConfig(QWidget *parent)
    : QWidget(parent)
{
}

GeneralConfig::~GeneralConfig()
{
}

void GeneralConfig::setupConnections()
{
    connect(addHeaderButton, SIGNAL(clicked()), this, SLOT(slotAddHeader()));
    connect(removeHeaderButton, SIGNAL(clicked()), this, SLOT(slotRemoveHeader()));
    connect(headerWidget, SIGNAL(itemChanged(QTreeWidgetItem *, int)), this, SLOT(checkItem(QTreeWidgetItem *, int)));
}

void GeneralConfig::slotAddHeader()
{
    QList<int> used = usedDays();
    int begin = 32;
    while (used.contains(begin)) {
        ++begin;
    }

    QStringList itemText;
    itemText << i18nc("Header item title", "Title") << i18nc("Header item tooltip", "Your tooltip") << QString::number(begin);
    TreeWidgetItem *headerItem = new TreeWidgetItem(itemText);
    headerItem->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEditable|Qt::ItemIsEnabled);
    headerWidget->addTopLevelItem(headerItem);
    emit headerItemCountChanged();
}

void GeneralConfig::slotRemoveHeader()
{
    if (headerWidget->currentItem()->text(2) == QString::number(0))
        return;

    int index = headerWidget->indexOfTopLevelItem(headerWidget->currentItem());
    headerWidget->takeTopLevelItem(index);
    emit headerItemCountChanged();
}

QList<int> GeneralConfig::usedDays()
{
    QList<int> usedDays;
    QTreeWidgetItemIterator it(headerWidget);
    while (*it) {
        usedDays << (*it)->text(2).toInt();
         ++it;
     }

    return usedDays;
}

void GeneralConfig::checkItem(QTreeWidgetItem *item, int column)
{
    if (column != 2)
        return;

    QList<int> usedDays;
    QTreeWidgetItemIterator it(headerWidget);
    while (*it) {
        if (*it != item) {
            usedDays << (*it)->text(2).toInt();
        }
         ++it;
     }

    int begin = item->text(2).toInt();
    if (usedDays.contains(begin)) {
        while (usedDays.contains(begin)) {
            ++begin;
        }
        item->setText(2, QString::number(begin));
    }
    

}
