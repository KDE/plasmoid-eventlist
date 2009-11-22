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

#include "eventapplet.h"
#include "eventmodel.h"
#include "eventitemdelegate.h"
#include "korganizerappletutil.h"

// qt headers
#include <QComboBox>
#include <QGraphicsLinearLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QToolButton>
#include <QVBoxLayout>
#include <QTreeView>
#include <QModelIndex>
#include <QTimer>
#include <QAction>
#include <QCheckBox>
#include <QtDebug>

// kde headers
#include <KColorScheme>
#include <KConfigDialog>
#include <KIcon>
#include <KIconLoader>

// plasma headers
#include <Plasma/Theme>

#include <akonadi/agentinstance.h>

K_EXPORT_PLASMA_APPLET(eventapplet, EventApplet)

static const char *CATEGORY_SOURCE = "Categories";
static const char *COLOR_SOURCE    = "Colors";
static const char *EVENT_SOURCE    = "Events";
static const char *SERVERSTATE_SOURCE = "ServerState";

EventApplet::EventApplet(QObject *parent, const QVariantList &args) :
    Plasma::PopupApplet(parent, args),
    m_engine(0),
    m_graphicsWidget(0),
    m_view(0),
    m_delegate(0),
    m_formatConfigUi(),
    m_colorConfigUi(),
    m_passedTimer(0),
    m_manager(0)
{
    KGlobal::locale()->insertCatalog("libkcal");
    KGlobal::locale()->insertCatalog("eventapplet");
    setBackgroundHints(DefaultBackground);
    setAspectRatioMode(Plasma::IgnoreAspectRatio);
    setHasConfigurationInterface(true);

    setPopupIcon("view-pim-tasks");
}

EventApplet::~EventApplet()
{
    delete m_view;
}

void EventApplet::init()
{
    KConfigGroup cg = config();
    QString normalEventFormat = cg.readEntry("NormalEventFormat", QString("%{startDate} %{startTime} %{summary}"));
    QString recurringEventFormat = cg.readEntry("RecurringEventsFormat", QString("%{startDate} %{yearsSince}. %{summary}"));
    int dtFormat = cg.readEntry("DateFormat", ShortDateFormat);
    QString dtString = cg.readEntry("CustomDateFormat", QString("dd.MM."));
    int period = cg.readEntry("Period", 365);

    m_useColors = cg.readEntry("UseColors", TRUE);
    m_checkInterval = cg.readEntry("CheckInterval", 5);
    m_urgency = cg.readEntry("UrgencyTime", 15);

    QList<QColor> colors;
    QColor urgentColor = QColor(cg.readEntry("UrgentColor", QString("#FF0000")));
    urgentColor.setAlphaF(cg.readEntry("UrgentOpacity", 10)/100.0);
    colors.insert(urgentColorPos, urgentColor);
    QColor passedColor = QColor(cg.readEntry("PassedColor", QString("#C3C3C3")));
    colors.insert(passedColorPos, passedColor);
    
    QColor birthdayColor = QColor(cg.readEntry("BirthdayColor", QString("#C0FFC0")));
    birthdayColor.setAlphaF(cg.readEntry("BirthdayOpacity", 10)/100.0);
    colors.insert(birthdayColorPos, birthdayColor);
    QColor anniversariesColor = QColor(cg.readEntry("AnniversariesColor", QString("#ABFFEA")));
    anniversariesColor.setAlphaF(cg.readEntry("AnniversariesOpacity", 10)/100.0);
    colors.insert(anniversariesColorPos, anniversariesColor);

    m_urgentBg = urgentColor;
    m_passedFg = passedColor;

    m_model = new EventModel(this, m_useColors, m_urgency, colors, period);
    m_delegate = new EventItemDelegate(this, normalEventFormat, recurringEventFormat, dtFormat, dtString);

    m_manager = Akonadi::AgentManager::self();

    graphicsWidget();

    m_engine = dataEngine("events");
    if (m_engine) {
        m_engine->connectSource(EVENT_SOURCE, this);
        m_engine->connectSource(CATEGORY_SOURCE, this);
        m_engine->connectSource(COLOR_SOURCE, this);
        m_engine->connectSource(SERVERSTATE_SOURCE, this);
    }

    setupActions();

    m_passedTimer = new QTimer();
    connect(m_passedTimer, SIGNAL(timeout()), this, SLOT(passedTimerExpired()));
}

