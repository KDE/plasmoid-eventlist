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
#include "eventfiltermodel.h"
#include "eventitemdelegate.h"
#include "korganizerappletutil.h"
#include "eventtreeview.h"

// qt headers
#include <QComboBox>
#include <QGraphicsLinearLayout>
#include <QGraphicsProxyWidget>
#include <QHBoxLayout>
#include <QLabel>
#include <QToolButton>
#include <QVBoxLayout>
#include <QTreeView>
#include <QModelIndex>
#include <QTimer>
#include <QAction>
#include <QCheckBox>
#include <QDateTime>
#include <QPoint>
#include <QCursor>

// kde headers
#include <KColorScheme>
#include <KConfigDialog>
#include <KIcon>
#include <KIconLoader>

// plasma headers
#include <Plasma/Theme>

#include <akonadi/control.h>
#include <akonadi/agentinstance.h>
#include <akonadi/servermanager.h>

K_EXPORT_PLASMA_APPLET(events, EventApplet)

static const int MAX_RETRIES = 12;

EventApplet::EventApplet(QObject *parent, const QVariantList &args) :
    Plasma::PopupApplet(parent, args),
    m_graphicsWidget(0),
    m_view(0),
    m_delegate(0),
    m_formatConfigUi(),
    m_colorConfigUi(),
    m_timer(0),
    m_agentManager(0)
{
    KGlobal::locale()->insertCatalog("libkcal");
    KGlobal::locale()->insertCatalog("eventapplet");
    setBackgroundHints(DefaultBackground);
    setAspectRatioMode(Plasma::IgnoreAspectRatio);
    setHasConfigurationInterface(true);

    setPopupIcon("view-pim-tasks");

    Akonadi::ServerManager::start();
}

EventApplet::~EventApplet()
{
    delete m_view;
}

void EventApplet::init()
{
    KConfigGroup cg = config();

    disabledResources = cg.readEntry("DisabledResources", QStringList());

    QString normalEventFormat = cg.readEntry("NormalEventFormat", QString("%{startDate} %{startTime} %{summary}"));
    QString recurringEventFormat = cg.readEntry("RecurringEventsFormat", QString("%{startDate} %{yearsSince}. %{summary}"));
    QString todoFormat = cg.readEntry("TodoFormat", QString("%{dueDate} %{summary}"));
    int dtFormat = cg.readEntry("DateFormat", ShortDateFormat);
    QString dtString = cg.readEntry("CustomDateFormat", QString("dd.MM."));
    m_period = cg.readEntry("Period", 365);

    m_urgency = cg.readEntry("UrgencyTime", 15);

    QColor urgentColor = QColor(cg.readEntry("UrgentColor", QString("#FF0000")));
    urgentColor.setAlphaF(cg.readEntry("UrgentOpacity", 10)/100.0);
    m_colors.insert(urgentColorPos, urgentColor);
    QColor passedColor = QColor(cg.readEntry("PassedColor", QString("#C3C3C3")));
    m_colors.insert(passedColorPos, passedColor);
    
    QColor birthdayColor = QColor(cg.readEntry("BirthdayColor", QString("#C0FFC0")));
    birthdayColor.setAlphaF(cg.readEntry("BirthdayOpacity", 10)/100.0);
    m_colors.insert(birthdayColorPos, birthdayColor);
    QColor anniversariesColor = QColor(cg.readEntry("AnniversariesColor", QString("#ABFFEA")));
    anniversariesColor.setAlphaF(cg.readEntry("AnniversariesOpacity", 10)/100.0);
    m_colors.insert(anniversariesColorPos, anniversariesColor);
    QColor todoColor = QColor(cg.readEntry("TodoColor", QString("#FFD235")));
    todoColor.setAlphaF(cg.readEntry("TodoOpacity", 10)/100.0);
    m_colors.insert(todoColorPos, todoColor);

    m_urgentBg = urgentColor;
    m_passedFg = passedColor;

    m_delegate = new EventItemDelegate(this, normalEventFormat, recurringEventFormat, todoFormat, dtFormat, dtString);

    graphicsWidget();

    Plasma::ToolTipManager::self()->registerWidget(this);
    createToolTip();

    lastCheckTime = QDateTime::currentDateTime();
    m_timer = new QTimer();
    connect(m_timer, SIGNAL(timeout()), this, SLOT(timerExpired()));
    QTimer::singleShot(0, this, SLOT(setupModel()));
}

