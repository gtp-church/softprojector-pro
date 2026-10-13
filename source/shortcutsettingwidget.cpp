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

#include "shortcutsettingwidget.hpp"
#include <QEvent>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QToolButton>
#include <QVBoxLayout>

ShortcutSettingWidget::ShortcutSettingWidget(QWidget *parent) :
    QWidget(parent)
{
    labelInfo = new QLabel(this);
    labelInfo->setWordWrap(true);

    table = new QTableWidget(0, MAX_SHORTCUT_KEYS + 1, this);
    table->verticalHeader()->setVisible(false);
    // Command names are shown in full; the key fields share the rest of the width
    table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    for(int column(1); column <= MAX_SHORTCUT_KEYS; ++column)
        table->horizontalHeader()->setSectionResizeMode(column, QHeaderView::Stretch);
    table->setSelectionMode(QAbstractItemView::NoSelection);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);

    buttonDefaults = new QPushButton(this);
    connect(buttonDefaults, &QPushButton::clicked, this, &ShortcutSettingWidget::resetToDefaults);

    QHBoxLayout *buttons = new QHBoxLayout;
    buttons->addStretch();
    buttons->addWidget(buttonDefaults);
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(labelInfo);
    layout->addWidget(table);
    layout->addLayout(buttons);

    retranslateUi();
}

void ShortcutSettingWidget::changeEvent(QEvent *e)
{
    QWidget::changeEvent(e);
    if(e->type() == QEvent::LanguageChange)
        retranslateUi();
}

void ShortcutSettingWidget::retranslateUi()
{
    QStringList headers;
    headers << tr("Command");
    for(int i(1); i <= MAX_SHORTCUT_KEYS; ++i)
        headers << tr("Keys %1").arg(i);
    table->setHorizontalHeaderLabels(headers);
    labelInfo->setText(tr("Click a field and press the keys. Keys work while the SoftProjector window is active."));
    buttonDefaults->setText(tr("Restore Defaults"));
}

void ShortcutSettingWidget::setCommands(const QList<ShortcutCommand> &shortcutCommands)
{
    commands = shortcutCommands;
    edits.clear();
    table->setRowCount(0);

    QString group;
    foreach(const ShortcutCommand &command, commands)
    {
        int row = table->rowCount();
        if(command.group != group)
        {
            // Group title row
            group = command.group;
            table->insertRow(row);
            QTableWidgetItem *item = new QTableWidgetItem(group);
            QFont font = item->font();
            font.setBold(true);
            item->setFont(font);
            table->setItem(row, 0, item);
            table->setSpan(row, 0, 1, MAX_SHORTCUT_KEYS + 1);
            ++row;
        }

        table->insertRow(row);
        table->setItem(row, 0, new QTableWidgetItem(command.name));
        QList<QKeySequenceEdit*> commandEdits;
        for(int i(0); i < MAX_SHORTCUT_KEYS; ++i)
        {
            QKeySequenceEdit *edit = addKeyField(row, i + 1);
            if(i < command.keys.count())
                edit->setKeySequence(command.keys.at(i));
            commandEdits.append(edit);
        }
        edits.append(commandEdits);
    }
}

QKeySequenceEdit *ShortcutSettingWidget::addKeyField(int row, int column)
{
    QWidget *field = new QWidget;
    QHBoxLayout *layout = new QHBoxLayout(field);
    layout->setContentsMargins(2, 0, 2, 0);
    layout->setSpacing(1);

    QKeySequenceEdit *edit = new QKeySequenceEdit(field);
    edit->setMinimumWidth(80);
    QToolButton *clear = new QToolButton(field);
    clear->setText(QString(QChar(0x00D7))); // multiplication sign
    clear->setToolTip(tr("Clear"));
    clear->setAutoRaise(true);
    layout->addWidget(edit);
    layout->addWidget(clear);

    connect(clear, &QToolButton::clicked, edit, &QKeySequenceEdit::clear);
    // A shortcut is a single key combination: stop recording after the first one
    connect(edit, &QKeySequenceEdit::keySequenceChanged, edit, [edit](const QKeySequence &key) {
        if(key.count() > 1)
            edit->setKeySequence(QKeySequence(key[0]));
        edit->clearFocus();
    });

    table->setCellWidget(row, column, field);
    return edit;
}

ShortcutKeys ShortcutSettingWidget::getKeys() const
{
    ShortcutKeys keys;
    for(int i(0); i < commands.count(); ++i)
    {
        QList<QKeySequence> commandKeys;
        foreach(QKeySequenceEdit *edit, edits.at(i))
        {
            QKeySequence key = edit->keySequence();
            if(!key.isEmpty() && !commandKeys.contains(key))
                commandKeys.append(key);
        }
        keys.insert(commands.at(i).id, commandKeys);
    }
    return keys;
}

QString ShortcutSettingWidget::findConflicts() const
{
    // Keys assigned to more than one command do not work at all, so they are not allowed
    ShortcutKeys keys = getKeys();
    QMap<QString, QStringList> commandsByKey;
    foreach(const ShortcutCommand &command, commands)
    {
        foreach(const QKeySequence &key, keys.value(command.id))
            commandsByKey[key.toString(QKeySequence::NativeText)].append(command.name);
    }

    QStringList conflicts;
    for(auto it = commandsByKey.constBegin(); it != commandsByKey.constEnd(); ++it)
    {
        if(it.value().count() > 1)
            conflicts.append(QString("%1: %2").arg(it.key()).arg(it.value().join(", ")));
    }
    return conflicts.join("\n");
}

void ShortcutSettingWidget::resetToDefaults()
{
    for(int i(0); i < commands.count(); ++i)
    {
        const QList<QKeySequence> &defaults = commands.at(i).defaultKeys;
        for(int k(0); k < edits.at(i).count(); ++k)
            edits.at(i).at(k)->setKeySequence(k < defaults.count() ? defaults.at(k) : QKeySequence());
    }
}
