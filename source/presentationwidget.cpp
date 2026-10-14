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
#include <QSplitter>
#include <QVBoxLayout>
#include <QtConcurrent>

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
    listSlides->setIconSize(QSize(192, 108));
    listSlides->setResizeMode(QListView::Adjust);
    listSlides->setMovement(QListView::Static);
    listSlides->setSpacing(6);
    listSlides->setUniformItemSizes(true);
    labelStatus = new QLabel;
    labelStatus->setWordWrap(true);

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
    rightLayout->addWidget(labelStatus);

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
    connect(listSlides, &QListWidget::itemDoubleClicked, this, &PresentationWidget::goLive);
    connect(&loader, &QFutureWatcher<LoadResult>::finished, this, &PresentationWidget::loadFinished);

    retranslateUi();
    reloadLibrary();
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

    QLabel *status = labelStatus;
    loader.setFuture(QtConcurrent::run([p, status]() {
        LoadResult result;
        result.presentation = p;
        result.ok = PresentationLoader::load(result.presentation, [status](int done, int total) {
            QMetaObject::invokeMethod(status, "setText", Qt::QueuedConnection,
                                      Q_ARG(QString, PresentationWidget::tr("Preparing slides: %1 of %2").arg(done).arg(total)));
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