void EventApplet::dataUpdated(const QString &name, const Plasma::DataEngine::Data &data)
{
    if (QString::compare(name, EVENT_SOURCE) == 0) {
        updateEventList(data["events"].toList());
    } else if (QString::compare(name, SERVERSTATE_SOURCE) == 0) {
        updateAkonadiState(data["serverrunning"].toBool());
    }
//     else if (QString::compare(name, CATEGORY_SOURCE) == 0) {
//         updateCategories(data["categories"].toStringList());
//     } else if (QString::compare(name, COLOR_SOURCE) == 0) {
//         updateColors(data["colors"].toMap());
//     }
}

QGraphicsWidget *EventApplet::graphicsWidget()
{
	if (!m_graphicsWidget) {
		m_graphicsWidget = new QGraphicsWidget(this);
		m_graphicsWidget->setMinimumSize(200, 125);
		m_graphicsWidget->setPreferredSize(350, 200);

	// Get colors
	QColor textColor = Plasma::Theme::defaultTheme()->color(Plasma::Theme::TextColor);
	QColor baseColor = Plasma::Theme::defaultTheme()->color(Plasma::Theme::BackgroundColor);
	QColor altBaseColor = baseColor.darker(150);
//	int green = altBaseColor.green() * 1.8;
//	altBaseColor.setGreen( green > 255 ? 255 : green ); // tint green
	QColor buttonColor = altBaseColor;
	baseColor.setAlpha(50);
	altBaseColor.setAlpha(50);
	buttonColor.setAlpha(150);
//	m_colorSubItemLabels = textColor;
//	m_colorSubItemLabels.setAlpha( 170 );

	// Set colors
	QPalette p = palette();
	p.setColor( QPalette::Base, baseColor );
	p.setColor( QPalette::AlternateBase, altBaseColor );
	p.setColor( QPalette::Button, buttonColor );
	p.setColor( QPalette::Foreground, textColor );
	p.setColor( QPalette::Text, textColor );

  m_view = new Plasma::TreeView(); //EventView();
		m_view->setModel(m_model);
		m_view->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );

		QTreeView *treeView = (QTreeView *)m_view->widget();
		treeView->setItemDelegate(m_delegate);
		treeView->setAlternatingRowColors( true );
		treeView->setAllColumnsShowFocus( false );
		treeView->setPalette( p );
		treeView->viewport()->setAutoFillBackground(false);
		treeView->viewport()->setPalette( p );
		treeView->setRootIsDecorated( false );
		treeView->setAnimated( true );
		treeView->setSortingEnabled( false );
		treeView->setHeaderHidden(true);
		treeView->setWordWrap( true );
		treeView->setFrameShape( QFrame::StyledPanel );
		treeView->setEditTriggers( QAbstractItemView::NoEditTriggers );
		treeView->setSelectionMode( QAbstractItemView::NoSelection );
		treeView->setSelectionBehavior( QAbstractItemView::SelectRows );
		treeView->header()->setCascadingSectionResizes( true );
		treeView->header()->setResizeMode( QHeaderView::Interactive );
		treeView->header()->setSortIndicator( 2, Qt::AscendingOrder );
		
        connect(treeView, SIGNAL(doubleClicked(const QModelIndex &)),
                SLOT(slotOpenEvent(const QModelIndex &)));

        Plasma::Label *title = new Plasma::Label();
        title->setText(i18n("Events from Akonadi resources"));
        QFont bold = font();
        bold.setBold(true);
        title->setFont(bold);

        QGraphicsLinearLayout *layout = new QGraphicsLinearLayout(Qt::Vertical);
        layout->addItem(title);
        layout->addItem(m_view);

        m_graphicsWidget->setLayout(layout);
        registerAsDragHandle(m_graphicsWidget);
    }

    return m_graphicsWidget;
}

// void EventApplet::updateCategories(const QStringList &categories)
// {
//     m_types->addItem(QString());
// 
//     foreach(const QString &category, categories) {
//         m_types->addItem(category);
//     }
// }

// void EventApplet::updateColors(const QMap <QString, QVariant> &colors)
// {
//     foreach(const QString &category, colors.keys()) {
//         if (colors[category].value<QColor>().value() > 0) {
//             m_types->setItemData(m_types->findText(category),
//                                 colors[category].value<QColor>(),
//                                 Qt::DecorationRole);
// 
//             // search for the categories event's to set the category color
//             foreach(const QModelIndex &index, m_model->match(m_model->index(0, 0),
//                                                             Qt::UserRole, category, -1)) {
//                 m_model->setCategory(index, colors[category].value<QColor>());
//             }
//         }
//     }
// }

