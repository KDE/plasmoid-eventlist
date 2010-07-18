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

#ifndef KORGANIZERAPPLETUTIL_H
#define KORGANIZERAPPLETUTIL_H

#include <QObject>

class KOrganizerAppletUtil : public QObject
{
public:
    static void showEvent(const QString &uid);
    static void showAddEvent();
    static void showAddTodo();

private:
    static void checkAndLaunchKontact();
    static void showMainWindow();
};
#endif

