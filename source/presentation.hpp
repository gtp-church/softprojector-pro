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

#ifndef PRESENTATION_HPP
#define PRESENTATION_HPP

#include <QList>
#include <QString>
#include <QStringList>
#include <functional>

class Presentation
{
    // Slides shown one after another: made from a PDF, a PowerPoint file or pictures
public:
    Presentation();
    bool isDocument() const;

    QString name;
    QStringList sources;    // The PDF or PowerPoint file, or the picture files in slide order
    QString libraryPath;    // File or folder in the library; empty for a presentation opened from elsewhere
    QStringList slides;     // Slide images, filled by PresentationLoader::load()
    QStringList thumbnails; // Small images of the slides, same order
};

class PresentationLoader
{
public:
    static const int slideWidth = 1920;
    static const int thumbnailWidth = 320;

    static QString libraryDir();
    static QString fileFilter();
    static bool isDocument(const QString &path);
    static bool isPicture(const QString &path);

    // Presentations in the library folder: every PDF, PowerPoint file and picture,
    // and every subfolder of pictures. Slides are not loaded yet.
    static QList<Presentation> libraryPresentations();
    static Presentation fromFiles(const QStringList &files);

    // Prepares the slide images; documents are converted once and kept in a cache.
    // Takes a while for big documents, so it is run in a worker thread.
    static bool load(Presentation &presentation, const std::function<void(int done, int total)> &progress,
                     QString *error);
};

#endif // PRESENTATION_HPP