void EventApplet::updateEventList(const QList <QVariant> &events)
{
    Plasma::DataEngine::Data data = m_engine->query(SERVERSTATE_SOURCE);
    bool isRunning = data["serverrunning"].toBool();
    
    m_model->resetModel(isRunning);

    if (isRunning) {
        foreach (const QVariant &event, events) {
            QMap <QString, QVariant> values = event.toMap();
            m_model->addEventItem(values);
        }

        QTreeView *treeView = (QTreeView *)m_view->widget();
        m_model->setSortRole(EventModel::SortRole);
        m_model->sort(0, Qt::AscendingOrder);
        treeView->expandAll();
        if (m_useColors) m_passedTimer->start(m_checkInterval * 60 * 1000);
    }
}

void EventApplet::updateAkonadiState(bool isRunning)
{
    if (!isRunning)
        updateEventList(QList<QVariant>());
}

void EventApplet::slotOpenEvent(const QModelIndex &index)
{
    QString uid = m_model->data(index, EventModel::UIDRole).toString();
    if (!uid.isEmpty())
        KOrganizerAppletUtil::showEvent(uid);
}

void EventApplet::openEventFromMenu()
{
    QTreeView *treeView = (QTreeView *)m_view->widget();
    slotOpenEvent(treeView->currentIndex());
}

void EventApplet::slotAddEvent()
{
    KOrganizerAppletUtil::showAddEvent();
}

void EventApplet::passedTimerExpired()
{
    colorizeUrgentAndPassed();
    if (m_useColors) m_passedTimer->start(m_checkInterval * 60 * 1000);
}

void EventApplet::setupActions()
{
    actions.clear();

    QAction *openEvent = new QAction(i18n("Open current event"), this);
    openEvent->setIcon(KIcon("document-edit"));
    connect(openEvent, SIGNAL(triggered()), this, SLOT(openEventFromMenu()));
    actions.append(openEvent);

    QAction *newEvent = new QAction(i18n("Add new event"), this);
    newEvent->setIcon(KIcon("appointment-new"));
    connect(newEvent, SIGNAL(triggered()), this, SLOT(slotAddEvent()));
    actions.append(newEvent);

    QAction *selectResources = new QAction(i18n("Select shown resources"), this);
    selectResources->setIcon(KIcon("view-calendar-tasks"));
    connect(selectResources, SIGNAL(triggered()), this, SLOT(setShownResources()));
    actions.append(selectResources);
}

void EventApplet::setShownResources()
{
    KDialog *dialog = new KDialog();
    dialog->setCaption(i18n("Select Resources"));
    dialog->setButtons(KDialog::Ok | KDialog::Cancel);

    QWidget *widget = new QWidget(dialog);
    QVBoxLayout *layout = new QVBoxLayout();
    
    Akonadi::AgentInstance::List inst = m_manager->instances();
    foreach (const Akonadi::AgentInstance &in, inst) {
        qDebug() << "Type:" << in.name() << in.identifier();
        QCheckBox *res = new QCheckBox(in.name());
        res->setChecked(TRUE);
        layout->addWidget(res);
    }

    widget->setLayout(layout);
    dialog->setMainWidget(widget);

    if (dialog->exec() == QDialog::Accepted) {
    }
}

QList<QAction *> EventApplet::contextualActions()
{
    return actions;
}

