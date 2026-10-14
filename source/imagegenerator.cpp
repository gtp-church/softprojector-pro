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

#include "imagegenerator.hpp"
#include <QFontInfo>
#include <functional>

// The biggest font size from start down to 1 at which the text fits; 1 when it never fits
// (a tiny screen, huge margins), so the screens never wait for a size that does not exist.
// Halving the range takes a few text layouts where going down a point at a time took hundreds
// for a long passage.
static int biggestFittingSize(int start, const std::function<bool(int)> &fits)
{
    start = qMax(1, start);
    if(fits(start))
        return start;
    int fitting = 0, tooBig = start; // fits(fitting) is true or fitting is 0, fits(tooBig) is false
    while(tooBig - fitting > 1)
    {
        int size = (fitting + tooBig) / 2;
        if(fits(size))
            fitting = size;
        else
            tooBig = size;
    }
    return qMax(1, fitting);
}

// Fonts are sized in points here; a font saved with a pixel size has no point size (-1)
static void scaleFontSize(QFont &font, int factor)
{
    int size = font.pointSize() > 0 ? font.pointSize() : QFontInfo(font).pointSize();
    font.setPointSize(qMax(1, size * factor));
}


ImageGenerator::ImageGenerator()
{
    m_type = 0;
    m_shadow = m_blurShadow = false;
    m_shadowOffset = 3;
    m_blurRadius = 5;
    m_screenSize = QSize(1280,960);
}

void ImageGenerator::setScreenSize(QSize size)
{
    m_screenSize = size;
}

QSize ImageGenerator::getScreenSize()
{
    return m_screenSize;
}

int ImageGenerator::width()
{
    return m_screenSize.width();
}

int ImageGenerator::height()
{
    return m_screenSize.height();
}

QPixmap ImageGenerator::generateEmptyImage()
{
    QPixmap pmap(m_screenSize);
    pmap.fill(QColor(0,0,0,0));
    return pmap;
}

QPixmap ImageGenerator::generateColorImage(QColor &color)
{
    QPixmap pmap(m_screenSize);
    pmap.fill(color);
    return pmap;
}

QPixmap ImageGenerator::generateBibleImage(Verse verse, BibleSettings &bSets)
{
    m_type = 1;
    m_verse = verse;
    m_bSets = bSets;


    scaleFontSize(m_bSets.textFont, 3);
    scaleFontSize(m_bSets.captionFont, 3);

    // TODO:FIX
//    m_shadow = (m_bSets.effectsType == 1 || m_bSets.effectsType == 2);
//    m_blurShadow = (m_bSets.effectsType == 2);
    m_shadow = m_bSets.useShadow;
    m_blurShadow = m_bSets.useBlurShadow;

    m_isTextPrepared = false;
    return renderText();
}

QPixmap ImageGenerator::generateSongImage(Stanza stanza, SongSettings &sSets)
{
    m_type = 2;
    m_stanza = stanza;
    m_sSets = sSets;
    scaleFontSize(m_sSets.textFont, 3);
    scaleFontSize(m_sSets.endingFont, 3);
    scaleFontSize(m_sSets.infoFont, 3);

    // TODO:FIX
//    m_shadow = (m_sSets.effectsType == 1 || m_sSets.effectsType == 2);
//    m_blurShadow = (m_sSets.effectsType == 2);
    m_shadow = m_sSets.useShadow;
    m_blurShadow = m_sSets.useBlurShadow;

    m_isTextPrepared = false;
    return renderText();
}

QPixmap ImageGenerator::generateAnnounceImage(AnnounceSlide announce, TextSettings &aSets)
{
    m_type = 3;
    m_announce = announce;
    m_aSets = aSets;
    scaleFontSize(m_aSets.textFont, 3);
    // TODO:FIX
//    m_shadow = (m_aSets.effectsType == 1 || m_aSets.effectsType == 2);
//    m_blurShadow = (m_aSets.effectsType == 2);
    m_shadow = m_aSets.useShadow;
    m_blurShadow = m_aSets.useBlurShadow;

    m_isTextPrepared = false;
    return renderText();

}

