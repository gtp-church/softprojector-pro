/***************************************************************************
//
//    softProjector - an open source media projection software
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation version 3 of the License.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
***************************************************************************/

#ifndef SHORTCUTS_HPP
#define SHORTCUTS_HPP

#include <QKeySequence>
#include <QList>
#include <QMap>
#include <QString>

// Number of key combinations that can be assigned to one command
const int MAX_SHORTCUT_KEYS = 3;

class ShortcutCommand
{
    // A command that can be bound to keys in Settings
public:
    QString id; // Name in the saved settings
    QString group;
    QString name;
    QList<QKeySequence> defaultKeys;
    QList<QKeySequence> keys;
};

// Keys of every command, by command id
typedef QMap<QString, QList<QKeySequence> > ShortcutKeys;

#endif // SHORTCUTS_HPP