void EventApplet::createConfigurationInterface(KConfigDialog *parent)
{
    QWidget *formatWidget = new QWidget();
    m_formatConfigUi.setupUi(formatWidget);
    QWidget *colorWidget = new QWidget();
    m_colorConfigUi.setupUi(colorWidget);
    parent->setButtons(KDialog::Ok | KDialog::Apply | KDialog::Cancel);

    connect(parent, SIGNAL(applyClicked()), this, SLOT(configAccepted()));
    connect(parent, SIGNAL(okClicked()), this, SLOT(configAccepted()));

    parent->addPage(formatWidget, i18n("Text Format"), icon());
    parent->addPage(colorWidget, i18n("Colors"), "fill-color");

    KConfigGroup cg = config();

	m_formatConfigUi.normalEventEdit->setText(cg.readEntry("NormalEventFormat", QString("%{startDate} %{startTime} %{summary}")));
	m_formatConfigUi.recurringEventsEdit->setText(cg.readEntry("RecurringEventsFormat", QString("%{startDate} %{yearsSince}. %{summary}")));
	m_formatConfigUi.dateFormatBox->setCurrentIndex(cg.readEntry("DateFormat", ShortDateFormat));
	m_formatConfigUi.customFormatEdit->setText(cg.readEntry("CustomDateFormat", QString("dd.MM.")));
    m_formatConfigUi.periodBox->setValue(cg.readEntry("Period", 365));

    m_colorConfigUi.useColorsCheckBox->setChecked(cg.readEntry("UseColors", TRUE));
    m_colorConfigUi.urgencyBox->setValue(cg.readEntry("UrgencyTime", 15));
    m_colorConfigUi.checkIntervalBox->setValue(cg.readEntry("CheckInterval", 5));

    m_colorConfigUi.startedColorButton->setColor(QColor(cg.readEntry("PassedColor", QString("#C3C3C3"))));
    m_colorConfigUi.urgentColorButton->setColor(QColor(cg.readEntry("UrgentColor", QString("#FF0000"))));
    m_colorConfigUi.urgentOpacity->setValue(cg.readEntry("UrgentOpacity", 10));

    m_colorConfigUi.birthdayColorButton->setColor(QColor(cg.readEntry("BirthdayColor", QString("#C0FFC0"))));
    m_colorConfigUi.birthdayOpacity->setValue(cg.readEntry("BirthdayOpacity", 10));
    m_colorConfigUi.anniversariesColorButton->setColor(QColor(cg.readEntry("AnniversariesColor", QString("#ABFFEA"))));
    m_colorConfigUi.anniversariesOpacity->setValue(cg.readEntry("AnniversariesOpacity", 10));
}

void EventApplet::configAccepted()
{
	KConfigGroup cg = config();

	QString normalEventFormat = m_formatConfigUi.normalEventEdit->text();
	cg.writeEntry("NormalEventFormat", normalEventFormat);
	QString recurringEventsFormat = m_formatConfigUi.recurringEventsEdit->text();
	cg.writeEntry("RecurringEventsFormat", recurringEventsFormat);
	int dateFormat = m_formatConfigUi.dateFormatBox->currentIndex();
	cg.writeEntry("DateFormat", dateFormat);
	QString customString = m_formatConfigUi.customFormatEdit->text();
	cg.writeEntry("CustomDateFormat", customString);

	m_delegate->settingsChanged(normalEventFormat, recurringEventsFormat, dateFormat, customString);

    int oldPeriod = cg.readEntry("Period", 365);
    int period = m_formatConfigUi.periodBox->value();
    cg.writeEntry("Period", period);

    m_useColors = m_colorConfigUi.useColorsCheckBox->isChecked();
    cg.writeEntry("UseColors", m_useColors);
    m_urgency = m_colorConfigUi.urgencyBox->value();
    cg.writeEntry("UrgencyTime", m_urgency);
    m_checkInterval = m_colorConfigUi.checkIntervalBox->value();
    cg.writeEntry("CheckInterval", m_checkInterval);
    
    QList<QColor> colors;

    m_urgentBg = m_colorConfigUi.urgentColorButton->color();
    int urgentOpacity = m_colorConfigUi.urgentOpacity->value();
    cg.writeEntry("UrgentColor", m_urgentBg.name());
    cg.writeEntry("UrgentOpacity", urgentOpacity);
    m_urgentBg.setAlphaF(urgentOpacity/100.0);
    colors.insert(urgentColorPos, m_urgentBg);

    m_passedFg = m_colorConfigUi.startedColorButton->color();
    cg.writeEntry("PassedColor", m_passedFg.name());
    colors.insert(passedColorPos, m_passedFg);
    
    QColor birthdayColor = m_colorConfigUi.birthdayColorButton->color();
    int birthdayOpacity = m_colorConfigUi.birthdayOpacity->value();
    cg.writeEntry("BirthdayColor", birthdayColor.name());
    cg.writeEntry("BirthdayOpacity", birthdayOpacity);
    birthdayColor.setAlphaF(birthdayOpacity/100.0);
    colors.insert(birthdayColorPos, birthdayColor);

    QColor anniversariesColor = m_colorConfigUi.anniversariesColorButton->color();
    int anniversariesOpacity = m_colorConfigUi.anniversariesOpacity->value();
    cg.writeEntry("AnniversariesColor", anniversariesColor.name());
    cg.writeEntry("AnniversariesOpacity", anniversariesOpacity);
    anniversariesColor.setAlphaF(anniversariesOpacity/100.0);
    colors.insert(anniversariesColorPos, anniversariesColor);

    if (m_useColors) {
        m_passedTimer->start(m_checkInterval * 60 * 1000);
    } else {
        m_passedTimer->stop();
    }

    m_model->settingsChanged(m_useColors, m_urgency, colors, period);

    if (oldPeriod != period) { //just rebuild model if period changed
        Plasma::DataEngine::Data data = m_engine->query(EVENT_SOURCE);
        updateEventList(data["events"].toList());
    } else {
        colorizeBirthdayAndAnniversaries(birthdayColor, anniversariesColor);
        colorizeUrgentAndPassed();
    }

    m_view->setModel(m_model);

	emit configNeedsSaving();
}

