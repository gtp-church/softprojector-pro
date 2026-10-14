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

#include "presentation.hpp"
#include "pdfrenderer.hpp"
#include <QCollator>
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QImageReader>
#include <QProcess>
#include <QStandardPaths>
#include <QTextStream>
#include <algorithm>
#include <atomic>

static std::atomic<bool> loadingStopped(false);

void PresentationLoader::stopLoading()
{
    loadingStopped = true;
}

Presentation::Presentation()
{
}

bool Presentation::isDocument() const
{
    return sources.count() == 1 && PresentationLoader::isDocument(sources.first());
}

QString PresentationLoader::libraryDir()
{
    return QCoreApplication::applicationDirPath() + "/" + QString::fromUtf8("Презентации");
}

QString PresentationLoader::fileFilter()
{
    return QCoreApplication::translate("PresentationLoader", "Presentations and pictures")
            + " (*.pdf *.pptx *.ppt *.ppsx *.pps *.png *.jpg *.jpeg *.bmp *.gif)";
}

bool PresentationLoader::isDocument(const QString &path)
{
    static const QStringList suffixes = QStringList() << "pdf" << "pptx" << "ppt" << "ppsx" << "pps";
    return suffixes.contains(QFileInfo(path).suffix().toLower());
}

bool PresentationLoader::isPicture(const QString &path)
{
    static const QStringList suffixes = QStringList() << "png" << "jpg" << "jpeg" << "bmp" << "gif";
    return suffixes.contains(QFileInfo(path).suffix().toLower());
}

static void sortNaturally(QStringList &names)
{
    // "Slide 2" before "Slide 10"
    QCollator collator;
    collator.setNumericMode(true);
    collator.setCaseSensitivity(Qt::CaseInsensitive);
    std::sort(names.begin(), names.end(), [&collator](const QString &a, const QString &b) {
        return collator.compare(QFileInfo(a).fileName(), QFileInfo(b).fileName()) < 0;
    });
}

static QStringList picturesIn(const QString &folder)
{
    QStringList pictures;
    foreach(const QFileInfo &file, QDir(folder).entryInfoList(QDir::Files))
    {
        if(PresentationLoader::isPicture(file.filePath()))
            pictures.append(file.absoluteFilePath());
    }
    sortNaturally(pictures);
    return pictures;
}

QList<Presentation> PresentationLoader::libraryPresentations()
{
    QDir dir(libraryDir());
    QStringList entries;
    foreach(const QFileInfo &entry, dir.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot))
    {
        if(entry.isDir() || isDocument(entry.filePath()) || isPicture(entry.filePath()))
            entries.append(entry.absoluteFilePath());
    }
    sortNaturally(entries);

    QList<Presentation> presentations;
    foreach(const QString &entry, entries)
    {
        QFileInfo info(entry);
        Presentation p;
        p.libraryPath = entry;
        if(info.isDir())
        {
            p.name = info.fileName();
            p.sources = picturesIn(entry);
            if(p.sources.isEmpty())
                continue;
        }
        else
        {
            p.name = info.completeBaseName();
            p.sources.append(entry);
        }
        presentations.append(p);
    }
    return presentations;
}

Presentation PresentationLoader::fromFiles(const QStringList &files)
{
    // A document is shown on its own; otherwise the pictures make one presentation
    Presentation p;
    foreach(const QString &file, files)
    {
        if(isDocument(file))
        {
            p.sources.append(QFileInfo(file).absoluteFilePath());
            p.name = QFileInfo(file).completeBaseName();
            return p;
        }
    }
    foreach(const QString &file, files)
    {
        if(isPicture(file))
            p.sources.append(QFileInfo(file).absoluteFilePath());
    }
    sortNaturally(p.sources);
    if(p.sources.count() == 1)
        p.name = QFileInfo(p.sources.first()).completeBaseName();
    else if(!p.sources.isEmpty())
        p.name = QFileInfo(p.sources.first()).dir().dirName();
    return p;
}

static QString cacheRoot()
{
    return QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "/presentations";
}