QPixmap ImageGenerator::renderText()
{
    QPixmap textMap(m_screenSize), shadowMap(m_screenSize), outMap(m_screenSize);
    //fill with transparent background
    textMap.fill(QColor(0,0,0,0));
    shadowMap.fill(QColor(0,0,0,0));
    outMap.fill(QColor(0,0,0,0));

    QPainter textPaint(&textMap), shadowPaint(&shadowMap), outPaint(&outMap);
    //TODO: remove set paint for shadow and make it as an option in settings.

    // int shadowOffset(0);

    // Draw main text
    switch (m_type) {
    case 1:
        // Draw Bible
        drawBibleText(&textPaint,false);
        if(m_shadow)
        {
            // prepare and draw bible shadow
            //shadowOffset = (m_bdSets.tFont.pointSize() / 15);
            drawBibleText(&shadowPaint,true);
        }
        break;
    case 2:
        // Braw Song
        drawSongText(&textPaint,false);
        if(m_shadow)
        {
            // prepare and draw song shadow
            //shadowOffset = (m_sSets.textFont.pointSize() / 15);
            drawSongText(&shadowPaint,true);
        }
        break;
    case 3:
        // Draw Announse
        drawAnnounceText(&textPaint,false);
        if(m_shadow)
        {
            // prepare and draw announcement shadow
            //shadowOffset = (m_aSets.textFont.pointSize() / 15);
            drawAnnounceText(&shadowPaint,true);
        }
        break;
    default:
        break;
    }

    textPaint.end();
    shadowPaint.end();


    // Set the blured image to the produced text image:
    if(m_blurShadow) // Blur the shadow:
    {
        QGraphicsBlurEffect* beff = new QGraphicsBlurEffect;
        beff->setBlurRadius(m_blurRadius);
        shadowMap = applyEffectToImage(shadowMap,beff);
    }

    // draw shadow onto output pixmap

    if(m_shadow || m_blurShadow)
        outPaint.drawPixmap(m_shadowOffset,m_shadowOffset,shadowMap);

    // draw text onto output pixmap
    outPaint.drawPixmap(0,0,textMap);
    outPaint.end();

    return outMap;
}

QRect ImageGenerator::boundRectOrDrawText(QPainter *painter, bool draw, int left, int top, int width, int height, int flags, QString text)
{
    // If draw is false, calculate the rectangle that the specified text would be
    // drawn into if it was draw. If draw is true, draw as well.
    // Output rect is returned.

    QRect out_rect;
    if(draw)
        painter->drawText(left, top, width, height, flags, text, &out_rect);
    else
        out_rect = painter->boundingRect(left, top, width, height, flags, text);
    return out_rect;
}

