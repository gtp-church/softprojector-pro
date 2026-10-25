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

#include "highlight.hpp"
#include <QPainter>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QAbstractTextDocumentLayout>
#include <QApplication>

Highlight::Highlight(QTextDocument *parent)
    : QSyntaxHighlighter(parent)
{
    HighlightingRule rule;
    QStringList patterns;

    // Verse formating
    verseFormat.setForeground(Qt::red);
    verseFormat.setBackground(Qt::yellow);
    patterns << "^Verse[^\n]*" << "^&Verse[^\n]*"
             << QString::fromUtf8("^Куплет[^\n]*") << QString::fromUtf8("^&Куплет[^\n]*")
             << QString::fromUtf8("^Strophe[^\n]*") << QString::fromUtf8("^&Strophe[^\n]*")
             << QString::fromUtf8("^Verš[^\n]*") << QString::fromUtf8("^&Verš[^\n]*");
    foreach (const QString &pattern, patterns)
    {
        rule.pattern = QRegExp(pattern);
        rule.format = verseFormat;
        highlightingRules.append(rule);
    }

    patterns.clear();

    // Chorus formating
    chorusFormat.setFontItalic(true);
    chorusFormat.setForeground(Qt::darkBlue);
    chorusFormat.setBackground(QColor(212,240,28,255));
    patterns << "^Chorus[^\n]*" << "^&Chorus[^\n]*"
             << QString::fromUtf8("^Sbor[^\n]*") << QString::fromUtf8("^&Sbor[^\n]*")
             << "^Refrain[^\n]*" << "^&Refrain[^\n]*"
             << QString::fromUtf8("^Припев[^\n]*") << QString::fromUtf8("^&Припев[^\n]*")
             << QString::fromUtf8("^Приспів[^\n]*") << QString::fromUtf8("^&Приспів[^\n]*")
             << QString::fromUtf8("^Refrén[^\n]*") << QString::fromUtf8("^&Refrén[^\n]*");
    foreach (const QString &pattern, patterns)
    {
        rule.pattern = QRegExp(pattern);
        rule.format = chorusFormat;
        highlightingRules.append(rule);
    }

    patterns.clear();

    // Vsavka formating
    vstavkaFormat.setForeground(Qt::darkMagenta);
    vstavkaFormat.setBackground(QColor(255,140,0,255));
    patterns << "^Slide[^\n]*" << "^Insert[^\n]*"
             << "^Intro[^\n]*" << "^Ending[^\n]*"
             << QString::fromUtf8("^Слайд[^\n]*") << QString::fromUtf8("^Вставка[^\n]*")
             << QString::fromUtf8("^Вступление[^\n]*") << QString::fromUtf8("^Окончание[^\n]*")
             << QString::fromUtf8("^Закінчення[^\n]*")
             << QString::fromUtf8("^Dia[^\n]*") << QString::fromUtf8("^Einfügung[^\n]*")
             << QString::fromUtf8("^Einleitung[^\n]*") << QString::fromUtf8("^Ende[^\n]*")
             << QString::fromUtf8("^Snímek[^\n]*") << QString::fromUtf8("^Vložka[^\n]*")
             << QString::fromUtf8("^Úvod[^\n]*") << QString::fromUtf8("^Závěr[^\n]*");
    foreach (const QString &pattern, patterns)
    {
        rule.pattern = QRegExp(pattern);
        rule.format = vstavkaFormat;
        highlightingRules.append(rule);
    }
}

void Highlight::highlightBlock(const QString &text)
{
    foreach (const HighlightingRule &rule, highlightingRules)
    {
        QRegExp expression(rule.pattern);
        int index = expression.indexIn(text);
        while (index >= 0)
        {
            int length = expression.matchedLength();
            setFormat(index, length, rule.format);
            index = expression.indexIn(text, index + length);
        }
    }

    setCurrentBlockState(0);
}

// *** Announcement Editor Highlighter

