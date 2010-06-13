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

EventItemDelegate::EventItemDelegate(QObject* parent, QString normal, QString birthdayOrAnniversay, QString todo, int dtFormat, QString dtString)
    : QStyledItemDelegate(parent),
    m_normal(normal),
    m_birthdayOrAnniversary(birthdayOrAnniversay),
    m_todo(todo),
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
    QMap<QString, QVariant> data = value.toMap();
    QHash<QString,QString> dataHash;
    ulong s;

    int itemType = data["itemType"].toInt();
    switch (itemType) {
        case EventModel::HeaderItem:
            return data["title"].toString();
            break;
        case EventModel::NormalItem:
        case EventModel::BirthdayItem:
        case EventModel::AnniversaryItem:
            dataHash.insert("startDate", formattedDate(data["startDate"]));
            dataHash.insert("endDate", formattedDate(data["endDate"]));
            dataHash.insert("startTime", KGlobal::locale()->formatTime(data["startDate"].toTime()));
            dataHash.insert("endTime", KGlobal::locale()->formatTime(data["endDate"].toTime()));
            s = data["startDate"].toDateTime().secsTo(data["endDate"].toDateTime());
            dataHash.insert("duration", QString::number(s / 3600));
            dataHash.insert("summary", data["summary"].toString());
            dataHash.insert("description", data["description"].toString());
            dataHash.insert("location", data["location"].toString());
            dataHash.insert("yearsSince", data["yearsSince"].toString());
            dataHash.insert("resourceName", data["resourceName"].toString());
            dataHash.insert("tab", "\t");
            break;
        case EventModel::TodoItem:
            return formattedDate(data["dueDate"]) + " " + data["summary"].toString();
        default:
            break;
    }

    QString myText;
    if (data["isBirthday"].toBool() == TRUE || data["isAnniversary"].toBool() == TRUE) {
        myText = m_birthdayOrAnniversary;
    } else {
        myText = m_normal;
    }

    return KMacroExpander::expandMacros(myText, dataHash);
}

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

void EventItemDelegate::settingsChanged(QString normal, QString birthdayOrAnniversary, QString todo, int format, QString customString)
{
    m_normal = normal;
    m_birthdayOrAnniversary = birthdayOrAnniversary;
    m_todo= todo;
    m_dateFormat = format;
    m_dateString = customString;
}
