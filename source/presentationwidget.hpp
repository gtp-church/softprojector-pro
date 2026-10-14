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
#include <QSlider>
#include <QStyledItemDelegate>
#include <QWidget>
#include "presentation.hpp"

class SlideDelegate : public QStyledItemDelegate
{
    // Draws a slide with its number in the corner and a frame when selected.
    // The width is fixed, or as wide as the list when it is 0.
public:
    explicit SlideDelegate(QListView *view);
    void setSlideWidth(int width);
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const;
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;

private:
    QListView *view;
    int slideWidth;
};

class PresentationWidget : public QWidget
{
    // The Presentations tab: PDF, PowerPoint and pictures shown slide by slide
    Q_OBJECT

public:
    explicit PresentationWidget(QWidget *parent = 0);
    ~PresentationWidget();
    void setCurrentSlide(const Presentation &presentation, int slide);
    void openPresentation(const QStringList &files);
    // A window with one slide in a big size, to read it without showing it on the screens
    static void showSlideViewer(QWidget *parent, const Presentation &presentation, int slide);
    // Scrolls a list of slides by a part of a slide per wheel step
    static void scrollSlides(QListView *list, QWheelEvent *wheel);
    // Lays the slides out again for a new list width, once the resizing is over
    static void relayoutSlidesLater(QListView *list);

signals:
    void sendPresentation(Presentation presentation, int slide);

public slots:
    void reloadLibrary();

protected:
    void changeEvent(QEvent *e);
    void showEvent(QShowEvent *e);
    bool eventFilter(QObject *object, QEvent *event);

private slots:
    void openFiles();
    void addToLibrary();
    void removeFromLibrary();
    void presentationSelected(int row);
    void goLive();
    void slideDoubleClicked();
    void setSlideSize(int width);
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
    QLabel *labelSize;
    QSlider *sliderSize;
    SlideDelegate *slideDelegate;
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
