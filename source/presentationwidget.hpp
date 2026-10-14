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

#ifndef PRESENTATIONWIDGET_HPP
#define PRESENTATIONWIDGET_HPP

#include <QFutureWatcher>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QWidget>
#include "presentation.hpp"

class PresentationWidget : public QWidget
{
    // The Presentations tab: PDF, PowerPoint and pictures shown slide by slide
    Q_OBJECT

public:
    explicit PresentationWidget(QWidget *parent = 0);
    void setCurrentSlide(const Presentation &presentation, int slide);
    void openPresentation(const QStringList &files);

signals:
    void sendPresentation(Presentation presentation, int slide);

public slots:
    void reloadLibrary();

protected:
    void changeEvent(QEvent *e);
    void showEvent(QShowEvent *e);

private slots:
    void openFiles();
    void addToLibrary();
    void removeFromLibrary();
    void presentationSelected(int row);
    void goLive();
    void loadFinished();

private:
    class LoadResult
    {
    public:
        Presentation presentation;
        bool ok;
        QString error;
    };

    void retranslateUi();
    void addPresentationItem(const Presentation &presentation);
    void loadPresentation(int row);
    void updateButtons();

    QListWidget *listPresentations;
    QListWidget *listSlides;
    QLabel *labelTitle;
    QLabel *labelStatus;
    QPushButton *pushButtonOpen;
    QPushButton *pushButtonAddToLibrary;
    QPushButton *pushButtonRemove;
    QPushButton *pushButtonGoLive;

    QList<Presentation> presentations; // Same order as listPresentations
    Presentation current;              // The one whose slides are shown
    int loadingRow;
    int pendingRow;
    QFutureWatcher<LoadResult> loader;
};

#endif // PRESENTATIONWIDGET_HPP