static QString cacheDirFor(const QStringList &sources)
{
    // A new cache folder whenever a file changes
    QCryptographicHash hash(QCryptographicHash::Sha1);
    hash.addData(QString("v1 %1 %2").arg(PresentationLoader::slideWidth).arg(PresentationLoader::thumbnailWidth).toUtf8());
    foreach(const QString &source, sources)
    {
        QFileInfo info(source);
        hash.addData(QString("|%1|%2|%3").arg(info.absoluteFilePath()).arg(info.size())
                     .arg(info.lastModified().toMSecsSinceEpoch()).toUtf8());
    }
    return cacheRoot() + "/" + hash.result().toHex();
}

static void removeOldCaches()
{
    // Keep the most recently made caches only
    QFileInfoList dirs = QDir(cacheRoot()).entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Time);
    for(int i(50); i < dirs.count(); ++i)
        QDir(dirs.at(i).absoluteFilePath()).removeRecursively();
}

static bool saveThumbnail(const QImage &slide, const QString &path)
{
    return slide.scaledToWidth(PresentationLoader::thumbnailWidth, Qt::SmoothTransformation).save(path, "PNG");
}

#ifdef Q_OS_WIN
static bool exportPowerPoint(const QString &file, const QString &outDir,
                             const std::function<void(int, int)> &progress, QString *error)
{
    // PowerPoint (installed on the computer) exports every shown slide as a picture
    const QString script = QString::fromUtf8(R"PS(
$ErrorActionPreference = 'Stop'
$app = New-Object -ComObject PowerPoint.Application
$openBefore = $app.Presentations.Count
try {
    $pres = $app.Presentations.Open($env:SP_PPT_IN, -1, 0, 0)
    try {
        $width = [int]$env:SP_PPT_WIDTH
        $height = [int][math]::Round($width * $pres.PageSetup.SlideHeight / $pres.PageSetup.SlideWidth)
        $slides = @($pres.Slides | Where-Object { -not $_.SlideShowTransition.Hidden })
        $i = 0
        foreach ($slide in $slides) {
            $i++
            $slide.Export((Join-Path $env:SP_PPT_OUT ('slide_{0:D3}.png' -f $i)), 'PNG', $width, $height)
            Write-Output ('{0}/{1}' -f $i, $slides.Count)
        }
    } finally { $pres.Close() }
} finally {
    if ($openBefore -eq 0) { $app.Quit() }
}
)PS");
    QProcess powershell;
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert("SP_PPT_IN", QDir::toNativeSeparators(file));
    env.insert("SP_PPT_OUT", QDir::toNativeSeparators(outDir));
    env.insert("SP_PPT_WIDTH", QString::number(PresentationLoader::slideWidth));
    powershell.setProcessEnvironment(env);
    QByteArray utf16(reinterpret_cast<const char *>(script.utf16()), script.size() * 2);
    powershell.start("powershell.exe", QStringList() << "-NoProfile" << "-NonInteractive"
                     << "-ExecutionPolicy" << "Bypass" << "-EncodedCommand" << QString(utf16.toBase64()));
    if(!powershell.waitForStarted())
    {
        *error = powershell.errorString();
        return false;
    }
    // PowerPoint can wait for an answer in a window nobody sees (a damaged or protected file).
    // Give up when no slide comes for a long time.
    const int stuckMs = 3 * 60 * 1000;
    QElapsedTimer sinceLastSlide;
    sinceLastSlide.start();
    while(powershell.state() != QProcess::NotRunning)
    {
        powershell.waitForReadyRead(1000);
        while(powershell.canReadLine())
        {
            QStringList done = QString::fromLocal8Bit(powershell.readLine()).trimmed().split('/');
            if(done.count() == 2)
            {
                progress(done.at(0).toInt(), done.at(1).toInt());
                sinceLastSlide.restart();
            }
        }
        if(loadingStopped)
        {
            powershell.kill();
            powershell.waitForFinished(5000);
            *error = QCoreApplication::translate("PresentationLoader", "Stopped.");
            return false;
        }
        if(sinceLastSlide.elapsed() > stuckMs)
        {
            powershell.kill();
            powershell.waitForFinished(5000);
            *error = QCoreApplication::translate("PresentationLoader",
                                                 "PowerPoint stopped responding. Close PowerPoint and try again.");
            return false;
        }
    }
    if(powershell.exitStatus() != QProcess::NormalExit || powershell.exitCode() != 0)
    {
        *error = QCoreApplication::translate("PresentationLoader", "PowerPoint could not open the presentation.")
                + "\n\n" + QString::fromLocal8Bit(powershell.readAllStandardError()).trimmed();
        return false;
    }
    return true;
}
#endif

