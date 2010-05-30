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

#include "eventitemdelegate.h"
#include "eventmodel.h"

// #include <Plasma/Svg>
// #include <Plasma/Theme>

#include <KGlobal>
#include <KLocale>
#include <KMacroExpanderBase>

#include <QDateTime>
#include <QHash>
// #include <QRectF>
// #include <QStyleOptionViewItem>

EventItemDelegate::EventItemDelegate(QObject* parent, QString normal, QString birthdayOrAnniversay, int dtFormat, QString dtString)
	: QStyledItemDelegate(parent),
	m_normal(normal),
	m_birthdayOrAnniversary(birthdayOrAnniversay),
	m_dateString(dtString),
	m_dateFormat(dtFormat)
{
}

EventItemDelegate::~EventItemDelegate()
{
}

QString EventItemDelegate::displayText(const QVariant &value, const QLocale &locale) const
{
	Q_UNUSED(locale);
	if (value.type() == QVariant::String)
		return value.toString();

	QList<QVariant> data = value.toList();
	QHash<QString,QString> dataHash;
	dataHash.insert("startDate", formattedDate(data.at(StartDtTimePos)));
	dataHash.insert("endDate", formattedDate(data.at(EndDtTimePos)));
	dataHash.insert("startTime", KGlobal::locale()->formatTime(data.at(StartDtTimePos).toTime()));
	dataHash.insert("endTime", KGlobal::locale()->formatTime(data.at(StartDtTimePos).toTime()));
	ulong s = data.at(StartDtTimePos).toDateTime().secsTo(data.at(EndDtTimePos).toDateTime());
	dataHash.insert("duration", QString::number(s / 3600));
	dataHash.insert("summary", data.at(SummaryPos).toString());
	dataHash.insert("description", data.at(DescriptionPos).toString());
	dataHash.insert("location", data.at(LocationPos).toString());
	dataHash.insert("yearsSince", data.at(YearsSincePos).toString());
    dataHash.insert("resourceName", data.at(resourceNamePos).toString());
	dataHash.insert("tab", "\t");

	QString myText;
	if (data.at(BirthdayOrAnniversaryPos).toBool() == TRUE) {
		myText = m_birthdayOrAnniversary;
	} else {
		myText = m_normal;
	}

	return KMacroExpander::expandMacros(myText, dataHash);
}

// void EventItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
//                                                 const QModelIndex &index) const
// {
// 	const QAbstractItemModel *model = index.model();
// 	QStyleOptionViewItem textOption = option;
// 	Plasma::Svg *svg = new Plasma::Svg();
// 	svg->setImagePath("widgets/calendar");
// 	svg->setContainsMultipleImages(true);
// 	QRect r = textOption.rect;
// 	svg->paint(painter, QRectF(r), "active");
// 
// // 	painter->drawText(textOption.rect(), 
// 	QStyledItemDelegate::paint(painter, textOption, index);
// }

QString EventItemDelegate::formattedDate(const QVariant &dtTime) const
{
	QString date;
	if (dtTime.toDateTime().isValid()) {
		switch (m_dateFormat) {
			case ShortDateFormat:
				date = KGlobal::locale()->formatDate(dtTime.toDate(), KLocale::ShortDate);
				break;
			case LongDateFormat:
				date = KGlobal::locale()->formatDate(dtTime.toDate(), KLocale::LongDate);
				break;
			case CustomDateFormat:
				date = dtTime.toDate().toString(m_dateString);
				break;
		}
	}

	return date;
}

void EventItemDelegate::settingsChanged(QString normal, QString birthdayOrAnniversary, int format, QString customString)
{
	m_normal = normal;
	m_birthdayOrAnniversary = birthdayOrAnniversary;
	m_dateFormat = format;
	m_dateString = customString;
}
