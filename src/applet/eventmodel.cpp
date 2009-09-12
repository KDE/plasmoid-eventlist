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
 *   Copyright (C) 2008 by Javier Goday <jgoday@gmail.com>
 *   Copyright (C) 2009 by gerdfleischer <gerdfleischer@web.de>
 */

#include "eventmodel.h"

// qt headers
#include <QDate>
#include <QStandardItem>

// kde headers
#include <KIcon>
#include <KGlobal>
#include <KLocale>
#include <KGlobalSettings>

#include <Plasma/Theme>

#include <KDebug>

EventModel::EventModel(QObject *parent, bool colors, int urgencyTime, QColor urgentClr, QColor passedClr, QColor birthdayClr, QColor anniversariesClr) : QStandardItemModel(parent),
	parentItem(0),
	todayItem(0),
	tomorrowItem(0),
	weekItem(0),
	monthItem(0),
	laterItem(0),
    useColors(colors),
    urgency(urgencyTime),
    urgentBg(urgentClr),
    passedFg(passedClr),
    birthdayBg(birthdayClr),
    anniversariesBg(anniversariesClr)
{
	parentItem = invisibleRootItem();
	initModel();
}

EventModel::~EventModel()
{
}

void EventModel::initModel()
{
    QFont bold = Plasma::Theme::defaultTheme()->font(Plasma::Theme::DefaultFont);
    bold.setBold(true);

	if (!todayItem) {
		todayItem = new QStandardItem();
		todayItem->setData(QVariant(i18n("Today")), Qt::DisplayRole);
		todayItem->setData(QVariant(QDateTime(QDate::currentDate())), EventModel::SortRole);
        todayItem->setData(QVariant(HeaderItem), EventModel::ItemRole);
        todayItem->setData(QVariant(QString()), EventModel::UIDRole);
        todayItem->setFont(bold);
	}

	if (!tomorrowItem) {
		tomorrowItem = new QStandardItem();
		tomorrowItem->setData(QVariant(i18n("Tomorrow")), Qt::DisplayRole);
		tomorrowItem->setData(QVariant(QDateTime(QDate::currentDate().addDays(1))), EventModel::SortRole);
        tomorrowItem->setData(QVariant(HeaderItem), EventModel::ItemRole);
        tomorrowItem->setData(QVariant(QString()), EventModel::UIDRole);
        tomorrowItem->setFont(bold);
	}

	if (!weekItem) {
		weekItem = new QStandardItem();
		weekItem->setData(QVariant(i18n("Week")), Qt::DisplayRole);
		weekItem->setData(QVariant(QDateTime(QDate::currentDate().addDays(2))), EventModel::SortRole);
        weekItem->setData(QVariant(HeaderItem), EventModel::ItemRole);
        weekItem->setData(QVariant(QString()), EventModel::UIDRole);
        weekItem->setFont(bold);
	}

	if (!monthItem) {
		monthItem = new QStandardItem();
		monthItem->setData(QVariant(i18n("Next 4 weeks")), Qt::DisplayRole);
		monthItem->setData(QVariant(QDateTime(QDate::currentDate().addDays(8))), EventModel::SortRole);
        monthItem->setData(QVariant(HeaderItem), EventModel::ItemRole);
        monthItem->setData(QVariant(QString()), EventModel::UIDRole);
        monthItem->setFont(bold);
		parentItem->appendRow(monthItem);
	}

	if (!laterItem) {
		laterItem = new QStandardItem();
		laterItem->setData(QVariant(i18n("Later")), Qt::DisplayRole);
		laterItem->setData(QVariant(QDateTime(QDate::currentDate().addDays(29))), EventModel::SortRole);
        laterItem->setData(QVariant(HeaderItem), EventModel::ItemRole);
        laterItem->setData(QVariant(QString()), EventModel::UIDRole);
        laterItem->setFont(bold);
	}
}

void EventModel::resetModel(bool isRunning)
{
	clear();
	parentItem = invisibleRootItem();
	todayItem = 0;
	tomorrowItem = 0;
	weekItem = 0;
	monthItem = 0;
	laterItem = 0;

    if (!isRunning) {
        QStandardItem *errorItem = new QStandardItem();
        errorItem->setData(QVariant(i18n("The Akonadi server is not running!")), Qt::DisplayRole);
        parentItem->appendRow(errorItem);
    } else {
        initModel();
    }
}

void EventModel::settingsChanged(bool colors, int urgencyTime, QColor urgentColor, QColor passedColor, QColor birthdayColor, QColor anniversariesColor)
{
    useColors = colors;
    urgency = urgencyTime;
    urgentBg = urgentColor;
    passedFg = passedColor;
    birthdayBg = birthdayColor;
    anniversariesBg = anniversariesColor;
}

// QList<int> EventModel::headerRows()
// {
// 	QList<int> rows;
// 	rows << todayItem->row() << tomorrowItem->row() << weekItem->row() << monthItem->row() << laterItem->row();
// 	return rows;
// }