void EventApplet::setupModel()
{
    Akonadi::Control::widgetNeedsAkonadi(m_view);

    Akonadi::Control::start();
    
    m_agentManager = Akonadi::AgentManager::self();

    m_model = new EventModel(this, m_urgency, m_colors, m_period);
    m_model->setSortRole(EventModel::SortRole);
    m_model->sort(0, Qt::AscendingOrder);

    m_filterModel = new EventFilterModel(this);
    m_filterModel->setPeriod(m_period);
    m_filterModel->setExcludedResources(disabledResources);
    m_filterModel->setDynamicSortFilter(TRUE);
    m_filterModel->setSourceModel(m_model);

    m_view->setModel(m_filterModel);
    m_view->expandAll();
    connect(m_model, SIGNAL(modelNeedsExpanding()), m_view, SLOT(expandAll()));

    setupActions();
    m_timer->start(2 * 60 * 1000);

    connect(Akonadi::ServerManager::self(), SIGNAL(started()), this, SLOT(akonadiStatusChanged()));
}

void EventApplet::akonadiStatusChanged()
{
    m_model->resetModel();
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
        QColor buttonColor = altBaseColor;
        baseColor.setAlpha(50);
        altBaseColor.setAlpha(50);
        buttonColor.setAlpha(150);

        // Set colors
        QPalette p = palette();
        p.setColor( QPalette::Base, baseColor );
        p.setColor( QPalette::AlternateBase, altBaseColor );
        p.setColor( QPalette::Button, buttonColor );
        p.setColor( QPalette::Foreground, textColor );
        p.setColor( QPalette::Text, textColor );

        proxyWidget = new QGraphicsProxyWidget();
        m_view = new EventTreeView();
        proxyWidget->setWidget(m_view);
        proxyWidget->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );

        m_view->setItemDelegate(m_delegate);
        m_view->setPalette(p);
        m_view->viewport()->setAutoFillBackground(false);
        m_view->viewport()->setPalette( p );

        connect(m_view, SIGNAL(doubleClicked(const QModelIndex &)),
                SLOT(slotOpenEvent(const QModelIndex &)));
        connect(m_view, SIGNAL(tooltipUpdated(QString)),
                SLOT(slotUpdateTooltip(QString)));

        Plasma::Label *title = new Plasma::Label();
        title->setText(i18n("Upcoming Events"));
        QFont bold = font();
        bold.setBold(true);
        title->setFont(bold);

        layout = new QGraphicsLinearLayout(Qt::Vertical);
        layout->addItem(title);
        layout->addItem(proxyWidget);

        m_graphicsWidget->setLayout(layout);
        registerAsDragHandle(m_graphicsWidget);
    }

    return m_graphicsWidget;
}

void EventApplet::slotOpenEvent(const QModelIndex &index)
{
    QString uid = m_model->data(index, EventModel::UIDRole).toString();
    if (!uid.isEmpty())
        KOrganizerAppletUtil::showEvent(uid);
}

void EventApplet::openEventFromMenu()
{
    slotOpenEvent(m_view->currentIndex());
}

void EventApplet::slotAddEvent()
{
    KOrganizerAppletUtil::showAddEvent();
}

void EventApplet::timerExpired()
{
    if (lastCheckTime.date().daysTo(QDate::currentDate()) < 0) {
        m_model->resetModel();
    } else {
        colorizeUrgentAndPassed();
    }

    lastCheckTime = QDateTime::currentDateTime();
    m_timer->start(2 * 60 * 1000);
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
    
    Akonadi::AgentInstance::List instList = m_agentManager->instances();
    foreach (const Akonadi::AgentInstance &inst, instList) {
        QStringList agentMimeTypes = inst.type().mimeTypes();
        if (agentMimeTypes.contains("application/x-vnd.akonadi.calendar.event")) {
            QCheckBox *resBox = new QCheckBox(inst.name());
            resBox->setChecked(!disabledResources.contains(inst.identifier()));
            resBox->setProperty("identifier", inst.identifier());
            layout->addWidget(resBox);
        }
    }

    widget->setLayout(layout);
    dialog->setMainWidget(widget);

    if (dialog->exec() == QDialog::Accepted) {
        disabledResources.clear();
        QList<QCheckBox *> resList = widget->findChildren<QCheckBox *>();
        foreach (QCheckBox *box, resList) {
            if (box->isChecked() == FALSE) {
                disabledResources.append(box->property("identifier").toString());
            }
        }

        KConfigGroup cg = config();
        cg.writeEntry("DisabledResources", disabledResources);
        emit configNeedsSaving();

        m_filterModel->setExcludedResources(disabledResources);
        m_view->expandAll();
    }
}

