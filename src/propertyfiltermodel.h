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

#ifndef CATEGORIESFILTERMODEL_H
#define CATEGORIESFILTERMODEL_H

#include <QSortFilterProxyModel>
#include <QStringList>

class PropertyFilterModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    explicit PropertyFilterModel(QObject *parent = 0);
    ~PropertyFilterModel();
    
    void setExcludedResources(QStringList resources);
    void setDisabledCategories(QStringList categories);
    
protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const;

private:
    QStringList m_excludedResources;
    QStringList m_disabledCategories;
};

#endif