void EventModel::addEventItem(const QMap <QString, QVariant> &values)
{
	QList<QVariant> data;
	if (values["recurs"].toBool()) {
		QList<QVariant> dtTimes = values["recurDates"].toList();
		foreach (QVariant eventDtTime, dtTimes) {
			QStandardItem *eventItem;
			eventItem = new QStandardItem();
			data.insert(StartDtTimePos, eventDtTime);
			data.insert(EndDtTimePos, values["endDate"]);
			data.insert(SummaryPos, values["summary"]);
			data.insert(DescriptionPos, values["description"]);
			data.insert(LocationPos, values["location"]);
			int n = eventDtTime.toDate().year() - values["startDate"].toDate().year();
			data.insert(YearsSincePos, QVariant(QString::number(n)));
			data.insert(BirthdayOrAnniversayPos, QVariant(values["isBirthday"].toBool() || values["isAnniversary"].toBool()));

            if (values["isBirthday"].toBool()) {
                if (useColors) eventItem->setBackground(QBrush(birthdayBg));
                eventItem->setData(QVariant(BirthdayItem), EventModel::ItemRole);
            } else if (values["isAnniversary"].toBool()) {
                if (useColors) eventItem->setBackground(QBrush(anniversariesBg));
                eventItem->setData(QVariant(AnniversaryItem), EventModel::ItemRole);
            }

			eventItem->setData(data, Qt::DisplayRole);
			eventItem->setData(eventDtTime, EventModel::SortRole);
            eventItem->setData(values["uid"], EventModel::UIDRole);
			eventItem->setToolTip(values ["tooltip"].toString());

			addItemRow(eventDtTime.toDate(), eventItem);
		}
	} else {
			QStandardItem *eventItem;
			eventItem = new QStandardItem();
			data.insert(StartDtTimePos, values["startDate"]);
			data.insert(EndDtTimePos, values["endDate"]);
			data.insert(SummaryPos, values["summary"]);
			data.insert(DescriptionPos, values["description"]);
			data.insert(LocationPos, values["location"]);
			data.insert(YearsSincePos, QVariant(QString()));
			data.insert(BirthdayOrAnniversayPos, QVariant(FALSE));

            eventItem->setData(QVariant(NormalItem), EventModel::ItemRole);
			eventItem->setData(data, Qt::DisplayRole);
			eventItem->setData(values["startDate"].toDateTime(), EventModel::SortRole);
            eventItem->setData(values["uid"], EventModel::UIDRole);
			eventItem->setToolTip(values ["tooltip"].toString());
            QDateTime itemDtTime = values["startDate"].toDateTime();
            if (useColors && itemDtTime > QDateTime::currentDateTime() && QDateTime::currentDateTime().secsTo(itemDtTime) < urgency * 60) {
                eventItem->setBackground(QBrush(urgentBg));
            } else if (useColors && QDateTime::currentDateTime() > itemDtTime) {
                eventItem->setForeground(QBrush(passedFg));
            }

			addItemRow(values["startDate"].toDate(), eventItem);
	}
}

void EventModel::addItemRow(QDate eventDate, QStandardItem *item)
{
    if (eventDate == QDate::currentDate()) {
        todayItem->appendRow(item);
        if (todayItem->row() == -1) parentItem->insertRow(figureRow(todayItem), todayItem);
    } else if (eventDate > QDate::currentDate() && eventDate <= QDate::currentDate().addDays(1)) {
        tomorrowItem->appendRow(item);
        if (!tomorrowItem->parent()) parentItem->insertRow(figureRow(tomorrowItem), tomorrowItem);
    } else if (eventDate > QDate::currentDate().addDays(1) && eventDate <= QDate::currentDate().addDays(7)) {
        weekItem->appendRow(item);
        if (weekItem->row() == -1) parentItem->insertRow(figureRow(weekItem), weekItem);
    } else if (eventDate > QDate::currentDate().addDays(7) && eventDate <= QDate::currentDate().addDays(28)) {
        monthItem->appendRow(item);
        if (monthItem->row() == -1) parentItem->insertRow(figureRow(monthItem), monthItem);
    } else if (eventDate > QDate::currentDate().addDays(28) && eventDate <= QDate::currentDate().addDays(365)) {
        laterItem->appendRow(item);
        if (laterItem->row() == -1) parentItem->appendRow(laterItem);
    }
}

int EventModel::figureRow(QStandardItem *headerItem)
{
    if (headerItem == todayItem) {
        return 0;
    } else if (headerItem == tomorrowItem) {
        if (todayItem->row() > -1)
            return 1;
        else
            return 0;
    } else if (headerItem == weekItem) {
        if (todayItem->row() > -1 && tomorrowItem->row() > -1)
            return 2;
        else if (todayItem->row() > -1 || tomorrowItem->row() > -1)
            return 1;
        else
            return 0;
    } else if (headerItem == monthItem) {
        if (todayItem->row() > -1 && tomorrowItem->row() > -1 && weekItem->row() > -1)
            return 3;
        else if (weekItem->row() > -1)
            return weekItem->row() + 1;
        else if (todayItem->row() > -1 || tomorrowItem->row() > -1)
            return 1;
        else
            return 0;
    }

    return -1;
}