QList<QAction *> EventApplet::contextualActions()
{
    return actions;
}

void EventApplet::slotUpdateTooltip(QString text)
{
    tooltip.setSubText(text);
    Plasma::ToolTipManager::self()->setContent(this, tooltip);
}

void EventApplet::createToolTip()
{
    tooltip = Plasma::ToolTipContent(i18n("Upcoming Events"), "", KIcon("view-pim-tasks"));
    tooltip.setMainText(i18n("Upcoming events from Akonadi resources"));
    tooltip.setAutohide(FALSE);
}

void EventApplet::hoverMoveEvent(QGraphicsSceneHoverEvent *event)
{
    Q_UNUSED(event);
    createToolTip();
    Plasma::ToolTipManager::self()->setContent(this, tooltip);
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
    m_formatConfigUi.todoEdit->setText(cg.readEntry("TodoFormat", QString("%{dueDate} %{summary}")));
    m_formatConfigUi.dateFormatBox->setCurrentIndex(cg.readEntry("DateFormat", ShortDateFormat));
    m_formatConfigUi.customFormatEdit->setText(cg.readEntry("CustomDateFormat", QString("dd.MM.")));
    m_formatConfigUi.periodBox->setValue(cg.readEntry("Period", 365));

    m_colorConfigUi.urgencyBox->setValue(cg.readEntry("UrgencyTime", 15));

    m_colorConfigUi.startedColorButton->setColor(QColor(cg.readEntry("PassedColor", QString("#C3C3C3"))));
    m_colorConfigUi.urgentColorButton->setColor(QColor(cg.readEntry("UrgentColor", QString("#FF0000"))));
    m_colorConfigUi.urgentOpacity->setValue(cg.readEntry("UrgentOpacity", 10));

    m_colorConfigUi.birthdayColorButton->setColor(QColor(cg.readEntry("BirthdayColor", QString("#C0FFC0"))));
    m_colorConfigUi.birthdayOpacity->setValue(cg.readEntry("BirthdayOpacity", 10));
    m_colorConfigUi.anniversariesColorButton->setColor(QColor(cg.readEntry("AnniversariesColor", QString("#ABFFEA"))));
    m_colorConfigUi.anniversariesOpacity->setValue(cg.readEntry("AnniversariesOpacity", 10));
    m_colorConfigUi.todoColorButton->setColor(QColor(cg.readEntry("TodoColor", QString("#FFD235"))));
    m_colorConfigUi.todoOpacity->setValue(cg.readEntry("TodoOpacity", 10));
}