void ImageGenerator::drawBibleText(QPainter *painter, bool isShadow)
{
    // Translation flags
    bool havePrimary = ("none" != m_bSets.versions.primaryBible);
    bool haveSecondary = ("none" != m_bSets.versions.secondaryBible);
    bool haveTrinary = ("none" != m_bSets.versions.trinaryBible);

    if(!havePrimary)
    {
        // Primary Bible must exist but it is none. Do not draw anything
        return;
    }

    // Margins:
    int left = 30;
    int top = 20;

    //int right = width - left;
    int w = m_screenSize.width() - left - left;
    int h = m_screenSize.height() - top - top;

    // set maximum screen size - For primary bibile  only
    int maxh = h * m_bSets.screenUse/100; // maximun screen height
    int maxtop = top; // top of max screen
    if(m_bSets.screenPosition == 0)
        maxtop  = top;
    if(m_bSets.screenPosition == 1)
        maxtop = top+h-maxh;

    // apply max screen use settings
    h = maxh;
    top = maxtop;

    // Repurpose maxh for maxh for max height for one translation including text and caption
    if(havePrimary && haveSecondary && haveTrinary)
    {
        maxh = h/3;
    }
    else if(havePrimary && haveSecondary)
    {
        maxh = h/2;
    }
    else
    {
       maxh = h;
    }

    // Adjust tops for 2nd and 3rd translations
    int top2 = top + maxh;
    int top3 = top + maxh + maxh;

    // Rects for storing the position of the text and caption drawing:
    QRect trect1, crect1, trect2, crect2, trect3, crect3;
    // Flags to be used for drawing verse text and caption:
    int tflags = Qt::TextWordWrap;
    tflags = Qt::TextWordWrap;

    if(m_bSets.textAlignmentV==0)
        tflags += Qt::AlignTop;
    else if(m_bSets.textAlignmentV==1)
        tflags += Qt::AlignVCenter;
    else if(m_bSets.textAlignmentV==2)
        tflags += Qt::AlignBottom;

    if(m_bSets.textAlignmentH==0)
        tflags += Qt::AlignLeft;
    else if(m_bSets.textAlignmentH==1)
        tflags += Qt::AlignHCenter;
    else if(m_bSets.textAlignmentH==2)
        tflags += Qt::AlignRight;

    int cflags = Qt::AlignTop ;
    if(m_bSets.captionAlignment==0)
        cflags += Qt::AlignLeft;
    else if(m_bSets.captionAlignment==1)
        cflags += Qt::AlignHCenter;
    else if(m_bSets.captionAlignment==2)
        cflags += Qt::AlignRight;

    if(!m_isTextPrepared)
    {
        m_bdSets.clear();
        // The caption gets smaller together with the text once it would be bigger than the text
        const int textSize = m_bSets.textFont.pointSize();
        const int captionSize = m_bSets.captionFont.pointSize();
        auto fits = [&](int size) {
            m_bSets.textFont.setPointSize(size);
            m_bSets.captionFont.setPointSize(qMax(1, captionSize > textSize ? captionSize - (textSize - size)
                                                                             : qMin(captionSize, size)));
            // Figure out how much space the drawing of each translation takes at this font size
            bool fit = true;
            if(havePrimary)
            {
                drawBibleTextToRect(painter,trect1,crect1,m_verse.primary_text,m_verse.primary_caption,
                                    tflags,cflags,top,left,w,maxh);
                fit = fit && (trect1.height()+crect1.height()) <= maxh;
            }
            if(haveSecondary)
            {
                drawBibleTextToRect(painter,trect2,crect2,m_verse.secondary_text,m_verse.secondary_caption,
                                    tflags,cflags,top2,left,w,maxh);
                fit = fit && (trect2.height()+crect2.height()) <= maxh;
            }
            if(haveTrinary)
            {
                drawBibleTextToRect(painter,trect3,crect3,m_verse.trinary_text,m_verse.trinary_caption,
                                    tflags,cflags,top3,left,w,maxh);
                fit = fit && (trect3.height()+crect3.height()) <= maxh;
            }
            return fit;
        };
        fits(biggestFittingSize(textSize, fits)); // Leaves the rects and fonts of the chosen size

        m_isTextPrepared = true;
        m_bdSets.ptRect = trect1;
        m_bdSets.pcRect = crect1;
        m_bdSets.stRect = trect2;
        m_bdSets.scRect = crect2;
        m_bdSets.ttRect = trect3;
        m_bdSets.tcRect = crect3;
        m_bdSets.tFont = m_bSets.textFont;
        m_bdSets.cFont = m_bSets.captionFont;
    }

    // Draw the bible text verse(s) at the final size:
    painter->setFont(m_bdSets.tFont);
    if(isShadow)
    {
        painter->setPen(m_bSets.textShadowColor);
    }
    else
    {
        painter->setPen(m_bSets.textColor);
    }

    painter->drawText(left,m_bdSets.ptRect.top(),w,m_bdSets.ptRect.height(), tflags, m_verse.primary_text);

    if(haveSecondary && !m_verse.secondary_text.isEmpty())
    {
        painter->drawText(left,m_bdSets.stRect.top(),w,m_bdSets.stRect.height(), tflags, m_verse.secondary_text);
    }

    if(haveTrinary && !m_verse.trinary_text.isEmpty())
    {
        painter->drawText(left,m_bdSets.ttRect.top(),w,m_bdSets.ttRect.height(), tflags, m_verse.trinary_text);
    }

    painter->setFont(m_bdSets.cFont);

    // Draw the bible text caption(s) at the final size:
    if(isShadow)
    {
        painter->setPen(m_bSets.captionShadowColor);
    }
    else
    {
        painter->setPen(m_bSets.captionColor);
    }

    painter->drawText(m_bdSets.pcRect, cflags, m_verse.primary_caption);

    if(haveSecondary && !m_verse.secondary_text.isEmpty())
    {
        painter->drawText(m_bdSets.scRect, cflags, m_verse.secondary_caption);
    }

    if(haveTrinary && !m_verse.trinary_text.isEmpty())
    {
        painter->drawText(m_bdSets.tcRect, cflags, m_verse.trinary_caption);
    }
}

