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

#include "presentationwidget.hpp"
#include <QDir>
#include <QEvent>
#include <QFileDialog>
#include <QFileIconProvider>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QMessageBox>
#include <QRegularExpression>
#include <QApplication>
#include <QDesktopWidget>
#include <QDialog>
#include <QImageReader>
#include <QKeyEvent>
#include <QPainter>
#include <QScrollBar>
#include <QSettings>
#include <QPointer>
#include <QSplitter>
#include <QTimer>
#include <QVBoxLayout>
#include <QWheelEvent>
#include <QtConcurrent>

SlideDelegate::SlideDelegate(QListView *view) :
    QStyledItemDelegate(view), view(view), slideWidth(0)
{
}

void SlideDelegate::setSlideWidth(int width)
{
    slideWidth = width;
}

QSize SlideDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(option);
    Q_UNUSED(index);
    // Never wider than the list, so the biggest size is one slide across it
    int available = view->viewport()->width() - 2 * view->spacing() - 2;
    int width = slideWidth > 0 ? qMin(slideWidth, available) : available;
    width = qMax(60, width);
    return QSize(width, width * 9 / 16 + 6);
}

void SlideDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    painter->save();
    QRect box = option.rect.adjusted(3, 3, -3, -3);
    painter->fillRect(box, Qt::black);

    // The slide keeps its shape inside a 16:9 box
    QIcon icon = qvariant_cast<QIcon>(index.data(Qt::DecorationRole));
    QPixmap slide = icon.pixmap(box.size());
    QRect target(QPoint(0, 0), slide.size().scaled(box.size(), Qt::KeepAspectRatio));
    target.moveCenter(box.center());
    painter->setRenderHint(QPainter::SmoothPixmapTransform);
    painter->drawPixmap(target, slide);

    QString number = index.data(Qt::DisplayRole).toString();
    QFont font = option.font;
    font.setBold(true);
    painter->setFont(font);
    QRect badge = painter->fontMetrics().boundingRect(number).adjusted(-6, -2, 6, 2);
    badge.moveTopLeft(target.topLeft() + QPoint(4, 4));
    painter->fillRect(badge, QColor(0, 0, 0, 170));
    painter->setPen(Qt::white);
    painter->drawText(badge, Qt::AlignCenter, number);

    if(option.state & QStyle::State_Selected)
    {
        painter->setPen(QPen(option.palette.highlight(), 4));
        painter->setBrush(Qt::NoBrush);
        painter->drawRect(option.rect.adjusted(2, 2, -2, -2));
    }
    painter->restore();
}

class SlideViewer : public QDialog
{
    // One slide in a big size; arrows go through the slides, Esc or Space closes
public:
    SlideViewer(QWidget *parent, const Presentation &presentation, int slide) :
        QDialog(parent), presentation(presentation), slide(slide)
    {
        label = new QLabel;
        label->setAlignment(Qt::AlignCenter);
        label->setStyleSheet("background-color: black;");
        label->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
        QVBoxLayout *layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->addWidget(label);
        QRect screen = QApplication::desktop()->availableGeometry(parent);
        resize(screen.width() * 8 / 10, screen.height() * 8 / 10);
        showSlide();
    }

protected:
    void keyPressEvent(QKeyEvent *e)
    {
        int count = presentation.slides.count();
        switch(e->key())
        {
        case Qt::Key_Left: case Qt::Key_Up: case Qt::Key_PageUp:
            slide = qMax(0, slide - 1);
            break;
        case Qt::Key_Right: case Qt::Key_Down: case Qt::Key_PageDown:
            slide = qMin(count - 1, slide + 1);
            break;
        case Qt::Key_Home:
            slide = 0;
            break;
        case Qt::Key_End:
            slide = count - 1;
            break;
        case Qt::Key_Space:
            reject();
            return;
        default:
            QDialog::keyPressEvent(e);
            return;
        }
        showSlide();
    }

    void resizeEvent(QResizeEvent *e)
    {
        QDialog::resizeEvent(e);
        updatePicture();
    }

private:
    void showSlide()
    {
        QImageReader reader(presentation.slides.at(slide));
        reader.setAutoTransform(true);
        picture = QPixmap::fromImage(reader.read());
        setWindowTitle(QString("%1 - %2").arg(presentation.name)
                       .arg(QCoreApplication::translate("PresentationWidget", "Slide %1 of %2")
                            .arg(slide + 1).arg(presentation.slides.count())));
        updatePicture();
    }