bool PresentationLoader::load(Presentation &presentation, const std::function<void(int, int)> &progress,
                              QString *error)
{
    presentation.slides.clear();
    presentation.thumbnails.clear();
    if(presentation.sources.isEmpty())
    {
        *error = QCoreApplication::translate("PresentationLoader", "There are no slides.");
        return false;
    }

    QDir cache(cacheDirFor(presentation.sources));
    QFile list(cache.filePath("slides.txt"));

    // Ready from an earlier time
    if(list.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QTextStream in(&list);
        in.setCodec("UTF-8");
        while(!in.atEnd())
        {
            QStringList line = in.readLine().split('\t');
            if(line.count() == 2)
            {
                presentation.slides.append(cache.absoluteFilePath(line.at(0)));
                presentation.thumbnails.append(cache.absoluteFilePath(line.at(1)));
            }
        }
        list.close();
        bool complete = !presentation.slides.isEmpty();
        foreach(const QString &file, presentation.slides + presentation.thumbnails)
            complete = complete && QFile::exists(file);
        if(complete)
            return true;
        presentation.slides.clear();
        presentation.thumbnails.clear();
    }

    cache.removeRecursively();
    QDir().mkpath(cache.path());
    QString source = presentation.sources.first();
    QString suffix = QFileInfo(source).suffix().toLower();

    if(suffix == "pdf")
    {
        bool ok = PdfRenderer::render(source, slideWidth, [&](int page, int pageCount, const QImage &image) {
            QString slide = cache.filePath(QString("slide_%1.png").arg(page + 1, 3, 10, QChar('0')));
            QString thumbnail = cache.filePath(QString("thumb_%1.png").arg(page + 1, 3, 10, QChar('0')));
            image.save(slide, "PNG");
            saveThumbnail(image, thumbnail);
            presentation.slides.append(slide);
            presentation.thumbnails.append(thumbnail);
            progress(page + 1, pageCount);
            return !loadingStopped;
        }, error);
        if(!ok)
            return false;
    }
    else if(isDocument(source))
    {
#ifdef Q_OS_WIN
        if(!exportPowerPoint(source, cache.path(), progress, error))
            return false;
        QStringList slides = cache.entryList(QStringList("slide_*.png"), QDir::Files, QDir::Name);
        for(int i(0); i < slides.count(); ++i)
        {
            QString thumbnail = cache.filePath(QString("thumb_%1.png").arg(i + 1, 3, 10, QChar('0')));
            saveThumbnail(QImage(cache.filePath(slides.at(i))), thumbnail);
            presentation.slides.append(cache.filePath(slides.at(i)));
            presentation.thumbnails.append(thumbnail);
        }
#else
        *error = QCoreApplication::translate("PresentationLoader",
                                             "PowerPoint files can be shown only on Windows with PowerPoint installed. "
                                             "Save the presentation as PDF to show it here.");
        return false;
#endif
    }
    else
    {
        // Pictures are shown from their own files; only the small images are made
        for(int i(0); i < presentation.sources.count() && !loadingStopped; ++i)
        {
            QImageReader reader(presentation.sources.at(i));
            reader.setAutoTransform(true);
            QImage image = reader.read();
            if(image.isNull())
                continue;
            QString thumbnail = cache.filePath(QString("thumb_%1.png").arg(i + 1, 3, 10, QChar('0')));
            saveThumbnail(image, thumbnail);
            presentation.slides.append(presentation.sources.at(i));
            presentation.thumbnails.append(thumbnail);
            progress(i + 1, presentation.sources.count());
        }
    }

    if(loadingStopped)
    {
        cache.removeRecursively(); // Not complete
        *error = QCoreApplication::translate("PresentationLoader", "Stopped.");
        return false;
    }
    if(presentation.slides.isEmpty())
    {
        *error = QCoreApplication::translate("PresentationLoader", "There are no slides.");
        return false;
    }

    if(list.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        QTextStream out(&list);
        out.setCodec("UTF-8");
        for(int i(0); i < presentation.slides.count(); ++i)
            out << cache.relativeFilePath(presentation.slides.at(i)) << "\t"
                << cache.relativeFilePath(presentation.thumbnails.at(i)) << "\n";
    }
    removeOldCaches();
    return true;
}