HighlightAnnounce::HighlightAnnounce(QTextDocument *parent)
    : QSyntaxHighlighter(parent)
{
    HighlightingRule rule;
    QStringList patterns;

    // Verse formating
    announceFormat.setForeground(Qt::red);
    announceFormat.setBackground(Qt::yellow);
    patterns << "^Announce[^\n]*" << "^Slide[^\n]*"
             << QString::fromUtf8("^Объявление[^\n]*") << QString::fromUtf8("^Слайд[^\n]*")
             << QString::fromUtf8("^Оголошення[^\n]*")
             << QString::fromUtf8("^Ankündigung[^\n]*") << QString::fromUtf8("^Dia[^\n]*")
             << QString::fromUtf8("^Oznámení[^\n]*") << QString::fromUtf8("^Snímek[^\n]*");
    foreach (const QString &pattern, patterns)
    {
        rule.pattern = QRegExp(pattern);
        rule.format = announceFormat;
        highlightingRules.append(rule);
    }
}

void HighlightAnnounce::highlightBlock(const QString &text)
{
    foreach (const HighlightingRule &rule, highlightingRules)
    {
        QRegExp expression(rule.pattern);
        int index = expression.indexIn(text);
        while (index >= 0)
        {
            int length = expression.matchedLength();
            setFormat(index, length, rule.format);
            index = expression.indexIn(text, index + length);
        }
    }

    setCurrentBlockState(0);
}

// *** Highligting for search results ***
HighlightSearch::HighlightSearch(QTextDocument *parent)
    : QSyntaxHighlighter(parent)
{
}

void HighlightSearch::highlightBlock(const QString &text)
{
    foreach (const HighlightingRule &rule, highlightingRules)
    {
        QRegExp expression(rule.pattern);
        int index = expression.indexIn(text);
        while (index >= 0)
        {
            int length = expression.matchedLength();
            setFormat(index, length, rule.format);
            index = expression.indexIn(text, index + length);
        }
    }

    setCurrentBlockState(0);
}

void HighlightSearch::setHighlightText(QString text)
{
    HighlightingRule rule;
    highlightingRules.clear();
    resultFormat.setForeground(Qt::red);
    rule.pattern = QRegExp(text,Qt::CaseInsensitive);
    rule.format = resultFormat;
    highlightingRules.append(rule);
}

/**********************************************/
/**** Class for higlighting search results ****/
/**********************************************/
HighlighterDelegate::HighlighterDelegate(QObject *parent)
    : QItemDelegate(parent)
{
    textDocument = new QTextDocument(this);
    highlighter = new HighlightSearch(textDocument);
}

void HighlighterDelegate::drawDisplay(QPainter *painter, const QStyleOptionViewItem &option, const QRect &rect, const QString &text) const
{
    Q_UNUSED(option);
    textDocument->setDocumentMargin(0);
    textDocument->setPlainText(text);

    QPixmap pixmap(rect.size());
    pixmap.fill(Qt::transparent);
    QPainter p(&pixmap);

    textDocument->drawContents(&p);

    painter->drawPixmap(rect, pixmap);
}

/**********************************************/
/**** Parts of a song in the song preview  ****/
/**********************************************/
StanzaDelegate::StanzaDelegate(QObject *parent)
    : QStyledItemDelegate(parent), highlighting(false)
{
    textDocument = new QTextDocument(this);
    highlighter = new HighlightSearch(textDocument);
}

void StanzaDelegate::setHighlighting(bool on)
{
    highlighting = on;
}

static const QStyle *styleOf(const QStyleOptionViewItem &option)
{
    return option.widget ? option.widget->style() : QApplication::style();
}

int StanzaDelegate::checkSpace(const QStyleOptionViewItem &option) const
{
    // Kept free on every row, with or without a check box, so all texts wrap the same way
    return styleOf(option)->pixelMetric(QStyle::PM_IndicatorWidth, &option, option.widget) + 12;
}

QRect StanzaDelegate::checkRect(const QStyleOptionViewItem &option) const
{
    const QStyle *style = styleOf(option);
    int width = style->pixelMetric(QStyle::PM_IndicatorWidth, &option, option.widget);
    int height = style->pixelMetric(QStyle::PM_IndicatorHeight, &option, option.widget);
    return QRect(option.rect.right() - 6 - width, option.rect.top() + (option.rect.height() - height) / 2,
                 width, height);
}

void StanzaDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);
    bool checkable = opt.features & QStyleOptionViewItem::HasCheckIndicator;
    opt.features &= ~QStyleOptionViewItem::HasCheckIndicator;
    const QStyle *style = styleOf(opt);

    // Background and selection across the whole row, the text left of the check box
    style->drawPrimitive(QStyle::PE_PanelItemViewItem, &opt, painter, opt.widget);
    QStyleOptionViewItem textOpt = opt;
    textOpt.rect.setRight(opt.rect.right() - checkSpace(opt));
    textOpt.features |= QStyleOptionViewItem::WrapText;
    if(highlighting)
    {
        QRect textRect = style->subElementRect(QStyle::SE_ItemViewItemText, &textOpt, opt.widget);
        textDocument->setDocumentMargin(0);
        textDocument->setDefaultFont(opt.font);
        textDocument->setPlainText(opt.text);
        textDocument->setTextWidth(textRect.width());
        QAbstractTextDocumentLayout::PaintContext context;
        QPalette::ColorGroup group = (opt.state & QStyle::State_Enabled) ? QPalette::Normal : QPalette::Disabled;
        // The text keeps its colour when selected, as in the other lists (black on light blue)
        context.palette.setColor(QPalette::Text, opt.palette.color(group, QPalette::Text));
        painter->save();
        painter->translate(textRect.topLeft());
        painter->setClipRect(textRect.translated(-textRect.topLeft()));
        textDocument->documentLayout()->draw(painter, context);
        painter->restore();
    }
    else
    {
        textOpt.backgroundBrush = Qt::NoBrush;
        // Already drawn; the text keeps its colour, as in the other lists (black on light blue).
        // A left-out verse stays grey.
        textOpt.state &= ~QStyle::State_Selected;
        style->drawControl(QStyle::CE_ItemViewItem, &textOpt, painter, opt.widget);
    }

    if(checkable)
    {
        QStyleOptionViewItem checkOpt = opt;
        checkOpt.rect = checkRect(opt);
        checkOpt.state &= ~QStyle::State_HasFocus;
        checkOpt.state |= (opt.checkState == Qt::Checked) ? QStyle::State_On : QStyle::State_Off;
        style->drawPrimitive(QStyle::PE_IndicatorItemViewItemCheck, &checkOpt, painter, opt.widget);
    }
}

QSize StanzaDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    // As wide as the list: long lines wrap instead of running off to the right, where the
    // check box has to stay visible
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);
    opt.features &= ~QStyleOptionViewItem::HasCheckIndicator;
    opt.features |= QStyleOptionViewItem::WrapText;
    int width = opt.rect.width();
    if(const QListView *list = qobject_cast<const QListView *>(opt.widget))
        width = list->viewport()->width() - 2 * list->spacing();
    int space = checkSpace(opt);
    opt.rect.setWidth(qMax(1, width - space));
    QSize size = styleOf(opt)->sizeFromContents(QStyle::CT_ItemViewItem, &opt, QSize(), opt.widget);
    size.setWidth(qMax(1, width));
    return size;
}

bool StanzaDelegate::editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option,
                                 const QModelIndex &index)
{
    // Like the standard check box handling, with the box at the right
    if(!(index.flags() & Qt::ItemIsUserCheckable) || !(index.flags() & Qt::ItemIsEnabled))
        return false;
    if(event->type() == QEvent::MouseButtonPress || event->type() == QEvent::MouseButtonRelease
            || event->type() == QEvent::MouseButtonDblClick)
    {
        QMouseEvent *mouse = static_cast<QMouseEvent *>(event);
        if(mouse->button() != Qt::LeftButton || !checkRect(option).adjusted(-4, -4, 4, 4).contains(mouse->pos()))
            return false;
        if(event->type() != QEvent::MouseButtonRelease)
            return true;
    }
    else if(event->type() == QEvent::KeyPress)
    {
        int key = static_cast<QKeyEvent *>(event)->key();
        if(key != Qt::Key_Space && key != Qt::Key_Select)
            return false;
    }
    else
        return false;

    Qt::CheckState state = index.data(Qt::CheckStateRole).toInt() == Qt::Checked ? Qt::Unchecked : Qt::Checked;
    return model->setData(index, state, Qt::CheckStateRole);
}
