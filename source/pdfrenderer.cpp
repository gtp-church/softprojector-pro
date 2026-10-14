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

#include "pdfrenderer.hpp"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QLibrary>
#include <QMutex>

// The few PDFium functions used here (see fpdfview.h of PDFium). On 64-bit Windows and on
// macOS there is a single calling convention, so plain function pointers match the library.
namespace
{
typedef void *FPDF_DOCUMENT;
typedef void *FPDF_PAGE;
typedef void *FPDF_BITMAP;

typedef void (*InitLibraryFn)();
typedef FPDF_DOCUMENT (*LoadMemDocumentFn)(const void *data, int size, const char *password);
typedef unsigned long (*GetLastErrorFn)();
typedef int (*GetPageCountFn)(FPDF_DOCUMENT document);
typedef FPDF_PAGE (*LoadPageFn)(FPDF_DOCUMENT document, int index);
typedef float (*GetPageSizeFn)(FPDF_PAGE page);
typedef FPDF_BITMAP (*BitmapCreateFn)(int width, int height, int alpha);
typedef int (*BitmapFillRectFn)(FPDF_BITMAP bitmap, int left, int top, int width, int height, unsigned int color);
typedef void (*RenderPageBitmapFn)(FPDF_BITMAP bitmap, FPDF_PAGE page, int x, int y, int width, int height,
                                   int rotate, int flags);
typedef void *(*BitmapGetBufferFn)(FPDF_BITMAP bitmap);
typedef int (*BitmapGetStrideFn)(FPDF_BITMAP bitmap);
typedef void (*BitmapDestroyFn)(FPDF_BITMAP bitmap);
typedef void (*ClosePageFn)(FPDF_PAGE page);
typedef void (*CloseDocumentFn)(FPDF_DOCUMENT document);

const int FPDF_ANNOT = 0x01; // Render annotations too

struct Pdfium
{
    bool loaded = false;
    InitLibraryFn initLibrary = nullptr;
    LoadMemDocumentFn loadMemDocument = nullptr;
    GetLastErrorFn getLastError = nullptr;
    GetPageCountFn getPageCount = nullptr;
    LoadPageFn loadPage = nullptr;
    GetPageSizeFn getPageWidth = nullptr;
    GetPageSizeFn getPageHeight = nullptr;
    BitmapCreateFn bitmapCreate = nullptr;
    BitmapFillRectFn bitmapFillRect = nullptr;
    RenderPageBitmapFn renderPageBitmap = nullptr;
    BitmapGetBufferFn bitmapGetBuffer = nullptr;
    BitmapGetStrideFn bitmapGetStride = nullptr;
    BitmapDestroyFn bitmapDestroy = nullptr;
    ClosePageFn closePage = nullptr;
    CloseDocumentFn closeDocument = nullptr;
};

// PDFium is not thread safe: one document at a time
QMutex pdfiumMutex;

Pdfium &pdfium()
{
    static Pdfium p;
    static bool tried = false;
    if(tried)
        return p;
    tried = true;

    static QLibrary library(QCoreApplication::applicationDirPath() + QDir::separator() + "pdfium");
    if(!library.load())
        return p;
    p.initLibrary = (InitLibraryFn)library.resolve("FPDF_InitLibrary");
    p.loadMemDocument = (LoadMemDocumentFn)library.resolve("FPDF_LoadMemDocument");
    p.getLastError = (GetLastErrorFn)library.resolve("FPDF_GetLastError");
    p.getPageCount = (GetPageCountFn)library.resolve("FPDF_GetPageCount");
    p.loadPage = (LoadPageFn)library.resolve("FPDF_LoadPage");
    p.getPageWidth = (GetPageSizeFn)library.resolve("FPDF_GetPageWidthF");
    p.getPageHeight = (GetPageSizeFn)library.resolve("FPDF_GetPageHeightF");
    p.bitmapCreate = (BitmapCreateFn)library.resolve("FPDFBitmap_Create");
    p.bitmapFillRect = (BitmapFillRectFn)library.resolve("FPDFBitmap_FillRect");
    p.renderPageBitmap = (RenderPageBitmapFn)library.resolve("FPDF_RenderPageBitmap");
    p.bitmapGetBuffer = (BitmapGetBufferFn)library.resolve("FPDFBitmap_GetBuffer");
    p.bitmapGetStride = (BitmapGetStrideFn)library.resolve("FPDFBitmap_GetStride");
    p.bitmapDestroy = (BitmapDestroyFn)library.resolve("FPDFBitmap_Destroy");
    p.closePage = (ClosePageFn)library.resolve("FPDF_ClosePage");
    p.closeDocument = (CloseDocumentFn)library.resolve("FPDF_CloseDocument");
    p.loaded = p.initLibrary && p.loadMemDocument && p.getLastError && p.getPageCount && p.loadPage
            && p.getPageWidth && p.getPageHeight && p.bitmapCreate && p.bitmapFillRect && p.renderPageBitmap
            && p.bitmapGetBuffer && p.bitmapGetStride && p.bitmapDestroy && p.closePage && p.closeDocument;
    if(p.loaded)
        p.initLibrary();
    return p;
}
}

bool PdfRenderer::isAvailable()
{
    QMutexLocker locker(&pdfiumMutex);
    return pdfium().loaded;
}

bool PdfRenderer::render(const QString &pdfPath, int width,
                         const std::function<void(int, int, const QImage &)> &pageReady, QString *error)
{
    QMutexLocker locker(&pdfiumMutex);
    Pdfium &p = pdfium();
    if(!p.loaded)
    {
        *error = QCoreApplication::translate("PdfRenderer", "The PDF library (pdfium) was not found next to the program.");
        return false;
    }

    // The file is read into memory: PDFium's own file loading does not handle every file name on Windows
    QFile file(pdfPath);
    if(!file.open(QIODevice::ReadOnly))
    {
        *error = file.errorString();
        return false;
    }
    QByteArray data = file.readAll();
    FPDF_DOCUMENT document = p.loadMemDocument(data.constData(), data.size(), nullptr);
    if(!document)
    {
        *error = QCoreApplication::translate("PdfRenderer", "The PDF file cannot be opened (error %1).")
                .arg(p.getLastError());
        return false;
    }

    int pageCount = p.getPageCount(document);
    for(int i(0); i < pageCount; ++i)
    {
        FPDF_PAGE page = p.loadPage(document, i);
        if(!page)
            continue;
        float pageWidth = p.getPageWidth(page);
        float pageHeight = p.getPageHeight(page);
        int height = (pageWidth > 0) ? qRound(width * pageHeight / pageWidth) : width;

        FPDF_BITMAP bitmap = p.bitmapCreate(width, height, 0);
        if(bitmap)
        {
            p.bitmapFillRect(bitmap, 0, 0, width, height, 0xFFFFFFFF);
            p.renderPageBitmap(bitmap, page, 0, 0, width, height, 0, FPDF_ANNOT);
            // The bitmap is BGRx, which is QImage::Format_RGB32 in memory
            QImage image((const uchar *)p.bitmapGetBuffer(bitmap), width, height,
                         p.bitmapGetStride(bitmap), QImage::Format_RGB32);
            pageReady(i, pageCount, image.copy());
            p.bitmapDestroy(bitmap);
        }
        p.closePage(page);
    }
    p.closeDocument(document);
    return true;
}
