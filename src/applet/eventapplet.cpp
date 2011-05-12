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
#include "headerdelegate.h"

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
#include <QMap>
#include <QVariant>
#include <QTreeWidgetItem>
#include <QTreeWidgetItemIterator>

#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusConnectionInterface>
#include <QDBusServiceWatcher>

// kde headers
#include <KColorScheme>
#include <KConfigDialog>
#include <KIcon>
#include <KIconLoader>
#include <KStandardDirs>
#include <KDirWatch>
#include <KTabWidget>
#include <KToolInvocation>

// plasma headers
#include <Plasma/Theme>

#include <akonadi/control.h>
#include <akonadi/agentinstance.h>
#include <akonadi/servermanager.h>
#include <akonadi/kcal/incidencemimetypevisitor.h>

K_EXPORT_PLASMA_APPLET(events, EventApplet)

static const int MAX_RETRIES = 12;
static const int WAIT_FOR_KO_MSECS = 2000;

EventApplet::EventApplet(QObject *parent, const QVariantList &args) :
    Plasma::PopupApplet(parent, args),
    m_graphicsWidget(0),
    m_view(0),
    m_delegate(0),
    m_eventFormatConfig(),
    m_todoFormatConfig(),
    m_colorConfigUi(),
    m_timer(0),
    m_agentManager(0),
    m_openEventWatcher(0),
    m_addEventWatcher(0),
    m_addTodoWatcher(0)
{
    KGlobal::locale()->insertCatalog("libkcal");
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
    QString todoFormat = cg.readEntry("TodoFormat", QString("%{dueDate} %{summary}"));
    QString noDueDateFormat = cg.readEntry("NoDueDateFormat", QString("%{summary}"));
    int dtFormat = cg.readEntry("DateFormat", ShortDateFormat);
    QString dtString = cg.readEntry("CustomDateFormat", QString("dd.MM."));
    m_period = cg.readEntry("Period", 365);

    m_urgency = cg.readEntry("UrgencyTime", 15);
    m_birthdayUrgency = cg.readEntry("BirthdayUrgencyTime", 14);

    m_urgentBg = QColor(cg.readEntry("UrgentColor", QString("#FF0000")));
    m_urgentBg.setAlphaF(cg.readEntry("UrgentOpacity", 10)/100.0);
    m_colors.insert(urgentColorPos, m_urgentBg);
    m_passedFg = QColor(cg.readEntry("PassedColor", QString("#C3C3C3")));
    m_colors.insert(passedColorPos, m_passedFg);
    
    m_todoBg = QColor(cg.readEntry("TodoColor", QString("#FFD235")));
    m_todoBg.setAlphaF(cg.readEntry("TodoOpacity", 10)/100.0);
    m_colors.insert(todoColorPos, m_todoBg);

    m_showFinishedTodos = cg.readEntry("ShowFinishedTodos", FALSE);
    
    m_finishedTodoBg = QColor(cg.readEntry("FinishedTodoColor", QString("#6FACE0")));
    m_finishedTodoBg.setAlphaF(cg.readEntry("FinishedTodoOpacity", 10)/100.0);
    m_colors.insert(finishedTodoColorPos, m_finishedTodoBg);

    int opacity = cg.readEntry("KOOpacity", 10);
    setupCategoryColors(opacity);

    QString koConfigPath = KStandardDirs::locateLocal("config", "korganizerrc");
    m_categoryColorWatch = new KDirWatch(this);
    m_categoryColorWatch->addFile(koConfigPath);
    connect(m_categoryColorWatch, SIGNAL(created(const QString &)), this, SLOT(koConfigChanged()));
    connect(m_categoryColorWatch, SIGNAL(dirty(const QString &)), this, SLOT(koConfigChanged()));

    connect(Plasma::Theme::defaultTheme(), SIGNAL(themeChanged()), this, SLOT(plasmaThemeChanged()));
    
    QStringList keys, values;
    keys << i18n("Birthday") << i18n("Holiday");
    values << QString("%{startDate} %{yearsSince}. %{summary}") << QString("%{startDate} %{summary} to %{endDate}");
    keys = cg.readEntry("CategoryFormatsKeys", keys);
    values = cg.readEntry("CategoryFormatsValues", values);

    for (int i = 0; i < keys.size(); ++i) {
        m_categoryFormat.insert(keys.at(i), values.at(i));
    }

    QStringList headerList;
    headerList << i18n("Today") << i18n("Events of today") << QString::number(0);
    headerList << i18n("Tomorrow") << i18n("Events for tomorrow") << QString::number(1);
    headerList << i18n("Week") << i18n("Events of the next week") << QString::number(2);
    headerList << i18n("Next 4 weeks") << i18n("Events for the next 4 weeks") << QString::number(8);
    headerList << i18n("Later") << i18n("Events later than 4 weeks") << QString::number(29);
    m_headerItemsList = cg.readEntry("HeaderItems", headerList);

    m_delegate = new EventItemDelegate(this, normalEventFormat, todoFormat, noDueDateFormat, dtFormat, dtString);
    m_delegate->setCategoryFormats(m_categoryFormat);

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

    m_model = new EventModel(this, m_urgency, m_birthdayUrgency, m_colors);
    m_model->setCategoryColors(m_categoryColors);
    m_model->setHeaderItems(m_headerItemsList);
    m_model->initModel();
    m_model->initMonitor();
    m_model->setSortRole(EventModel::SortRole);
    m_model->sort(0, Qt::AscendingOrder);

    m_filterModel = new EventFilterModel(this);
    m_filterModel->setPeriod(m_period);
    m_filterModel->setShowFinishedTodos(m_showFinishedTodos);
    m_filterModel->setExcludedResources(disabledResources);
    m_filterModel->setDynamicSortFilter(TRUE);
    m_filterModel->setSourceModel(m_model);

    m_view->setModel(m_filterModel);
    m_view->expandAll();
    connect(m_model, SIGNAL(modelNeedsExpanding()), m_view, SLOT(expandAll()));

    m_timer->start(2 * 60 * 1000);

    connect(Akonadi::ServerManager::self(), SIGNAL(started()), this, SLOT(akonadiStatusChanged()));
}

