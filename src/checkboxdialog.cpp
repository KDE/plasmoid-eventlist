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
 *   Copyright (C) 2011 by gerdfleischer <gerdfleischer@web.de>
 */

#include "checkboxdialog.h"

#include <QWidget>
#include <QVBoxLayout>
#include <QMap>
#include <QMapIterator>
#include <QCheckBox>

#include <KLocale>


CheckBoxDialog::CheckBoxDialog(QWidget *parent, QStringList disabledProperties, QMap<QString, QString> properties)
    : KDialog(parent),
    m_checkBoxWidget(0)
{
    setupCheckBoxWidget(disabledProperties, properties);

    connect(this, SIGNAL(user1Clicked()), this, SLOT(slotUncheckAll()));
    connect(this, SIGNAL(user2Clicked()), this, SLOT(slotCheckAll()));
    connect(this, SIGNAL(resetClicked()), this, SLOT(resetProperties()));
}

CheckBoxDialog::~CheckBoxDialog()
{
}

void CheckBoxDialog::setupCheckBoxWidget(QStringList disabledProperties, QMap<QString, QString> properties)
{
    m_disabledProperties = disabledProperties;

    if (m_checkBoxWidget) {
        delete m_checkBoxWidget;
    }

    m_checkBoxWidget = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout();

    QMap<QString, QString>::const_iterator i = properties.constBegin();
    while (i != properties.constEnd()) {
        QCheckBox *propBox = new QCheckBox(i.key());
        propBox->setChecked(!disabledProperties.contains(i.value()));
        propBox->setProperty("prop", i.value());
        layout->addWidget(propBox);
        ++i;
    }

    m_checkBoxWidget->setLayout(layout);
    setMainWidget(m_checkBoxWidget);


}

void CheckBoxDialog::slotUncheckAll()
{
    QList<QCheckBox *> boxList = m_checkBoxWidget->findChildren<QCheckBox *>();
    foreach (QCheckBox *box, boxList) {
        box->setChecked(false);
    }
}

void CheckBoxDialog::slotCheckAll()
{
    QList<QCheckBox *> boxList = m_checkBoxWidget->findChildren<QCheckBox *>();
    foreach (QCheckBox *box, boxList) {
        box->setChecked(true);
    }
}

void CheckBoxDialog::resetProperties()
{
    QList<QCheckBox *> boxList = m_checkBoxWidget->findChildren<QCheckBox *>();
    foreach (QCheckBox *box, boxList) {
        box->setChecked(!m_disabledProperties.contains(box->property("prop").toString()));
    }
}

QStringList CheckBoxDialog::disabledProperties()
{
    QStringList disabled = QStringList();

    QList<QCheckBox *> boxList = m_checkBoxWidget->findChildren<QCheckBox *>();
    foreach (QCheckBox *box, boxList) {
        if (box->isChecked() == false) {
            disabled.append(box->property("prop").toString());
        }
    }

    return disabled;
}

#include "checkboxdialog.moc"