    void updatePicture()
    {
        label->setPixmap(picture.scaled(label->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }

    Presentation presentation;
    int slide;
    QLabel *label;
    QPixmap picture;
};

void PresentationWidget::relayoutSlidesLater(QListView *list)
{
    // Not right away: the layout itself can resize the list (scroll bar), which would call it again
    // and again. Several resizes in a row lead to one layout.
    if(list->property("relayoutPending").toBool())
        return;
    list->setProperty("relayoutPending", true);
    QTimer::singleShot(0, list, [list]() {
        list->setProperty("relayoutPending", false);
        list->doItemsLayout();
    });
}

void PresentationWidget::scrollSlides(QListView *list, QWheelEvent *wheel)
{
    QScrollBar *bar = list->verticalScrollBar();
    int step = qMax(40, list->sizeHintForRow(0) / 3);
    bar->setValue(bar->value() - wheel->angleDelta().y() * step / 120);
}

void PresentationWidget::showSlideViewer(QWidget *parent, const Presentation &presentation, int slide)
{
    if(slide < 0 || slide >= presentation.slides.count())
        return;
    SlideViewer viewer(parent, presentation, slide);
    viewer.exec();
}

static QString keyOf(const Presentation &p)
{
    return p.sources.join("\n");
}

static QString uniquePath(const QString &folder, const QString &fileName)
{
    // "Name.pdf", then "Name (2).pdf", "Name (3).pdf"...
    QFileInfo info(fileName);
    QString path = folder + "/" + fileName;
    for(int i(2); QFileInfo::exists(path); ++i)
    {
        QString name = QString("%1 (%2)").arg(info.completeBaseName()).arg(i);
        if(!info.suffix().isEmpty())
            name += "." + info.suffix();
        path = folder + "/" + name;
    }
    return path;
}

PresentationWidget::PresentationWidget(QWidget *parent) :
    QWidget(parent), loadingRow(-1)
{
    listPresentations = new QListWidget;
    listPresentations->setIconSize(QSize(24, 24));
    pushButtonOpen = new QPushButton(QIcon(":/icons/icons/open.png"), QString());
    pushButtonAddToLibrary = new QPushButton(QIcon(":/icons/icons/add.png"), QString());
    pushButtonRemove = new QPushButton(QIcon(":/icons/icons/remove.png"), QString());

    labelTitle = new QLabel;
    QFont titleFont = labelTitle->font();
    titleFont.setBold(true);
    labelTitle->setFont(titleFont);
    pushButtonGoLive = new QPushButton(QIcon(":/icons/icons/go_live.png"), QString());
    pushButtonGoLive->setObjectName("pushButtonGoLive"); // Found by name for the "Go Live" keys
    listSlides = new QListWidget;
    listSlides->setViewMode(QListView::IconMode);
    listSlides->setResizeMode(QListView::Adjust);
    listSlides->setMovement(QListView::Static);
    listSlides->setSpacing(4);
    listSlides->setUniformItemSizes(true);
    listSlides->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    // The slide width follows the list width; a scroll bar that comes and goes would change it every time
    listSlides->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    slideDelegate = new SlideDelegate(listSlides);
    listSlides->setItemDelegate(slideDelegate);
    listSlides->installEventFilter(this);
    listSlides->viewport()->installEventFilter(this);
    labelStatus = new QLabel;
    labelStatus->setWordWrap(true);
    labelSize = new QLabel;
    sliderSize = new QSlider(Qt::Horizontal);
    sliderSize->setRange(120, PresentationLoader::thumbnailWidth);
    sliderSize->setMaximumWidth(200);

    QWidget *left = new QWidget;
    QVBoxLayout *leftLayout = new QVBoxLayout(left);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->addWidget(listPresentations);
    leftLayout->addWidget(pushButtonOpen);
    leftLayout->addWidget(pushButtonAddToLibrary);
    leftLayout->addWidget(pushButtonRemove);

    QWidget *right = new QWidget;
    QVBoxLayout *rightLayout = new QVBoxLayout(right);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    QHBoxLayout *titleLayout = new QHBoxLayout;
    titleLayout->addWidget(labelTitle, 1);
    titleLayout->addWidget(pushButtonGoLive);
    rightLayout->addLayout(titleLayout);
    rightLayout->addWidget(listSlides, 1);
    QHBoxLayout *statusLayout = new QHBoxLayout;
    statusLayout->addWidget(labelStatus, 1);
    statusLayout->addWidget(labelSize);
    statusLayout->addWidget(sliderSize);
    rightLayout->addLayout(statusLayout);

    QSplitter *splitter = new QSplitter;
    splitter->addWidget(left);
    splitter->addWidget(right);
    splitter->setStretchFactor(1, 1);
    splitter->setSizes(QList<int>() << 200 << 500);
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->addWidget(splitter);

    connect(pushButtonOpen, &QPushButton::clicked, this, &PresentationWidget::openFiles);
    connect(pushButtonAddToLibrary, &QPushButton::clicked, this, &PresentationWidget::addToLibrary);
    connect(pushButtonRemove, &QPushButton::clicked, this, &PresentationWidget::removeFromLibrary);
    connect(pushButtonGoLive, &QPushButton::clicked, this, &PresentationWidget::goLive);
    connect(listPresentations, &QListWidget::currentRowChanged, this, &PresentationWidget::presentationSelected);
    connect(listSlides, &QListWidget::itemDoubleClicked, this, &PresentationWidget::slideDoubleClicked);
    connect(sliderSize, &QSlider::valueChanged, this, &PresentationWidget::setSlideSize);
    connect(&loader, &QFutureWatcher<LoadResult>::finished, this, &PresentationWidget::loadFinished);

    sliderSize->setValue(QSettings("SoftProjector", "SoftProjector").value("presentationSlideSize", 240).toInt());
    setSlideSize(sliderSize->value());
    retranslateUi();
    reloadLibrary();
}

void PresentationWidget::setSlideSize(int width)
{
    slideDelegate->setSlideWidth(width);
    listSlides->doItemsLayout();
    QSettings("SoftProjector", "SoftProjector").setValue("presentationSlideSize", width);
}

PresentationWidget::~PresentationWidget()
{
    // Do not leave slides being made when the program closes
    PresentationLoader::stopLoading();
    loader.waitForFinished();
}

bool PresentationWidget::eventFilter(QObject *object, QEvent *event)
{
    if(object == listSlides->viewport() && event->type() == QEvent::Resize)
        relayoutSlidesLater(listSlides);
    // Ctrl + mouse wheel changes the size of the slides; Space shows the selected slide big
    if(object == listSlides->viewport() && event->type() == QEvent::Wheel)
    {
        QWheelEvent *wheel = static_cast<QWheelEvent *>(event);
        if(wheel->modifiers() & Qt::ControlModifier)
        {
            sliderSize->setValue(sliderSize->value() + (wheel->angleDelta().y() > 0 ? 40 : -40));
            return true;
        }
        scrollSlides(listSlides, wheel);
        return true;
    }
    if(object == listSlides && event->type() == QEvent::KeyPress
            && static_cast<QKeyEvent *>(event)->key() == Qt::Key_Space)
    {
        showSlideViewer(this, current, listSlides->currentRow());
        return true;
    }
    return QWidget::eventFilter(object, event);
}

void PresentationWidget::slideDoubleClicked()
{
    if(QApplication::keyboardModifiers() & Qt::ControlModifier)
        showSlideViewer(this, current, listSlides->currentRow());
    else
        goLive();
}

void PresentationWidget::changeEvent(QEvent *e)
{
    QWidget::changeEvent(e);
    if(e->type() == QEvent::LanguageChange)
        retranslateUi();
}

void PresentationWidget::showEvent(QShowEvent *e)
{
    // Files may have been added to the library folder meanwhile
    QWidget::showEvent(e);
    reloadLibrary();
}

void PresentationWidget::retranslateUi()
{
    pushButtonOpen->setText(tr("Open File..."));
    pushButtonOpen->setToolTip(tr("Open a PDF, a PowerPoint presentation or pictures from any folder"));
    pushButtonAddToLibrary->setText(tr("Add to Library"));
    pushButtonAddToLibrary->setToolTip(tr("Keep this presentation in the library: the Presentations folder next to the program"));
    pushButtonGoLive->setText(tr("Go Live (F5)"));
    labelSize->setText(tr("Size:"));
    sliderSize->setToolTip(tr("Size of the slides (also Ctrl + mouse wheel)"));
    listSlides->setToolTip(tr("Double-click: show on the screens. Ctrl + double-click or Space: look at the slide closely."));
    updateButtons();
    if(current.slides.isEmpty() && !loader.isRunning())
        labelStatus->setText(tr("Choose a presentation on the left or open a file."));
}

void PresentationWidget::addPresentationItem(const Presentation &presentation)
{
    QString path = presentation.libraryPath.isEmpty() ? presentation.sources.first() : presentation.libraryPath;
    QListWidgetItem *item = new QListWidgetItem(QFileIconProvider().icon(QFileInfo(path)), presentation.name);
    if(presentation.libraryPath.isEmpty())
    {
        // Opened from elsewhere, not in the library
        QFont font = item->font();
        font.setItalic(true);
        item->setFont(font);
        item->setToolTip(tr("Opened from %1").arg(QDir::toNativeSeparators(QFileInfo(path).absolutePath())));
    }
    listPresentations->addItem(item);
}

void PresentationWidget::reloadLibrary()
{
    int row = listPresentations->currentRow();
    QString selected = (row >= 0 && row < presentations.count()) ? keyOf(presentations.at(row)) : QString();

    QList<Presentation> opened;
    foreach(const Presentation &p, presentations)
    {
        if(p.libraryPath.isEmpty())
            opened.append(p);
    }
    QDir().mkpath(PresentationLoader::libraryDir());
    presentations = opened + PresentationLoader::libraryPresentations();

    QSignalBlocker blocker(listPresentations);
    listPresentations->clear();
    int selectRow = -1;
    for(int i(0); i < presentations.count(); ++i)
    {
        addPresentationItem(presentations.at(i));
        if(keyOf(presentations.at(i)) == selected)
            selectRow = i;
    }
    listPresentations->setCurrentRow(selectRow);
    updateButtons();
}

void PresentationWidget::presentationSelected(int row)
{
    updateButtons();
    if(row < 0 || row >= presentations.count())
        return;
    if(keyOf(presentations.at(row)) == keyOf(current) && !current.slides.isEmpty())
        return;
    loadPresentation(row);
}

void PresentationWidget::loadPresentation(int row)
{
    // A presentation is loaded in a worker thread; another one chosen meanwhile is loaded after it
    if(loader.isRunning())
        return;
    loadingRow = row;
    Presentation p = presentations.at(row);
    current = Presentation();
    listSlides->clear();
    labelTitle->setText(p.name);
    labelStatus->setText(tr("Preparing slides..."));
    updateButtons();

    // The label may be gone by the time a progress message arrives (the program is closing)
    QPointer<QLabel> status = labelStatus;
    loader.setFuture(QtConcurrent::run([p, status]() {
        LoadResult result;
        result.presentation = p;
        result.ok = PresentationLoader::load(result.presentation, [status](int done, int total) {
            QString text = PresentationWidget::tr("Preparing slides: %1 of %2").arg(done).arg(total);
            QMetaObject::invokeMethod(qApp, [status, text]() {
                if(status)
                    status->setText(text);
            }, Qt::QueuedConnection);
        }, &result.error);
        return result;
    }));
}

void PresentationWidget::loadFinished()
{
    LoadResult result = loader.result();

    // Another presentation was chosen while this one was loading
    int row = listPresentations->currentRow();
    if(row >= 0 && row < presentations.count() && keyOf(presentations.at(row)) != keyOf(result.presentation))
    {
        loadPresentation(row);
        return;
    }

    if(!result.ok)
    {
        current = Presentation();
        labelStatus->setText(result.error);
        updateButtons();
        return;
    }

    current = result.presentation;
    labelTitle->setText(current.name);
    for(int i(0); i < current.slides.count(); ++i)
        listSlides->addItem(new QListWidgetItem(QIcon(current.thumbnails.at(i)), QString::number(i + 1)));
    listSlides->setCurrentRow(0);
    labelStatus->setText(tr("Slides: %1").arg(current.slides.count()));
    updateButtons();
}

void PresentationWidget::updateButtons()
{
    int row = listPresentations->currentRow();
    bool valid = (row >= 0 && row < presentations.count());
    bool inLibrary = valid && !presentations.at(row).libraryPath.isEmpty();
    pushButtonAddToLibrary->setEnabled(valid && !inLibrary);
    pushButtonRemove->setEnabled(valid);
    pushButtonRemove->setText(inLibrary || !valid ? tr("Delete from Library") : tr("Close"));
    pushButtonGoLive->setEnabled(!current.slides.isEmpty());
}

void PresentationWidget::goLive()
{
    if(current.slides.isEmpty())
        return;
    emit sendPresentation(current, qMax(0, listSlides->currentRow()));
}

void PresentationWidget::setCurrentSlide(const Presentation &presentation, int slide)
{
    // Keeps the selected slide in step with the screens, so the tab shows where the presentation is
    if(keyOf(presentation) != keyOf(current) || slide < 0 || slide >= listSlides->count())
        return;
    QSignalBlocker blocker(listSlides);
    listSlides->setCurrentRow(slide);
}

void PresentationWidget::openFiles()
{
    static QString lastFolder;
    QStringList files = QFileDialog::getOpenFileNames(this, tr("Open Presentation"), lastFolder,
                                                      PresentationLoader::fileFilter());
    if(files.isEmpty())
        return;
    lastFolder = QFileInfo(files.first()).absolutePath();
    openPresentation(files);
}

void PresentationWidget::openPresentation(const QStringList &files)
{
    Presentation p = PresentationLoader::fromFiles(files);
    if(p.sources.isEmpty())
        return;
    for(int i(0); i < presentations.count(); ++i)
    {
        if(keyOf(presentations.at(i)) == keyOf(p))
        {
            listPresentations->setCurrentRow(i);
            return;
        }
    }
    presentations.prepend(p);
    {
        QSignalBlocker blocker(listPresentations);
        listPresentations->clear();
        foreach(const Presentation &presentation, presentations)
            addPresentationItem(presentation);
    }
    listPresentations->setCurrentRow(0);
}

void PresentationWidget::addToLibrary()
{
    int row = listPresentations->currentRow();
    if(row < 0 || row >= presentations.count() || !presentations.at(row).libraryPath.isEmpty())
        return;
    Presentation p = presentations.at(row);
    QString library = PresentationLoader::libraryDir();
    QDir().mkpath(library);

    QString target;
    if(p.isDocument())
    {
        target = uniquePath(library, QFileInfo(p.sources.first()).fileName());
        if(!QFile::copy(p.sources.first(), target))
        {
            QMessageBox::warning(this, tr("Add to Library"), tr("Could not copy %1 to the library.").arg(p.name));
            return;
        }
    }
    else
    {
        // Pictures go to a folder of their own
        bool ok;
        QString name = QInputDialog::getText(this, tr("Add to Library"), tr("Name of the presentation:"),
                                             QLineEdit::Normal, p.name, &ok).trimmed();
        name.remove(QRegularExpression("[\\\\/:*?\"<>|]"));
        if(!ok || name.isEmpty())
            return;
        target = uniquePath(library, name);
        QDir().mkpath(target);
        foreach(const QString &picture, p.sources)
        {
            if(!QFile::copy(picture, target + "/" + QFileInfo(picture).fileName()))
            {
                QMessageBox::warning(this, tr("Add to Library"), tr("Could not copy %1 to the library.").arg(picture));
                return;
            }
        }
    }

    // The library copy takes the place of the opened one
    presentations.removeAt(row);
    reloadLibrary();
    for(int i(0); i < presentations.count(); ++i)
    {
        if(presentations.at(i).libraryPath == QFileInfo(target).absoluteFilePath())
            listPresentations->setCurrentRow(i);
    }
}

void PresentationWidget::removeFromLibrary()
{
    int row = listPresentations->currentRow();
    if(row < 0 || row >= presentations.count())
        return;
    Presentation p = presentations.at(row);

    if(!p.libraryPath.isEmpty())
    {
        if(QMessageBox::question(this, tr("Delete from Library"),
                                 tr("Delete \"%1\" from the library? It will be moved to the Recycle Bin.").arg(p.name))
                != QMessageBox::Yes)
            return;
        if(!QFile::moveToTrash(p.libraryPath))
        {
            QMessageBox::warning(this, tr("Delete from Library"),
                                 tr("Could not move %1 to the Recycle Bin.").arg(QDir::toNativeSeparators(p.libraryPath)));
            return;
        }
    }

    presentations.removeAt(row);
    if(keyOf(p) == keyOf(current))
    {
        current = Presentation();
        listSlides->clear();
        labelTitle->clear();
        labelStatus->setText(tr("Choose a presentation on the left or open a file."));
    }
    reloadLibrary();
}
