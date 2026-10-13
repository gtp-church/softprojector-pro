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

#ifndef SHORTCUTSETTINGWIDGET_HPP
#define SHORTCUTSETTINGWIDGET_HPP

#include <QWidget>
#include <QKeySequenceEdit>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include "shortcuts.hpp"

class ShortcutSettingWidget : public QWidget
{
    // Settings page where keys are assigned to commands
    Q_OBJECT

public:
    explicit ShortcutSettingWidget(QWidget *parent = 0);
    void setCommands(const QList<ShortcutCommand> &shortcutCommands);
    ShortcutKeys getKeys() const;
    QString findConflicts() const;

protected:
    void changeEvent(QEvent *e);

private slots:
    void resetToDefaults();

private:
    void retranslateUi();
    QKeySequenceEdit *addKeyField(int row, int column);

    QLabel *labelInfo;
    QTableWidget *table;
    QPushButton *buttonDefaults;
    QList<ShortcutCommand> commands;
    QList<QList<QKeySequenceEdit*> > edits; // Key fields of every command
};

#endif // SHORTCUTSETTINGWIDGET_HPP