void EventApplet::setupCategoryColors(int opacity)
{
    m_categoryColors.clear();

    KConfig *koConfig = new KConfig("korganizerrc");

    KConfigGroup general(koConfig, "General");
    QStringList categories = general.readEntry("Custom Categories", QStringList());

    KConfigGroup categoryColors(koConfig, "Category Colors2");
    foreach(const QString &category, categories) {
        QColor cColor = categoryColors.readEntry(category, QColor());
        if (cColor.isValid()) {
            cColor.setAlphaF(opacity/100.0);
            m_categoryColors.insert(category, cColor);
        }
    }
}

void EventApplet::plasmaThemeChanged()
{
#if KDE_IS_VERSION(4,5,0)
    QString currentStyle = Plasma::Theme::defaultTheme()->styleSheet();
    title->setStyleSheet(currentStyle);
#endif

    QColor textColor = Plasma::Theme::defaultTheme()->color(Plasma::Theme::TextColor);
    QColor baseColor = Plasma::Theme::defaultTheme()->color(Plasma::Theme::BackgroundColor);
    QColor altBaseColor = baseColor.darker(150);
    QColor buttonColor = altBaseColor;
    baseColor.setAlpha(50);
    altBaseColor.setAlpha(50);
    buttonColor.setAlpha(150);

    QPalette p = palette();
    p.setColor(QPalette::Base, baseColor);
    p.setColor(QPalette::AlternateBase, altBaseColor);
    p.setColor(QPalette::Button, buttonColor);
    p.setColor(QPalette::Foreground, textColor);
    p.setColor(QPalette::Text, textColor);
    
    m_view->viewport()->setPalette(p);
    m_view->setPalette(p);

    colorizeModel(FALSE);
}

