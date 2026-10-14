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

#ifndef PDFRENDERER_HPP
#define PDFRENDERER_HPP

#include <QImage>
#include <QString>
#include <functional>

class PdfRenderer
{
    // Renders PDF pages with the PDFium library (pdfium.dll / libpdfium.dylib next to the program).
    // The library is loaded when first needed; without it PDF files cannot be opened.
public:
    static bool isAvailable();

    // Calls pageReady for every page rendered to the given width. Returns false and sets error on failure.
    static bool render(const QString &pdfPath, int width,
                       const std::function<void(int page, int pageCount, const QImage &image)> &pageReady,
                       QString *error);
};

#endif // PDFRENDERER_HPP
