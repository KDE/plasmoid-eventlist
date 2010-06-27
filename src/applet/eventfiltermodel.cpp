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
#include "eventfiltermodel.h"

#include <QVariant>
#include <QDate>

EventFilterModel::EventFilterModel(QObject *parent) : QSortFilterProxyModel(parent)
{
    m_period = 365;
    m_excludedResources = QStringList();
}

EventFilterModel::~EventFilterModel()
{
}

void EventFilterModel::setPeriod(int period)
{
    m_period = period;
    invalidateFilter();
}

void EventFilterModel::setExcludedResources(QStringList resources)
{
    m_excludedResources = resources;
    m_excludedResources.sort();
    invalidateFilter();
}

bool EventFilterModel::filterAcceptsRow( int sourceRow, const QModelIndex &sourceParent ) const
{
    const QModelIndex idx = sourceModel()->index( sourceRow, 0, sourceParent );

    const QVariant t = idx.data(EventModel::ItemTypeRole);
    const QVariant d = idx.data(EventModel::SortRole);
    QDate date= d.toDate();

    if (date.isValid()) {
        if (date > QDate::currentDate().addDays(365)) {
            return TRUE; // todos with no specified due date
        } else if (date > QDate::currentDate().addDays(m_period)) {
            return FALSE;
        } else if (date < QDate::currentDate()) {
            if (t.toInt() == EventModel::HeaderItem) {
                if (sourceModel()->hasChildren(idx)) {
                    int rows = sourceModel()->rowCount(idx);
                    for (int row = 0; row < rows; ++row) {
                        QModelIndex childIdx = sourceModel()->index(row, 0, idx);
                        const QVariant v = childIdx.data(Qt::DisplayRole);
                        QMap<QString, QVariant> values = v.toMap();
                        if (t.toInt() == EventModel::TodoItem) {
                            if (values["completed"].toBool() == FALSE)
                                return TRUE;
                        } else {
                            if (values["endDate"].toDate() >= QDate::currentDate())
                                return TRUE;
                        }
                    }
                }
                return FALSE;
            } else {
                const QVariant v = idx.data(Qt::DisplayRole);
                QMap<QString, QVariant> values = v.toMap();
                if (t.toInt() == EventModel::TodoItem) {
                    if (values["completed"].toBool() == TRUE)
                        return FALSE;
                } else {
                    if (values["endDate"].toDate() < QDate::currentDate())
                        return FALSE;
                }
            }
        }
    }

    const QVariant r = idx.data(EventModel::ResourceRole);
    
    if (t.toInt() == EventModel::HeaderItem) {
        return !containsAllResources(r.toStringList());
    } else if (m_excludedResources.contains(r.toString())) {
        return FALSE;
    }

    return TRUE;
}

bool EventFilterModel::containsAllResources(const QStringList &resList) const
{
    bool hasAll = true;

    foreach (QString res, resList) {
        if (!m_excludedResources.contains(res)) {
            hasAll = false;
            break;
        }
    }

    return hasAll;
}