void EventApplet::koConfigChanged()
{
    KConfigGroup cg = config();
    int opacity = cg.readEntry("KOOpacity", 10);
    setupCategoryColors(opacity);
    m_model->setCategoryColors(m_categoryColors);
    colorizeModel(FALSE);
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

        connect(m_view, SIGNAL(tooltipUpdated(QString)),
                SLOT(slotUpdateTooltip(QString)));

        title = new Plasma::Label();
        title->setText("<b>" + i18n("Upcoming Events") + "</b>");

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
    m_uid = QString();
#ifdef HAS_AKONADI_PIM
    m_uid = m_filterModel->data(index, EventModel::ItemIDRole).toString();
#else
    m_uid = m_filterModel->data(index, EventModel::UIDRole).toString();
#endif

    if (!m_openEventWatcher) {
        m_openEventWatcher = new QDBusServiceWatcher("org.kde.korganizer",
                                                     QDBusConnection::sessionBus(),
                                                     QDBusServiceWatcher::WatchForRegistration,
                                                     this);
    }

    if (!m_uid.isEmpty()) {
        if (!QDBusConnection::sessionBus().interface()->isServiceRegistered("org.kde.korganizer")) {
            connect(m_openEventWatcher, SIGNAL(serviceRegistered(const QString &)),
                    this, SLOT(korganizerStartedOpenEvent(const QString &)));

            KToolInvocation::startServiceByDesktopName("korganizer");
        } else {
            KOrganizerAppletUtil::showEvent(m_uid);
        }
    }
}

void EventApplet::korganizerStartedOpenEvent(const QString &)
{
    delete m_openEventWatcher;
    m_openEventWatcher = 0;
    QTimer::singleShot(WAIT_FOR_KO_MSECS, this, SLOT(timedOpenEvent()));
}

void EventApplet::timedOpenEvent()
{
    KOrganizerAppletUtil::showEvent(m_uid);    
}

void EventApplet::openEventFromMenu()
{
    slotOpenEvent(m_view->indexAtCursor());
}

void EventApplet::slotAddEvent()
{
    if (!m_addEventWatcher) {
        m_addEventWatcher = new QDBusServiceWatcher("org.kde.korganizer",
                                                     QDBusConnection::sessionBus(),
                                                     QDBusServiceWatcher::WatchForRegistration,
                                                     this);
    }

    if (!QDBusConnection::sessionBus().interface()->isServiceRegistered("org.kde.korganizer")) {
        connect(m_addEventWatcher, SIGNAL(serviceRegistered(const QString &)),
                this, SLOT(korganizerStartedAddEvent(const QString &)));

        KToolInvocation::startServiceByDesktopName("korganizer");
    } else {
        KOrganizerAppletUtil::showAddEvent();
    }
}

void EventApplet::korganizerStartedAddEvent(const QString &)
{
    delete m_addEventWatcher;
    m_addEventWatcher = 0;
    QTimer::singleShot(WAIT_FOR_KO_MSECS, this, SLOT(timedAddEvent()));
}

void EventApplet::timedAddEvent()
{
    KOrganizerAppletUtil::showAddEvent();    
}

void EventApplet::slotAddTodo()
{
    if (!m_addTodoWatcher) {
        m_addTodoWatcher = new QDBusServiceWatcher("org.kde.korganizer",
                                                     QDBusConnection::sessionBus(),
                                                     QDBusServiceWatcher::WatchForRegistration,
                                                     this);
    }

    if (!QDBusConnection::sessionBus().interface()->isServiceRegistered("org.kde.korganizer")) {
        connect(m_addTodoWatcher, SIGNAL(serviceRegistered(const QString &)),
                this, SLOT(korganizerStartedAddTodo(const QString &)));
        
        KToolInvocation::startServiceByDesktopName("korganizer");
    } else {
        KOrganizerAppletUtil::showAddTodo();
    }
}

