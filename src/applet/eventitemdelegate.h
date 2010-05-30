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
 *   Copyright (C) 2009 by gerdfleischer <gerdfleischer@web.de>
 */

#ifndef EVENTITEMDELEGATE_H
#define EVENTITEMDELEGATE_H

#include <QStyledItemDelegate>

class EventItemDelegate : public QStyledItemDelegate
{
	Q_OBJECT
	
public:
	EventItemDelegate(QObject* parent, QString normal, QString birthdayOrAnniversary, int dtFormat, QString dtString);
	~EventItemDelegate();

	QString displayText(const QVariant &value, const QLocale &locale)  const;
// 	void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
	QString formattedDate(const QVariant &dtTime) const;
	void settingsChanged(QString normal, QString birthdayOrAnniversary, int format, QString customString);

private:
	QString m_normal, m_birthdayOrAnniversary, m_dateString;
	int m_dateFormat;

};

#endif

