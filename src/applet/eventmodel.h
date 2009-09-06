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

#ifndef EVENTMODEL_H
#define EVENTMODEL_H

// qt headers
#include <QStandardItemModel>
#include <QColor>
#include <QHash>
#include <QString>

class QStandardItem;

static const int ShortDateFormat = 0;
static const int LongDateFormat = 1;
static const int CustomDateFormat = 2;

static const int StartDtTimePos = 0;
static const int EndDtTimePos = 1;
static const int SummaryPos = 2;
static const int DescriptionPos = 3;
static const int LocationPos = 4;
static const int YearsSincePos = 5;
static const int BirthdayOrAnniversayPos = 6;
static const int UIDPos = 7;

static const int HeaderItem = 0;
static const int NormalItem = 1;
static const int BirthdayItem = 2;
static const int AnniversaryItem = 3;

/**
* Model of the view
* Categorizes the events using the startDate property
*/
class EventModel : public QStandardItemModel
{
    Q_OBJECT
public:
    enum EventRole {
		SortRole = Qt::UserRole + 1,
        UIDRole,
        ItemRole
    };

    enum EventCategoryType {
        ByStartDate = 0,
        ByDueDate = 1
    };

    EventModel(QObject *parent = 0, bool colors = TRUE, int urgencyTime = 15, QColor urgentClr = QColor(), QColor passedClr = QColor(), QColor birthdayClr = QColor(), QColor anniversariesClr = QColor());
    ~EventModel();

public:
	void setDateFormat(int format, QString string);
	void initModel();
	void resetModel(bool isRunning);
    void settingsChanged(bool colors, int urgencyTime, QColor urgentColor, QColor passedColor, QColor birthdayColor, QColor anniversariesColor);
//	QList<int> headerRows();

public slots:
    void addEventItem(const QMap <QString, QVariant> &values);

private:
	void addItemRow(QDate eventDate, QStandardItem *items);

private:
	QStandardItem *parentItem, *todayItem, *tomorrowItem, *weekItem, *monthItem, *laterItem;
    bool useColors;
    int urgency;
    QColor urgentBg, passedFg, birthdayBg, anniversariesBg;
};

#endif