void EventApplet::korganizerStartedAddTodo(const QString &)
{
    delete m_addTodoWatcher;
    m_addTodoWatcher = 0;
    QTimer::singleShot(WAIT_FOR_KO_MSECS, this, SLOT(timedAddTodo()));
}

void EventApplet::timedAddTodo()
{
    KOrganizerAppletUtil::showAddTodo();    
}

void EventApplet::timerExpired()
{
    if (lastCheckTime.date() != QDate::currentDate()) {
        m_model->resetModel();
    } else {
        colorizeModel(TRUE);
    }

    lastCheckTime = QDateTime::currentDateTime();
    m_timer->start(2 * 60 * 1000);
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
        if (agentMimeTypes.contains(Akonadi::IncidenceMimeTypeVisitor::eventMimeType()) ||
            agentMimeTypes.contains(Akonadi::IncidenceMimeTypeVisitor::todoMimeType()) ||
            agentMimeTypes.contains("text/calendar")) {
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
    QList<QAction *> currentActions;

    QModelIndex idx = m_view->indexAtCursor();
    if (idx.isValid()) {
        QVariant type = idx.data(EventModel::ItemTypeRole);
        if (type.toInt() == EventModel::NormalItem || type.toInt() == EventModel::TodoItem) {
            QString actionTitle = m_view->summaryAtCursor();
            if (actionTitle.length() > 24) {
                actionTitle.truncate(24);
                actionTitle.append("...");
            }
            QAction *openEvent = new QAction(i18n("Open \"%1\"", actionTitle), this);
            openEvent->setIcon(KIcon("document-edit"));
            connect(openEvent, SIGNAL(triggered()), this, SLOT(openEventFromMenu()));
            currentActions.append(openEvent);
        }
    }

    QAction *newEvent = new QAction(i18n("Add new event"), this);
    newEvent->setIcon(KIcon("appointment-new"));
    connect(newEvent, SIGNAL(triggered()), this, SLOT(slotAddEvent()));
    currentActions.append(newEvent);

    QAction *newTodo = new QAction(i18n("Add new todo"), this);
    newTodo->setIcon(KIcon("view-task-add"));
    connect(newTodo, SIGNAL(triggered()), this, SLOT(slotAddTodo()));
    currentActions.append(newTodo);

    QAction *selectResources = new QAction(i18n("Select shown resources"), this);
    selectResources->setIcon(KIcon("view-calendar-tasks"));
    connect(selectResources, SIGNAL(triggered()), this, SLOT(setShownResources()));
    currentActions.append(selectResources);

    return currentActions;
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
    QWidget *generalWidget = new QWidget();
    m_generalConfig.setupUi(generalWidget);
    HeaderDelegate *hd = new HeaderDelegate();
    m_generalConfig.headerWidget->setItemDelegateForColumn(2, hd);
    m_generalConfig.setupConnections();

    KTabWidget *formatTabs = new KTabWidget();
    
    QWidget *eventFormatWidget = new QWidget();
    m_eventFormatConfig.setupUi(eventFormatWidget);
    m_eventFormatConfig.setupConnections();
    QWidget *todoFormatWidget = new QWidget();
    m_todoFormatConfig.setupUi(todoFormatWidget);

    formatTabs->addTab(eventFormatWidget, i18n("Event text formatting"));
    formatTabs->addTab(todoFormatWidget, i18n("Todo text formatting"));
    QWidget *colorWidget = new QWidget();
    m_colorConfigUi.setupUi(colorWidget);
    parent->setButtons(KDialog::Ok | KDialog::Apply | KDialog::Cancel);

    connect(parent, SIGNAL(applyClicked()), this, SLOT(configAccepted()));
    connect(parent, SIGNAL(okClicked()), this, SLOT(configAccepted()));

    parent->addPage(generalWidget, i18n("General"), "view-list-tree");
    parent->addPage(formatTabs, i18n("Text Format"), "format-text-bold");
    parent->addPage(colorWidget, i18n("Colors"), "fill-color");

    KConfigGroup cg = config();

    for (int i = 0; i < m_headerItemsList.size(); i += 3) {
        QStringList itemText;
        itemText << m_headerItemsList.value(i) << m_headerItemsList.value(i + 1) << m_headerItemsList.value(i + 2);
        TreeWidgetItem *headerItem = new TreeWidgetItem(itemText);
        headerItem->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEditable|Qt::ItemIsEnabled);
        m_generalConfig.headerWidget->addTopLevelItem(headerItem);
    }
    m_generalConfig.headerWidget->sortByColumn(2, Qt::AscendingOrder);
    m_generalConfig.periodBox->setValue(cg.readEntry("Period", 365));
    m_generalConfig.dateFormatBox->setCurrentIndex(cg.readEntry("DateFormat", ShortDateFormat));
    m_generalConfig.customFormatEdit->setText(cg.readEntry("CustomDateFormat", QString("dd.MM.")));

    m_eventFormatConfig.normalEventEdit->setText(cg.readEntry("NormalEventFormat", QString("%{startDate} %{startTime} %{summary}")));

    QMap<QString, QString>::const_iterator i = m_categoryFormat.constBegin();
    while (i != m_categoryFormat.constEnd()) {
        QStringList itemText;
        itemText << i.key() << i.value();
        QTreeWidgetItem *categoryItem = new QTreeWidgetItem(itemText);
        categoryItem->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEditable|Qt::ItemIsEnabled);
        m_eventFormatConfig.categoryFormatWidget->addTopLevelItem(categoryItem);
        ++i;
    }

    m_todoFormatConfig.todoEdit->setText(cg.readEntry("TodoFormat", QString("%{dueDate} %{summary}")));
    m_todoFormatConfig.noDueDateEdit->setText(cg.readEntry("NoDueDateFormat", QString("%{summary}")));

    m_colorConfigUi.urgencyBox->setValue(cg.readEntry("UrgencyTime", 15));
    m_colorConfigUi.birthdayUrgencyBox->setValue(cg.readEntry("BirthdayUrgencyTime", 14));

    m_colorConfigUi.startedColorButton->setColor(QColor(cg.readEntry("PassedColor", QString("#C3C3C3"))));
    m_colorConfigUi.urgentColorButton->setColor(QColor(cg.readEntry("UrgentColor", QString("#FF0000"))));
    m_colorConfigUi.urgentOpacity->setValue(cg.readEntry("UrgentOpacity", 10));

    m_colorConfigUi.todoColorButton->setColor(QColor(cg.readEntry("TodoColor", QString("#FFD235"))));
    m_colorConfigUi.todoOpacity->setValue(cg.readEntry("TodoOpacity", 10));
    m_colorConfigUi.showFinishedTodos->setChecked(cg.readEntry("ShowFinishedTodos", FALSE));
    m_colorConfigUi.finishedTodoButton->setColor(QColor(cg.readEntry("FinishedTodoColor", QString("#6FACE0"))));
    m_colorConfigUi.finishedTodoOpacity->setValue(cg.readEntry("FinishedTodoOpacity", 10));

    m_colorConfigUi.korganizerOpacity->setValue(cg.readEntry("KOOpacity", 10));
}

