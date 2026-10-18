/***************************************************************************
//
//    softProjector - an open source media projection software
//    Copyright (C) 2017  Vladislav Kobzar
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

#ifndef HIGHLIGHT_HPP
#define HIGHLIGHT_HPP

#include <QtWidgets>
#include <QSyntaxHighlighter>
#include <QHash>
#include <QTextCharFormat>

class Highlight : public QSyntaxHighlighter
{
    Q_OBJECT

public:
    Highlight(QTextDocument *parent = 0);

protected:
    void highlightBlock(const QString &text);

private:
    struct HighlightingRule
    {
        QRegExp pattern;
        QTextCharFormat format;
    };
    QVector<HighlightingRule> highlightingRules;

    QTextCharFormat verseFormat, chorusFormat, vstavkaFormat;
};

// *** Announcement editor highlighter
class HighlightAnnounce : public QSyntaxHighlighter
{
    Q_OBJECT

public:
    HighlightAnnounce(QTextDocument *parent = 0);

protected:
    void highlightBlock(const QString &text);

private:
    struct HighlightingRule
    {
        QRegExp pattern;
        QTextCharFormat format;
    };
    QVector<HighlightingRule> highlightingRules;

    QTextCharFormat announceFormat;
};

// *** Highligting for search results ***
class HighlightSearch : public QSyntaxHighlighter
{
    Q_OBJECT

public:
    HighlightSearch(QTextDocument *parent = 0);
    void setHighlightText(QString text);

protected:
    void highlightBlock(const QString &text);

private:
    struct HighlightingRule
    {
        QRegExp pattern;
        QTextCharFormat format;
    };
    QVector<HighlightingRule> highlightingRules;
    QTextCharFormat resultFormat;
};

class HighlighterDelegate: public QItemDelegate
{
    Q_OBJECT

public:
    HighlighterDelegate(QObject *parent = 0);
    HighlightSearch *highlighter;

protected:
    void drawDisplay(QPainter *painter, const QStyleOptionViewItem &option, const QRect &rect, const QString &text) const;

private:
    QTextDocument *textDocument;

};

class StanzaDelegate: public QStyledItemDelegate
{
    // The parts of a song in the preview: the check box of a verse is at the right edge, so verses
    // and choruses start at the same place. Search words can be highlighted.
public:
    StanzaDelegate(QObject *parent = 0);
    HighlightSearch *highlighter;
    void setHighlighting(bool on);

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const;

protected:
    bool editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option,
                     const QModelIndex &index);

private:
    QTextDocument *textDocument;
    bool highlighting;
    QRect checkRect(const QStyleOptionViewItem &option) const;
    int checkSpace(const QStyleOptionViewItem &option) const;
};

#endif // HIGHLIGHT_HPP
