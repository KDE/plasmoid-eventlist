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
 *   Copyright (C) 2010 by gerdfleischer <gerdfleischer@web.de>
 */

#include "eventmodel.h"
#include "propertyfiltermodel.h"

#include <QVariant>
#include <QDate>

PropertyFilterModel::PropertyFilterModel(QObject *parent) : QSortFilterProxyModel(parent)
{
    m_excludedResources = QStringList();
    m_disabledCategories = QStringList();
}

PropertyFilterModel::~PropertyFilterModel()
{
}

void PropertyFilterModel::setExcludedResources(QStringList resources)
{
    m_excludedResources = resources;
    m_excludedResources.sort();
    invalidateFilter();
}

void PropertyFilterModel::setDisabledCategories(QStringList categories)
{
    m_disabledCategories = categories;
    m_disabledCategories.sort();
    invalidateFilter();
}

bool PropertyFilterModel::filterAcceptsRow( int sourceRow, const QModelIndex &sourceParent ) const
{
    const QModelIndex idx = sourceModel()->index( sourceRow, 0, sourceParent );

    const int itemType = idx.data(EventModel::ItemTypeRole).toInt();
    const QString resourceRole = idx.data(EventModel::ResourceRole).toString();

    if (itemType == EventModel::HeaderItem) {
        return TRUE;
    } else {
        const QMap<QString, QVariant> values = idx.data(Qt::DisplayRole).toMap();
        foreach (QString disabledCategory, m_disabledCategories) {
            if (values["categories"].toStringList().contains(disabledCategory)) {
                return FALSE;
            }
        }

        return TRUE;
    }
}