void EventApplet::configAccepted()
{
    KConfigGroup cg = config();

    //general config
    QStringList oldHeaderList = m_headerItemsList;
    m_headerItemsList.clear();
    QTreeWidgetItemIterator hi(m_generalConfig.headerWidget);
    while (*hi) {
        m_headerItemsList << (*hi)->text(0) << (*hi)->text(1) << (*hi)->text(2);
         ++hi;
     }
    cg.writeEntry("HeaderItems", m_headerItemsList);

    int oldPeriod = cg.readEntry("Period", 365);
    m_period = m_generalConfig.periodBox->value();
    cg.writeEntry("Period", m_period);
    int dateFormat = m_generalConfig.dateFormatBox->currentIndex();
    cg.writeEntry("DateFormat", dateFormat);
    QString customString = m_generalConfig.customFormatEdit->text();
    cg.writeEntry("CustomDateFormat", customString);

    //event config
    QString normalEventFormat = m_eventFormatConfig.normalEventEdit->text();
    cg.writeEntry("NormalEventFormat", normalEventFormat);
    m_categoryFormat.clear();
    QStringList keys, values;
    QTreeWidgetItemIterator it(m_eventFormatConfig.categoryFormatWidget);
    while (*it) {
        m_categoryFormat[(*it)->text(0)] = (*it)->text(1);
        keys << (*it)->text(0);
        values << (*it)->text(1);
         ++it;
     }
    cg.writeEntry("CategoryFormatsKeys", keys);
    cg.writeEntry("CategoryFormatsValues", values);

    //todo config
    QString todoFormat = m_todoFormatConfig.todoEdit->text();
    cg.writeEntry("TodoFormat", todoFormat);
    QString noDueDateFormat = m_todoFormatConfig.noDueDateEdit->text();
    cg.writeEntry("NoDueDateFormat", noDueDateFormat);

    m_delegate->setCategoryFormats(m_categoryFormat);
    m_delegate->settingsChanged(normalEventFormat, todoFormat, noDueDateFormat, dateFormat, customString);

    //color config
    int oldUrgency = m_urgency;
    m_urgency = m_colorConfigUi.urgencyBox->value();
    cg.writeEntry("UrgencyTime", m_urgency);
    
    int oldBirthdayUrgency = m_birthdayUrgency;
    m_birthdayUrgency = m_colorConfigUi.birthdayUrgencyBox->value();
    cg.writeEntry("BirthdayUrgencyTime", m_birthdayUrgency);

    QList<QColor> oldColors = m_colors;
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
    
    m_todoBg = m_colorConfigUi.todoColorButton->color();
    int todoOpacity = m_colorConfigUi.todoOpacity->value();
    cg.writeEntry("TodoColor", m_todoBg.name());
    cg.writeEntry("TodoOpacity", todoOpacity);
    m_todoBg.setAlphaF(todoOpacity/100.0);
    m_colors.insert(todoColorPos, m_todoBg);

    bool oldShowFinishedTodos = m_showFinishedTodos;
    m_showFinishedTodos = m_colorConfigUi.showFinishedTodos->isChecked();
    cg.writeEntry("ShowFinishedTodos", m_showFinishedTodos);

    m_finishedTodoBg = m_colorConfigUi.finishedTodoButton->color();
    int finishedTodoOpacity = m_colorConfigUi.finishedTodoOpacity->value();
    cg.writeEntry("FinishedTodoColor", m_finishedTodoBg.name());
    cg.writeEntry("FinishedTodoOpacity", finishedTodoOpacity);
    m_finishedTodoBg.setAlphaF(finishedTodoOpacity/100.0);
    m_colors.insert(finishedTodoColorPos, m_finishedTodoBg);

    QHash<QString, QColor> oldColorHash = m_categoryColors;
    int opacity = m_colorConfigUi.korganizerOpacity->value();
    cg.writeEntry("KOOpacity", opacity);
    setupCategoryColors(opacity);

    m_model->settingsChanged(m_urgency, m_birthdayUrgency, m_colors);
    m_model->setCategoryColors(m_categoryColors);

    if (oldPeriod != m_period) {
        m_filterModel->setPeriod(m_period);
    }

    if (oldShowFinishedTodos != m_showFinishedTodos) {
        m_filterModel->setShowFinishedTodos(m_showFinishedTodos);
    }

    if (oldHeaderList != m_headerItemsList) {
        m_model->setHeaderItems(m_headerItemsList);
        m_model->resetModel();
    } else if (oldUrgency != m_urgency || oldBirthdayUrgency != m_birthdayUrgency || oldColors != m_colors ||
        oldColorHash != m_categoryColors) {
        colorizeModel(FALSE);
    }

    m_view->expandAll();

    emit configNeedsSaving();
}

