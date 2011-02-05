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

#include "formatconfig.h"
#include <KDebug>

FormatConfig::FormatConfig(QWidget *parent)
    : QWidget(parent)
{
}

FormatConfig::~FormatConfig()
{
}

void FormatConfig::setupConnections()
{
	connect(addCategoryButton, SIGNAL(clicked()), this, SLOT(slotAddCategory()));
	connect(copyCategoryButton, SIGNAL(clicked()), this, SLOT(slotCopyCategory()));
	connect(removeCategoryButton, SIGNAL(clicked()), this, SLOT(slotRemoveCategory()));
}

void FormatConfig::slotAddCategory()
{
	QStringList itemText;
	itemText << i18n("Unspecified") << QString("%{startDate} %{startTime} %{summary}");
	QTreeWidgetItem *categoryItem = new QTreeWidgetItem(itemText);
	categoryItem->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEditable|Qt::ItemIsEnabled);
	categoryFormatWidget->addTopLevelItem(categoryItem);
}

void FormatConfig::slotCopyCategory()
{
	if (categoryFormatWidget->currentItem()) {
		QTreeWidgetItem *categoryItem = categoryFormatWidget->currentItem()->clone();
		categoryItem->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEditable|Qt::ItemIsEnabled);
		categoryFormatWidget->addTopLevelItem(categoryItem);
	}
}

void FormatConfig::slotRemoveCategory()
{
	int index = categoryFormatWidget->indexOfTopLevelItem(categoryFormatWidget->currentItem());
	categoryFormatWidget->takeTopLevelItem(index);
}