void EventApplet::colorizeBirthdayAndAnniversaries(QColor birthdayColor, QColor anniversariesColor)
{
    int headerRows = m_model->rowCount(QModelIndex());
    for (int r = 0; r < headerRows; ++r) {
        QModelIndex headerIndex = m_model->index(r, 0, QModelIndex());
        int childRows = m_model->rowCount(headerIndex);
        for (int c = 0; c < childRows; ++c) {
            QModelIndex index = m_model->index(c, 0, headerIndex);
            int itemRole = m_model->data(index, EventModel::ItemRole).toInt();
            if (m_useColors && itemRole == BirthdayItem) {
                m_model->setData(index, QVariant(QBrush(birthdayColor)), Qt::BackgroundRole);
            } else if (m_useColors && itemRole == AnniversaryItem) {
                m_model->setData(index, QVariant(QBrush(anniversariesColor)), Qt::BackgroundRole);
            } else if (!m_useColors && (itemRole == BirthdayItem || itemRole == AnniversaryItem)) {
                m_model->setData(index, QVariant(QBrush(Qt::transparent)), Qt::BackgroundRole);
            }
        }
    }
}

void EventApplet::colorizeUrgentAndPassed()
{
    QDateTime now = QDateTime::currentDateTime();

    QModelIndex todayIndex = m_model->index(0, 0, QModelIndex());
    int todayRows = m_model->rowCount(todayIndex);
    for (int t = 0; t < todayRows; ++t) {
        QModelIndex index = m_model->index(t, 0, todayIndex);
        int itemRole = m_model->data(index, EventModel::ItemRole).toInt();
        QDateTime itemDtTime = m_model->data(index, EventModel::SortRole).toDateTime();
        if (itemRole == NormalItem) {
            if (m_useColors && itemDtTime > now && now.secsTo(itemDtTime) < m_urgency * 60) {
                m_model->setData(index, QVariant(QBrush(m_urgentBg)), Qt::BackgroundRole);
            } else if (m_useColors && now > itemDtTime) {
                m_model->setData(index, QVariant(QBrush(m_passedFg)), Qt::ForegroundRole);
                m_model->setData(index, QVariant(QBrush(Qt::transparent)), Qt::BackgroundRole);
            } else {
                m_model->setData(index, QVariant(QBrush(Qt::transparent)), Qt::BackgroundRole);
                QColor defaultTextColor = Plasma::Theme::defaultTheme()->color(Plasma::Theme::TextColor);
                m_model->setData(index, QVariant(QBrush(defaultTextColor)), Qt::ForegroundRole);
            }
        }
    }

    QModelIndex tomorrowIndex = m_model->index(1, 0, QModelIndex());
    int tomorrowRows = m_model->rowCount(tomorrowIndex);
    for (int w = 0; w < tomorrowRows; ++w) {
        QModelIndex index = m_model->index(w, 0, tomorrowIndex);
        int itemRole = m_model->data(index, EventModel::ItemRole).toInt();
        QDateTime itemDtTime = m_model->data(index, EventModel::SortRole).toDateTime();
        if (itemRole == NormalItem) {
            if (m_useColors && now.secsTo(itemDtTime) < m_urgency * 60) {
                m_model->setData(index, QVariant(QBrush(m_urgentBg)), Qt::BackgroundRole);
            } else {
                m_model->setData(index, QVariant(QBrush(Qt::transparent)), Qt::BackgroundRole);
            }
        }
    }
}
