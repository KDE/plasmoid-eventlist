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

#include "eventfiltermodel.h"
#include "eventmodel.h"

#include <QVariant>
#include <QDate>

EventFilterModel::EventFilterModel(QObject *parent) : QSortFilterProxyModel(parent)
{
    m_period = 365;
    m_excludedResources = QStringList();
    m_disabledCategories = QStringList();
}

EventFilterModel::~EventFilterModel()
{
}

void EventFilterModel::setPeriod(int period)
{
    m_period = period;
    invalidateFilter();
}

void EventFilterModel::setShowFinishedTodos(bool showFinishedTodos)
{
    m_showFinishedTodos = showFinishedTodos;
    invalidateFilter();
}

void EventFilterModel::setExcludedResources(QStringList resources)
{
    m_excludedResources = resources;
    m_excludedResources.sort();
    invalidateFilter();
}

void EventFilterModel::setDisabledCategories(QStringList categories)
{
    m_disabledCategories = categories;
    m_disabledCategories.sort();
    invalidateFilter();
}

bool EventFilterModel::isDisabledCategory(QModelIndex idx) const
{
    bool isDisabled = false;
    const QMap<QString, QVariant> values = idx.data(Qt::DisplayRole).toMap();
    foreach (QString disabledCategory, m_disabledCategories) {
        if (values["categories"].toStringList().contains(disabledCategory)) {
            isDisabled = true;
            break;
        }
    }

    return isDisabled;
}

bool EventFilterModel::filterAcceptsRow( int sourceRow, const QModelIndex &sourceParent ) const
{
    const QModelIndex idx = sourceModel()->index( sourceRow, 0, sourceParent );

    const int itemType = idx.data(EventModel::ItemTypeRole).toInt();
    const QString resourceRole = idx.data(EventModel::ResourceRole).toString();

    const QVariant d = idx.data(EventModel::SortRole);
    const QDate date= d.toDate();

    if (date.isValid()) {
        if (date > QDate::currentDate().addDays(365)) { // todos with no specified due date
            if (itemType == EventModel::HeaderItem) {
                int rows = sourceModel()->rowCount(idx);
                for (int row = 0; row < rows; ++ row) { // if the header would be empty dont show it
                    QModelIndex childIdx = sourceModel()->index(row, 0, idx);
                    const QString cr = childIdx.data(EventModel::ResourceRole).toString();
                    if (!m_excludedResources.contains(cr) && !isDisabledCategory(childIdx)) {
                        const QMap<QString, QVariant> values = childIdx.data(Qt::DisplayRole).toMap();
                        if (m_showFinishedTodos || values["completed"].toBool() == false)
                            return true;
                    }
                }
                return false;
            } else {
                if (!m_excludedResources.contains(resourceRole) && !isDisabledCategory(idx)) {
                    const QMap<QString, QVariant> values = idx.data(Qt::DisplayRole).toMap();
                    if (m_showFinishedTodos || values["completed"].toBool() == false) {
                        return true;
                    }
                }
                return false;
            }
        } else if (date > QDate::currentDate().addDays(m_period)) { // stuff later than ...
            return false;
        } else if (date < QDate::currentDate()) { // older stuff
            if (itemType == EventModel::HeaderItem) { // dont show empty header
                if (sourceModel()->hasChildren(idx)) {
                    int rows = sourceModel()->rowCount(idx);
                    for (int row = 0; row < rows; ++row) {
                        QModelIndex childIdx = sourceModel()->index(row, 0, idx);
                        const QString cr = childIdx.data(EventModel::ResourceRole).toString();
                        if (!m_excludedResources.contains(cr) && !isDisabledCategory(childIdx)) {
                            const int childType = childIdx.data(EventModel::ItemTypeRole).toInt();
                            const QMap<QString, QVariant> values = childIdx.data(Qt::DisplayRole).toMap();
                            if (childType == EventModel::TodoItem) { // dont show old finished stuff
                                if (values["completed"].toBool() == false)
                                    return true;
                            } else {
                                if (values["endDate"].toDate() >= QDate::currentDate())
                                    return true;
                            }
                        }
                    }
                }
                return false;
            } else {
                if (m_excludedResources.contains(resourceRole) || isDisabledCategory(idx)) {
                    return false;
                }
                
                const QMap<QString, QVariant> values = idx.data(Qt::DisplayRole).toMap();
                if (itemType == EventModel::TodoItem) {
                    if (values["completed"].toBool() == true) {
                        return false;
                    }
                } else if (values["endDate"].toDate() < QDate::currentDate()) {
                    return false;
                }
            }
        } else { // stuff from today to period
            if (itemType == EventModel::HeaderItem) { // dont show empty header
                int rows = sourceModel()->rowCount(idx);
                for (int row = 0; row < rows; ++ row) {
                    QModelIndex childIdx = sourceModel()->index(row, 0, idx);
                    const QString cr = childIdx.data(EventModel::ResourceRole).toString();
                    const QDate cd = childIdx.data(EventModel::SortRole).toDate();
                    if ((!m_excludedResources.contains(cr) && !isDisabledCategory(childIdx)) && cd <= QDate::currentDate().addDays(m_period)) {
                        const int childType = childIdx.data(EventModel::ItemTypeRole).toInt();
                        const QMap<QString, QVariant> values = childIdx.data(Qt::DisplayRole).toMap();
                        if (childType != EventModel::TodoItem) {
                            return true;
                        } else if (m_showFinishedTodos || values["completed"].toBool() == false) {
                            return true;
                        }
                    }
                }
                return false;
            } else if (m_excludedResources.contains(resourceRole) || isDisabledCategory(idx)) {
                return false;
            } else if (itemType == EventModel::TodoItem) {
                const QMap<QString, QVariant> values = idx.data(Qt::DisplayRole).toMap();
                if (!m_showFinishedTodos && values["completed"].toBool() == true)
                    return false;
            }
        }
    } else { // no valid date
        return false;
    }

    return true;
}