void ImageGenerator::drawBibleTextToRect(QPainter *painter, QRect& trect, QRect& crect, QString ttext,
                                         QString ctext, int tflags, int cflags, int top, int left,
                                         int width, int height)
{
    // prepare caption
    painter->setFont(m_bSets.captionFont);
    crect = painter->boundingRect(left, top, width, height, cflags, ctext);

    // prepare text
    painter->setFont(m_bSets.textFont);
    trect = painter->boundingRect(left, top, width, height-crect.height(), tflags, ttext);

    // reset capion location
    int ch = crect.height();
    int th = trect.height();
    if(m_bSets.captionPosition == 0)
    {
        crect.setTop(trect.top());
        crect.setHeight(ch);
        trect.setTop(crect.bottom());
        trect.setHeight(th);
    }
    else if(m_bSets.captionPosition == 1)
    {
        crect.setTop(trect.bottom());
        crect.setHeight(ch);
    }
}

void ImageGenerator::drawSongText(QPainter *painter, bool isShadow)
{
    // Draw the text of the current song verse to the specified painter; making
    // sure that the output rect is narrower than <width> and shorter than <height>.

    QString main_text = m_stanza.stanza;
    QString caption_str;
    QString song_ending = " ";

    //QStringList lines_list = song_list.at(current_song_verse).split("\n");
    QString song_num_str = QString::number(m_stanza.number);
    QString song_key_str = m_stanza.tune;

    // Check whether to display song numbers
    if (m_sSets.showSongNumber)
        song_num_str = song_num_str;
    else
        song_num_str = " ";

    // Check whether to display song key
    if (m_sSets.showSongKey)
        song_num_str = song_key_str + "  " + song_num_str;
    else
        song_num_str = song_num_str;

    // Check wheter to display stanza tiles
    if (m_sSets.showStanzaTitle)
        caption_str = m_stanza.stanzaTitle;
    else
        caption_str = " ";

    // If No cation,number or tune, give the space to song text
    if(!m_sSets.showSongNumber && !m_sSets.showSongKey && !m_sSets.showStanzaTitle)
    {
        song_num_str.clear();
        caption_str.clear();
    }

    // Prepare Song ending string
    if(m_stanza.isLast)
    {
        // first check if to show ending
        if(m_sSets.showSongEnding)
        {
            if(m_sSets.endingType == 0)
                song_ending = "*    *    *";
            else if(m_sSets.endingType == 1)
                song_ending = "-    -    -";
            else if(m_sSets.endingType == 2)
                song_ending = QString::fromUtf8("°    °    °");
            else if(m_sSets.endingType == 3)
                song_ending = QString::fromUtf8("•    •    •");
            else if(m_sSets.endingType == 4)
                song_ending = QString::fromUtf8("●    ●    ●");
            else if(m_sSets.endingType == 5)
                song_ending = QString::fromUtf8("▪    ▪    ▪");
            else if(m_sSets.endingType == 6)
                song_ending = QString::fromUtf8("■    ■    ■");
            else if(m_sSets.endingType == 7)
            {
                // First check if copyrigth info exist. If it does show it.
                // If some exist, then show what exist. If nothing exist, then show '* * *'
                // TODO: Fix translations
                //                if(!m_stanza.wordsBy.isEmpty() && !m_stanza.musicBy.isEmpty())
                //                    song_ending = QString(tr("Words by: %1, Music by: %2")).arg(m_stanza.wordsBy).arg(m_stanza.musicBy);
                //                else if(!m_stanza.wordsBy.isEmpty() && m_stanza.musicBy.isEmpty())
                //                    song_ending = QString(tr("Words by: %1")).arg(m_stanza.wordsBy);
                //                else if(m_stanza.wordsBy.isEmpty() && !m_stanza.musicBy.isEmpty())
                //                    song_ending = QString(tr("Music by: %1")).arg(m_stanza.musicBy);
                //                else if(m_stanza.wordsBy.isEmpty() && m_stanza.musicBy.isEmpty())
                song_ending = "*    *    *";
            }
        }
    }

    int width, height;
    // if not to show song ending, return its space to main text
    if(!m_sSets.showSongEnding)
        song_ending.clear();

    // Margins:
    int left = 30;
    int top = 20;
    int w = m_screenSize.width() - left - left;
    int h = m_screenSize.height() - top - top;
    int maxh = h * m_sSets.screenUse/100;
    int maxtop = top; // top of max screen
    if(m_sSets.screenPosition == 0)
        maxtop  = top;
    if(m_sSets.screenPosition == 1)
        maxtop = top+h-maxh;

    height = maxh;
    top = maxtop;
    width = w;

    QRect caption_rect, num_rect, main_rect, ending_rect;
    int main_flags(0);

    if(m_sSets.textAlignmentV==0)
        main_flags += Qt::AlignTop;
    else if(m_sSets.textAlignmentV==1)
        main_flags += Qt::AlignVCenter;
    else if(m_sSets.textAlignmentV==2)
        main_flags += Qt::AlignBottom;
    if(m_sSets.textAlignmentH==0)
        main_flags += Qt::AlignLeft;
    else if(m_sSets.textAlignmentH==1)
        main_flags += Qt::AlignHCenter;
    else if(m_sSets.textAlignmentH==2)
        main_flags += Qt::AlignRight;

    QFont main_font = m_sSets.textFont;

    int caph, endh, mainh, mainw, totalh;

    if(!m_isTextPrepared)
    {
        m_sdSets.clear();

        // Prepare Caption
        painter->setFont(m_sSets.infoFont);
        caption_rect = boundRectOrDrawText(painter, false, left, top, width, height, Qt::AlignLeft | Qt::AlignTop, caption_str);
        caph = caption_rect.height();

        // Prepare Ending
        painter->setFont(m_sSets.endingFont);
        ending_rect = boundRectOrDrawText(painter, false, left, top, width, height, Qt::AlignHCenter | Qt::AlignTop, song_ending);

        // Decrease songe ending font size so that it would fit in the screen width
        auto endingFits = [&](int size) {
            m_sSets.endingFont.setPointSize(size);
            painter->setFont(m_sSets.endingFont);
            ending_rect = boundRectOrDrawText(painter, false, left, top, width, height, Qt::AlignHCenter | Qt::AlignTop, song_ending);
            return ending_rect.width() <= width;
        };
        endingFits(biggestFittingSize(m_sSets.endingFont.pointSize(), endingFits));
        endh = ending_rect.height();

        // Decrease song text to fit the screen
        auto mainFits = [&](int size) {
            main_font.setPointSize(size);
            painter->setFont(main_font);
            main_rect = boundRectOrDrawText(painter, false, left, top, width, height, main_flags, main_text);
            mainh = main_rect.height();
            mainw = main_rect.width();
            totalh = caph+endh+mainh;
            return mainw <= width && totalh <= height;
        };
        mainFits(biggestFittingSize(main_font.pointSize(), mainFits));

        // Check if main font is less then 4/5 of original. if so, then song preparation again with text wrap
        if(main_font.pointSize() <(m_sSets.textFont.pointSize()*4/5))
        {
            main_flags += Qt::TextWordWrap;
            main_font = m_sSets.textFont;
            mainFits(biggestFittingSize(main_font.pointSize(), mainFits));
        }
        m_sSets.textFont = main_font;
        m_isTextPrepared = true;
        m_sdSets.cRect = caption_rect;
        m_sdSets.tRect = main_rect;
        m_sdSets.eRect = ending_rect;
        m_sdSets.tFlags = main_flags;
    }

    // AT This piont, all sizes should be good.
    // Set object location and DRAW
    caption_rect = m_sdSets.cRect;
    main_rect = m_sdSets.tRect;
    ending_rect = m_sdSets.eRect;
    caph = m_sdSets.cRect.height();
    endh = m_sdSets.eRect.height();
    main_flags = m_sdSets.tFlags;
    mainh = height-caph-endh;
    if(m_sSets.infoAling == 0 && m_sSets.endingPosition == 0)
    {
        painter->setFont(m_sSets.infoFont);
        if(isShadow)
            painter->setPen(m_sSets.infoShadowColor);
        else
            painter->setPen(m_sSets.infoColor);
        caption_rect = boundRectOrDrawText(painter, true, left, top, width, height, Qt::AlignLeft | Qt::AlignTop, caption_str);
        num_rect = boundRectOrDrawText(painter, true, left, top, width, height, Qt::AlignRight | Qt::AlignTop, song_num_str);
        painter->setFont(m_sSets.textFont);
        if(isShadow)
            painter->setPen(m_sSets.textShadowColor);
        else
            painter->setPen(m_sSets.textColor);
        main_rect = boundRectOrDrawText(painter, true, left, caption_rect.bottom(), width, mainh, main_flags, main_text);
        painter->setFont(m_sSets.endingFont);
        if(isShadow)
            painter->setPen(m_sSets.endingShadowColor);
        else
            painter->setPen(m_sSets.endingColor);
        ending_rect = boundRectOrDrawText(painter, true, left, main_rect.bottom(), width, height, Qt::AlignHCenter | Qt::AlignTop, song_ending);
    }
    else if(m_sSets.infoAling == 0 && m_sSets.endingPosition == 1)
    {
        painter->setFont(m_sSets.infoFont);
        if(isShadow)
            painter->setPen(m_sSets.infoShadowColor);
        else
            painter->setPen(m_sSets.infoColor);
        caption_rect = boundRectOrDrawText(painter, true, left, top, width, height, Qt::AlignLeft | Qt::AlignTop, caption_str);
        num_rect = boundRectOrDrawText(painter, true, left, top, width, height, Qt::AlignRight | Qt::AlignTop, song_num_str);
        painter->setFont(m_sSets.endingFont);
        if(isShadow)
            painter->setPen(m_sSets.endingShadowColor);
        else
            painter->setPen(m_sSets.endingColor);
        ending_rect = boundRectOrDrawText(painter, true, left, top, width, height, Qt::AlignHCenter | Qt::AlignBottom, song_ending);
        painter->setFont(m_sSets.textFont);
        if(isShadow)
            painter->setPen(m_sSets.textShadowColor);
        else
            painter->setPen(m_sSets.textColor);
        main_rect = boundRectOrDrawText(painter, true, left, caption_rect.bottom(), width, mainh, main_flags, main_text);
    }
    else if(m_sSets.infoAling == 1 && m_sSets.endingPosition == 0)
    {
        painter->setFont(m_sSets.textFont);
        if(isShadow)
            painter->setPen(m_sSets.textShadowColor);
        else
            painter->setPen(m_sSets.textColor);
        main_rect = boundRectOrDrawText(painter, true, left, top, width, mainh, main_flags, main_text);
        painter->setFont(m_sSets.infoFont);
        if(isShadow)
            painter->setPen(m_sSets.infoShadowColor);
        else
            painter->setPen(m_sSets.infoColor);
        caption_rect = boundRectOrDrawText(painter, true, left, top, width, height, Qt::AlignLeft | Qt::AlignBottom, caption_str);
        num_rect = boundRectOrDrawText(painter, true, left, top, width, height, Qt::AlignRight | Qt::AlignBottom, song_num_str);
        painter->setFont(m_sSets.endingFont);
        if(isShadow)
            painter->setPen(m_sSets.endingShadowColor);
        else
            painter->setPen(m_sSets.endingColor);
        ending_rect = boundRectOrDrawText(painter, true, left, main_rect.bottom(), width, height, Qt::AlignHCenter | Qt::AlignTop, song_ending);
    }
    else if(m_sSets.infoAling == 1 && m_sSets.endingPosition == 1)
    {
        endh = height-caph;
        painter->setFont(m_sSets.textFont);
        if(isShadow)
            painter->setPen(m_sSets.textShadowColor);
        else
            painter->setPen(m_sSets.textColor);
        main_rect = boundRectOrDrawText(painter, true, left, top, width, mainh, main_flags, main_text);
        painter->setFont(m_sSets.infoFont);
        if(isShadow)
            painter->setPen(m_sSets.infoShadowColor);
        else
            painter->setPen(m_sSets.infoColor);
        caption_rect = boundRectOrDrawText(painter, true, left, top, width, height, Qt::AlignLeft | Qt::AlignBottom, caption_str);
        num_rect = boundRectOrDrawText(painter, true, left, top, width, height, Qt::AlignRight | Qt::AlignBottom, song_num_str);
        painter->setFont(m_sSets.endingFont);
        if(isShadow)
            painter->setPen(m_sSets.endingShadowColor);
        else
            painter->setPen(m_sSets.endingColor);
        ending_rect = boundRectOrDrawText(painter, true, left, top, width, endh, Qt::AlignHCenter | Qt::AlignBottom, song_ending);
    }
}

