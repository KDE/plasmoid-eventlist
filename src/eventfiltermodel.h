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

#ifndef EVENTFILTERMODEL_H
#define EVENTFILTERMODEL_H

#include <QSortFilterProxyModel>
#include <QStringList>

class EventFilterModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    explicit EventFilterModel(QObject *parent = 0);
    ~EventFilterModel();
    
    void setPeriod(int period);
    void setShowFinishedTodos(bool showFinishedTodos);
    void setExcludedResources(QStringList resources);
    void setDisabledCategories(QStringList categories);
    
protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const;

private:
    bool isDisabledCategory(QModelIndex idx) const;

private:
    int m_period;
    bool m_showFinishedTodos;
    QStringList m_excludedResources, m_disabledCategories;
};

#endif
