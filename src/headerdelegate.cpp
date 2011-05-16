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
 *
 *   almost copied from the spinbox delegate example
 */

#include "headerdelegate.h"

#include <KNumInput>

HeaderDelegate::HeaderDelegate(QObject* parent)
    : QStyledItemDelegate(parent)
{
}

HeaderDelegate::~HeaderDelegate()
{
}

QWidget *HeaderDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &/* option */, const QModelIndex &index) const
{
    int value = index.model()->data(index, Qt::EditRole).toString().toInt();
    if (value == 0)    
        return 0;

    KIntSpinBox *editor = new KIntSpinBox(parent);
    editor->setMinimum(1);
    editor->setMaximum(365);

    return editor;
}

void HeaderDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    int value = index.model()->data(index, Qt::EditRole).toString().toInt();

    KIntSpinBox *spinBox = static_cast<KIntSpinBox*>(editor);
    spinBox->setValue(value);
}

void HeaderDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    KIntSpinBox *spinBox = static_cast<KIntSpinBox*>(editor);
    spinBox->interpretText();
    int value = spinBox->value();

    model->setData(index, QString::number(value), Qt::EditRole);
}

void HeaderDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &/* index */) const
{
    editor->setGeometry(option.rect);
}