void EventApplet::configAccepted()
{
    KConfigGroup cg = config();

    QString normalEventFormat = m_formatConfigUi.normalEventEdit->text();
    cg.writeEntry("NormalEventFormat", normalEventFormat);
    QString recurringEventsFormat = m_formatConfigUi.recurringEventsEdit->text();
    cg.writeEntry("RecurringEventsFormat", recurringEventsFormat);
    QString todoFormat = m_formatConfigUi.todoEdit->text();
    cg.writeEntry("TodoFormat", todoFormat);
    int dateFormat = m_formatConfigUi.dateFormatBox->currentIndex();
    cg.writeEntry("DateFormat", dateFormat);
    QString customString = m_formatConfigUi.customFormatEdit->text();
    cg.writeEntry("CustomDateFormat", customString);

    m_delegate->settingsChanged(normalEventFormat, recurringEventsFormat, todoFormat, dateFormat, customString);

    int oldPeriod = cg.readEntry("Period", 365);
    m_period = m_formatConfigUi.periodBox->value();
    cg.writeEntry("Period", m_period);

    m_urgency = m_colorConfigUi.urgencyBox->value();
    cg.writeEntry("UrgencyTime", m_urgency);
    
    m_colors.clear();

    m_urgentBg = m_colorConfigUi.urgentColorButton->color();
    int urgentOpacity = m_colorConfigUi.urgentOpacity->value();
    cg.writeEntry("UrgentColor", m_urgentBg.name());
    cg.writeEntry("UrgentOpacity", urgentOpacity);
    m_urgentBg.setAlphaF(urgentOpacity/100.0);
    m_colors.insert(urgentColorPos, m_urgentBg);

    m_passedFg = m_colorConfigUi.startedColorButton->color();
    cg.writeEntry("PassedColor", m_passedFg.name());
    m_colors.insert(passedColorPos, m_passedFg);
    
    QColor birthdayColor = m_colorConfigUi.birthdayColorButton->color();
    int birthdayOpacity = m_colorConfigUi.birthdayOpacity->value();
    cg.writeEntry("BirthdayColor", birthdayColor.name());
    cg.writeEntry("BirthdayOpacity", birthdayOpacity);
    birthdayColor.setAlphaF(birthdayOpacity/100.0);
    m_colors.insert(birthdayColorPos, birthdayColor);

    QColor anniversariesColor = m_colorConfigUi.anniversariesColorButton->color();
    int anniversariesOpacity = m_colorConfigUi.anniversariesOpacity->value();
    cg.writeEntry("AnniversariesColor", anniversariesColor.name());
    cg.writeEntry("AnniversariesOpacity", anniversariesOpacity);
    anniversariesColor.setAlphaF(anniversariesOpacity/100.0);
    m_colors.insert(anniversariesColorPos, anniversariesColor);

    QColor todoColor = m_colorConfigUi.todoColorButton->color();
    int todoOpacity = m_colorConfigUi.todoOpacity->value();
    cg.writeEntry("TodoColor", todoColor.name());
    cg.writeEntry("TodoOpacity", todoOpacity);
    todoColor.setAlphaF(todoOpacity/100.0);
    m_colors.insert(todoColorPos, todoColor);

    m_model->settingsChanged(m_urgency, m_colors, m_period);

    if (oldPeriod != m_period) { //just rebuild model if period changed
        m_filterModel->setPeriod(m_period);
    }

    colorizeBirthdayAndAnniversaries(birthdayColor, anniversariesColor);
    colorizeUrgentAndPassed();

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
            int itemRole = m_model->data(index, EventModel::ItemTypeRole).toInt();
            if (itemRole == EventModel::BirthdayItem) {
                m_model->setData(index, QVariant(QBrush(birthdayColor)), Qt::BackgroundRole);
            } else if (itemRole == EventModel::AnniversaryItem) {
                m_model->setData(index, QVariant(QBrush(anniversariesColor)), Qt::BackgroundRole);
            } else if (itemRole == EventModel::BirthdayItem || itemRole == EventModel::AnniversaryItem) {
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
        int itemRole = m_model->data(index, EventModel::ItemTypeRole).toInt();
        QDateTime itemDtTime = m_model->data(index, EventModel::SortRole).toDateTime();
        if (itemRole == EventModel::NormalItem) {
            if (itemDtTime > now && now.secsTo(itemDtTime) < m_urgency * 60) {
                m_model->setData(index, QVariant(QBrush(m_urgentBg)), Qt::BackgroundRole);
            } else if (now > itemDtTime) {
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
        int itemRole = m_model->data(index, EventModel::ItemTypeRole).toInt();
        QDateTime itemDtTime = m_model->data(index, EventModel::SortRole).toDateTime();
        if (itemRole == EventModel::NormalItem) {
            if (now.secsTo(itemDtTime) < m_urgency * 60) {
                m_model->setData(index, QVariant(QBrush(m_urgentBg)), Qt::BackgroundRole);
            } else {
                m_model->setData(index, QVariant(QBrush(Qt::transparent)), Qt::BackgroundRole);
            }
        }
    }
}

#include "eventapplet.moc"