void EventApplet::colorizeModel(bool timerTriggered)
{
    QColor defaultTextColor = Plasma::Theme::defaultTheme()->color(Plasma::Theme::TextColor);
    QDateTime now = QDateTime::currentDateTime();

    int headerRows = m_model->rowCount(QModelIndex());
    for (int r = 0; r < headerRows; ++r) {
        QModelIndex headerIndex = m_model->index(r, 0, QModelIndex());
        QDateTime headerDtTime = m_model->data(headerIndex, EventModel::SortRole).toDateTime();
        m_model->setData(headerIndex, QVariant(QBrush(defaultTextColor)), Qt::ForegroundRole);
        if (timerTriggered && now.daysTo(headerDtTime) > m_birthdayUrgency) {
            break;
        }
        int childRows = m_model->rowCount(headerIndex);
        for (int c = 0; c < childRows; ++c) {
            QModelIndex index = m_model->index(c, 0, headerIndex);
            const QVariant v = index.data(Qt::DisplayRole);
            QMap<QString, QVariant> values = v.toMap();
            QString category = values["mainCategory"].toString();
            int itemRole = m_model->data(index, EventModel::ItemTypeRole).toInt();
            QDateTime itemDtTime = m_model->data(index, EventModel::SortRole).toDateTime();

            m_model->setData(index, QVariant(QBrush(defaultTextColor)), Qt::ForegroundRole);

            if (timerTriggered && now.daysTo(itemDtTime) > m_birthdayUrgency) {
                break;
            } else if (itemRole == EventModel::BirthdayItem || itemRole == EventModel::AnniversaryItem) {
                if (itemDtTime.date() >= now.date() && now.daysTo(itemDtTime) < m_birthdayUrgency) {
                    m_model->setData(index, QVariant(QBrush(m_urgentBg)), Qt::BackgroundRole);
                } else if (m_categoryColors.contains(category)) {
                    m_model->setData(index, QVariant(QBrush(m_categoryColors.value(category))), Qt::BackgroundRole);
                } else {
                    m_model->setData(index, QVariant(QBrush(Qt::transparent)), Qt::BackgroundRole);
                }
            } else if (itemRole == EventModel::NormalItem) {
                if (itemDtTime > now && now.secsTo(itemDtTime) < m_urgency * 60) {
                    m_model->setData(index, QVariant(QBrush(m_urgentBg)), Qt::BackgroundRole);
                } else if (now > itemDtTime) {
                    m_model->setData(index, QVariant(QBrush(m_passedFg)), Qt::ForegroundRole);
                    m_model->setData(index, QVariant(QBrush(Qt::transparent)), Qt::BackgroundRole);
                } else if (m_categoryColors.contains(category)) {
                    m_model->setData(index, QVariant(QBrush(m_categoryColors.value(category))), Qt::BackgroundRole);
                } else {
                    m_model->setData(index, QVariant(QBrush(Qt::transparent)), Qt::BackgroundRole);
                }
            } else if (itemRole == EventModel::TodoItem) {
                if (values["completed"].toBool() == TRUE) {
                    m_model->setData(index, QVariant(QBrush(m_finishedTodoBg)), Qt::BackgroundRole);
                } else if (m_categoryColors.contains(category)) {
                    m_model->setData(index, QVariant(QBrush(m_categoryColors.value(category))), Qt::BackgroundRole);
                } else {
                    m_model->setData(index, QVariant(QBrush(m_todoBg)), Qt::BackgroundRole);
                }
            }
        }
    }
}

#include "eventapplet.moc"
