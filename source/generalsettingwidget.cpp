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

#include "generalsettingwidget.hpp"
#include "ui_generalsettingwidget.h"
#include "decklinkoutput.hpp"

GeneralSettingWidget::GeneralSettingWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::GeneralSettingWidget)
{
    ui->setupUi(this);

    // Video format of a Blackmagic card used as the secondary display
    labelDeckLinkFormat = new QLabel(tr("Card video format:"));
    comboBoxDeckLinkFormat = new QComboBox;
    comboBoxDeckLinkFormat->addItems(DeckLinkOutput::formats());
    int row = ui->gridLayout->rowCount();
    ui->gridLayout->addWidget(labelDeckLinkFormat, row, 0);
    ui->gridLayout->addWidget(comboBoxDeckLinkFormat, row, 1);
    comboBoxDeckLinkFormat->setToolTip(tr("The same format as set on the device the card is connected to (e.g. the ATEM switcher). With another format it may show no signal."));
}

QString GeneralSettingWidget::cardItem(const QString &card)
{
    return tr("Blackmagic card: %1").arg(card);
}

void GeneralSettingWidget::updateDeckLinkFormat()
{
    bool card = !ui->comboBoxDisplayScreen_2->currentData().toString().isEmpty();
    labelDeckLinkFormat->setEnabled(card);
    comboBoxDeckLinkFormat->setEnabled(card);
}

GeneralSettingWidget::~GeneralSettingWidget()
{
    delete ui;
}

void GeneralSettingWidget::changeEvent(QEvent *e)
{
    QWidget::changeEvent(e);
     switch ( e->type() ) {
     case QEvent::LanguageChange:
         ui->retranslateUi(this);
         labelDeckLinkFormat->setText(tr("Card video format:"));
         comboBoxDeckLinkFormat->setToolTip(tr("The same format as set on the device the card is connected to (e.g. the ATEM switcher). With another format it may show no signal."));
         break;
     default:
         break;
     }
}

void GeneralSettingWidget::setSettings(GeneralSettings settings)
{
    mySettings = settings;
    loadSettings();
}

void GeneralSettingWidget::loadSettings()
{
    ui->checkBoxDisplayOnTop->setChecked(mySettings.displayIsOnTop);
    ui->checkBoxDisplayOnStartUp->setChecked(mySettings.displayOnStartUp);

    // Load Themes
    loadThemes();
    ui->comboBoxTheme->setCurrentIndex(themeIdList.indexOf(mySettings.currentThemeId));

    // Get display screen infomration
    monitors.clear();
    QDesktopWidget d;
    int screen_count = d.screenCount();
    ui->comboBoxDisplayScreen->clear();
    for(int i(0); i<screen_count;++i)
        monitors << QString("%1 - %2x%3").arg(i+1).arg(d.screenGeometry(i).width()).arg(d.screenGeometry(i).height());

    // A card that is not plugged in now stays chosen
    cards = DeckLinkOutput::devices();
    if(mySettings.displayScreen2 == DISPLAY_ON_DECKLINK && !mySettings.deckLinkDevice.isEmpty()
            && !cards.contains(mySettings.deckLinkDevice))
        cards.append(mySettings.deckLinkDevice);

    if(screen_count>1 || !cards.isEmpty())
        ui->groupBoxDisplayScreen->setEnabled(true);
    else
        ui->groupBoxDisplayScreen->setEnabled(false);

    // Fill display screen comboBoxes
    ui->comboBoxDisplayScreen->clear();
    ui->comboBoxDisplayScreen->addItems(monitors);
    ui->comboBoxDisplayScreen_2->clear();
    ui->comboBoxDisplayScreen_2->addItem(tr("none"));
    ui->comboBoxDisplayScreen_2->addItems(monitors);
    foreach(const QString &card, cards)
        ui->comboBoxDisplayScreen_2->addItem(cardItem(card), card);
    comboBoxDeckLinkFormat->setCurrentIndex(qBound(0, mySettings.deckLinkFormat, comboBoxDeckLinkFormat->count() - 1));

    // Set primary display screen
    if(mySettings.displayScreen<0 || mySettings.displayScreen>=screen_count)
        ui->comboBoxDisplayScreen->setCurrentIndex(0);
    else
        ui->comboBoxDisplayScreen->setCurrentIndex(mySettings.displayScreen);

    // Set secondaty display screen
    if(mySettings.displayScreen2 == DISPLAY_ON_DECKLINK)
        ui->comboBoxDisplayScreen_2->setCurrentIndex(ui->comboBoxDisplayScreen_2->findData(mySettings.deckLinkDevice));
    else if(mySettings.displayScreen2<-1 || mySettings.displayScreen2>=screen_count)
        ui->comboBoxDisplayScreen_2->setCurrentIndex(0);
    else
        ui->comboBoxDisplayScreen_2->setCurrentIndex(mySettings.displayScreen2+1);
    updateSecondaryDisplayScreen();

    // Set Display Controls
    if(screen_count>1)
        ui->groupBoxDisplayControls->setEnabled(false);
    else
        ui->groupBoxDisplayControls->setEnabled(true);
    ui->comboBoxIconSize->setCurrentIndex(mySettings.displayControls.buttonSize);
    ui->comboBoxControlsAlignV->setCurrentIndex(mySettings.displayControls.alignmentV);
    ui->comboBoxControlsAlignH->setCurrentIndex(mySettings.displayControls.alignmentH);
    ui->horizontalSliderOpacity->setValue(mySettings.displayControls.opacity*100);
}