void ImageGenerator::drawAnnounceText(QPainter *painter, bool isShadow)
{
    // Margins:
    int left = 30;
    int top = 20;
    int w = m_screenSize.width() - left - left;
    int h = m_screenSize.height() - top - top;

    int flags = Qt::TextWordWrap;
    if(m_aSets.textAlignmentV==0)
        flags += Qt::AlignTop;
    else if(m_aSets.textAlignmentV==1)
        flags += Qt::AlignVCenter;
    else if(m_aSets.textAlignmentV==2)
        flags += Qt::AlignBottom;
    if(m_aSets.textAlignmentH==0)
        flags += Qt::AlignLeft;
    else if(m_aSets.textAlignmentH==1)
        flags += Qt::AlignHCenter;
    else if(m_aSets.textAlignmentH==2)
        flags += Qt::AlignRight;

    QFont font = m_aSets.textFont;
    int orig_font_size = font.pointSize();

    // Keep decreasing the font size until the text fits into the allocated space:
    QRect rect;

    if(!m_isTextPrepared)
    {
        auto fits = [&](int size) {
            font.setPointSize(size);
            painter->setFont(font);
            rect = painter->boundingRect(left, top, w, h, flags, m_announce.text);
            return rect.width() <= w && rect.height() <= h;
        };
        fits(biggestFittingSize(orig_font_size, fits));

        // Force wrapping of songs that have really wide lines:
        // (Do not allow font to be shrinked less than a 4/5 of the desired font)
        if( font.pointSize() < (orig_font_size*4/5) )
        {
            flags = (flags | Qt::TextWordWrap);
            fits(biggestFittingSize(orig_font_size, fits));
        }
        m_aSets.textFont = font;
        m_adSets.tRect = rect;
        m_isTextPrepared = true;
    }

    painter->setFont(m_aSets.textFont);
    if(isShadow)
        painter->setPen(QColor(Qt::black));
    else
        painter->setPen(m_aSets.textColor);
    painter->drawText(m_adSets.tRect, flags, m_announce.text);
}


QPixmap ImageGenerator::applyEffectToImage(QPixmap src, QGraphicsEffect *effect)
{
    if(src.isNull()) return QPixmap(); //No need to do anything else!
    if(!effect) return src; //No need to do anything else!
    QGraphicsScene scene;
    QGraphicsPixmapItem item;
    item.setPixmap(src);
    item.setGraphicsEffect(effect);
    scene.addItem(&item);
    QPixmap res(src.size());
    res.fill(Qt::transparent);
    QPainter ptr(&res);
    scene.render(&ptr);
    return res;
}