void GeneralSettingWidget::loadThemes()
{
    QSqlQuery sq;
    sq.exec("SELECT id, name FROM Themes");
    themeIdList.clear();
    themes.clear();
    while(sq.next())
    {
        themeIdList<< sq.value(0).toInt();
        themes<<sq.value(1).toString();
    }
    ui->comboBoxTheme->clear();
    ui->comboBoxTheme->addItems(themes);
}

GeneralSettings GeneralSettingWidget::getSettings()
{
    mySettings.displayIsOnTop = ui->checkBoxDisplayOnTop->isChecked();
    mySettings.displayOnStartUp = ui->checkBoxDisplayOnStartUp->isChecked();

    int tmx = ui->comboBoxTheme->currentIndex();
    if(tmx != -1)
        mySettings.currentThemeId = themeIdList.at(tmx);
    else
        mySettings.currentThemeId = 0;

    mySettings.displayScreen = ui->comboBoxDisplayScreen->currentIndex();
    QString card = ui->comboBoxDisplayScreen_2->currentData().toString();
    if(!card.isEmpty())
    {
        mySettings.displayScreen2 = DISPLAY_ON_DECKLINK;
        mySettings.deckLinkDevice = card;
    }
    else
        mySettings.displayScreen2 = monitors.indexOf(ui->comboBoxDisplayScreen_2->currentText());
    mySettings.deckLinkFormat = comboBoxDeckLinkFormat->currentIndex();

    mySettings.displayControls.buttonSize = ui->comboBoxIconSize->currentIndex();
    mySettings.displayControls.alignmentV = ui->comboBoxControlsAlignV->currentIndex();
    mySettings.displayControls.alignmentH = ui->comboBoxControlsAlignH->currentIndex();
    qreal r = ui->horizontalSliderOpacity->value();
    r = r/100;
    mySettings.displayControls.opacity = r;

    return mySettings;
}

void GeneralSettingWidget::on_pushButtonDefault_clicked()
{
    GeneralSettings g;
    mySettings = g;
    loadSettings();
}

void GeneralSettingWidget::updateSecondaryDisplayScreen()
{
    QString ds1 = ui->comboBoxDisplayScreen->currentText();
    QString ds2 = ui->comboBoxDisplayScreen_2->currentText();
    QStringList monitors2 = monitors;
    monitors2.removeOne(ds1);

    ui->comboBoxDisplayScreen_2->clear();
    ui->comboBoxDisplayScreen_2->addItem(tr("None"));
    ui->comboBoxDisplayScreen_2->addItems(monitors2);
    foreach(const QString &card, cards)
        ui->comboBoxDisplayScreen_2->addItem(cardItem(card), card);

    int i = ui->comboBoxDisplayScreen_2->findText(ds2);
    if(i != -1)
        ui->comboBoxDisplayScreen_2->setCurrentIndex(i);
    else
    {
        ui->comboBoxDisplayScreen_2->setCurrentIndex(0);
        emit setDisp2Use(false);
    }
    updateDeckLinkFormat();
}

void GeneralSettingWidget::on_comboBoxDisplayScreen_activated(const QString &arg1)
{
    updateSecondaryDisplayScreen();
}

void GeneralSettingWidget::on_comboBoxDisplayScreen_2_activated(int index)
{
    if(index<=0)
        emit setDisp2Use(false);
    else
        emit setDisp2Use(true);
    updateDeckLinkFormat();
}

void GeneralSettingWidget::on_pushButtonAddTheme_clicked()
{
    Theme tm;
    ThemeInfo tmi;
    int nId;

    AddSongbookDialog theme_dia;
    theme_dia.setWindowTitle(tr("Edit Theme"));
    theme_dia.setWindowText(tr("Theme Name:"),tr("Comments:"));
    theme_dia.setSongbook(tr("Default"),tr("This theme will contain program default settings."));
    int ret = theme_dia.exec();
    switch(ret)
    {
    case AddSongbookDialog::Accepted:
        tmi.name = theme_dia.title;
        tmi.comments = theme_dia.info;
        tm.setThemeInfo(tmi);
        tm.saveThemeNew();
        nId = tm.getThemeId();

        loadThemes();

        ui->comboBoxTheme->setCurrentIndex(themeIdList.indexOf(nId));
        emit themeChanged(nId);

        break;
    case AddSongbookDialog::Rejected:
        break;
    }
}

void GeneralSettingWidget::on_comboBoxTheme_activated(int index)
{
    emit themeChanged(themeIdList.at(index));
}
