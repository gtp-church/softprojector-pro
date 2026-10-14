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

#include "decklinkoutput.hpp"
#include <QCoreApplication>

#ifdef HAVE_DECKLINK
#include <windows.h>
#include "DeckLinkAPI.h" // Made from 3rdparty/decklink/DeckLinkAPI.idl by widl when building
#endif

namespace
{
struct Format
{
    const char *name;
    unsigned int mode; // BMDDisplayMode
    int width;
    int height;
};

// The first one is the default
const Format outputFormats[] = {
    {"1080p 25", 0x48703235, 1920, 1080},
    {"1080p 29.97", 0x48703239, 1920, 1080},
    {"1080p 30", 0x48703330, 1920, 1080},
    {"1080p 24", 0x32347073, 1920, 1080},
    {"1080i 50", 0x48693530, 1920, 1080},
    {"1080i 59.94", 0x48693539, 1920, 1080},
    {"720p 50", 0x68703530, 1280, 720},
    {"720p 59.94", 0x68703539, 1280, 720},
};
const int formatCount = sizeof(outputFormats) / sizeof(outputFormats[0]);
const unsigned int pixelFormat8BitYUV = 0x32767579; // bmdFormat8BitYUV, '2vuy'
}

class DeckLinkOutput::Private
{
public:
#ifdef HAVE_DECKLINK
    IDeckLinkOutput *output = nullptr;
#endif
    QSize size;
};

DeckLinkOutput::DeckLinkOutput() : d(new Private)
{
}

DeckLinkOutput::~DeckLinkOutput()
{
    stop();
    delete d;
}

QStringList DeckLinkOutput::formats()
{
    QStringList names;
    for(int i(0); i < formatCount; ++i)
        names.append(outputFormats[i].name);
    return names;
}

QSize DeckLinkOutput::formatSize(int format)
{
    if(format < 0 || format >= formatCount)
        format = 0;
    return QSize(outputFormats[format].width, outputFormats[format].height);
}

QByteArray DeckLinkOutput::toUyvy(const QImage &frame)
{
    // Two pixels share one colour sample: Cb Y0 Cr Y1. Coefficients of BT.709, video levels
    // (Y 16-235, Cb/Cr 16-240), in 1/65536 steps.
    QImage image = frame.convertToFormat(QImage::Format_RGB32);
    int width = image.width() & ~1;
    QByteArray uyvy(width * image.height() * 2, Qt::Uninitialized);
    uchar *out = reinterpret_cast<uchar *>(uyvy.data());
    for(int y(0); y < image.height(); ++y)
    {
        const QRgb *line = reinterpret_cast<const QRgb *>(image.constScanLine(y));
        for(int x(0); x < width; x += 2)
        {
            int r0 = qRed(line[x]), g0 = qGreen(line[x]), b0 = qBlue(line[x]);
            int r1 = qRed(line[x + 1]), g1 = qGreen(line[x + 1]), b1 = qBlue(line[x + 1]);
            int y0 = 16 + ((11966 * r0 + 40254 * g0 + 4064 * b0 + 32768) >> 16);
            int y1 = 16 + ((11966 * r1 + 40254 * g1 + 4064 * b1 + 32768) >> 16);
            int r = r0 + r1, g = g0 + g1, b = b0 + b1; // Sum of the two pixels
            int cb = 128 + ((-6596 * r - 22189 * g + 28785 * b + 65536) >> 17);
            int cr = 128 + ((28785 * r - 26145 * g - 2640 * b + 65536) >> 17);
            *out++ = uchar(qBound(16, cb, 240));
            *out++ = uchar(qBound(16, y0, 235));
            *out++ = uchar(qBound(16, cr, 240));
            *out++ = uchar(qBound(16, y1, 235));
        }
    }
    return uyvy;
}

#ifdef HAVE_DECKLINK

static IDeckLinkIterator *createIterator()
{
    IDeckLinkIterator *iterator = nullptr;
    if(FAILED(CoCreateInstance(CLSID_CDeckLinkIterator, nullptr, CLSCTX_ALL, IID_IDeckLinkIterator,
                               reinterpret_cast<void **>(&iterator))))
        return nullptr;
    return iterator;
}

static QString displayName(IDeckLink *deckLink)
{
    BSTR name = nullptr;
    QString result;
    if(deckLink->GetDisplayName(&name) == S_OK && name)
    {
        result = QString::fromWCharArray(name);
        SysFreeString(name);
    }
    return result;
}

QStringList DeckLinkOutput::devices()
{
    // Only cards that can send video out
    QStringList names;
    IDeckLinkIterator *iterator = createIterator();
    if(!iterator)
        return names;
    IDeckLink *deckLink = nullptr;
    while(iterator->Next(&deckLink) == S_OK)
    {
        IDeckLinkOutput *output = nullptr;
        if(deckLink->QueryInterface(IID_IDeckLinkOutput, reinterpret_cast<void **>(&output)) == S_OK)
        {
            names.append(displayName(deckLink));
            output->Release();
        }
        deckLink->Release();
    }
    iterator->Release();
    return names;
}

bool DeckLinkOutput::start(const QString &device, int format, QString *error)
{
    stop();
    if(format < 0 || format >= formatCount)
        format = 0;

    IDeckLinkIterator *iterator = createIterator();
    if(!iterator)
    {
        *error = QCoreApplication::translate("DeckLinkOutput",
                                             "The Blackmagic Desktop Video driver is not installed.");
        return false;
    }
    IDeckLink *deckLink = nullptr;
    while(iterator->Next(&deckLink) == S_OK)
    {
        if(displayName(deckLink) == device)
            break;
        deckLink->Release();
        deckLink = nullptr;
    }
    iterator->Release();
    if(!deckLink)
    {
        *error = QCoreApplication::translate("DeckLinkOutput", "The card %1 was not found.").arg(device);
        return false;
    }

    IDeckLinkOutput *output = nullptr;
    HRESULT result = deckLink->QueryInterface(IID_IDeckLinkOutput, reinterpret_cast<void **>(&output));
    deckLink->Release();
    if(result != S_OK)
    {
        *error = QCoreApplication::translate("DeckLinkOutput", "The card %1 cannot send video out.").arg(device);
        return false;
    }
    if(output->EnableVideoOutput((BMDDisplayMode)outputFormats[format].mode, bmdVideoOutputFlagDefault) != S_OK)
    {
        output->Release();
        *error = QCoreApplication::translate("DeckLinkOutput",
                                             "The card %1 cannot send %2, or another program is using it.")
                .arg(device).arg(outputFormats[format].name);
        return false;
    }
    d->output = output;
    d->size = formatSize(format);
    return true;
}

void DeckLinkOutput::stop()
{
    if(!d->output)
        return;
    d->output->DisableVideoOutput();
    d->output->Release();
    d->output = nullptr;
}

bool DeckLinkOutput::isRunning() const
{
    return d->output != nullptr;
}

void DeckLinkOutput::showFrame(const QImage &frame)
{
    if(!d->output)
        return;
    QImage image = frame;
    if(image.size() != d->size)
        image = image.scaled(d->size, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    QByteArray uyvy = toUyvy(image);

    IDeckLinkMutableVideoFrame *videoFrame = nullptr;
    if(d->output->CreateVideoFrame(d->size.width(), d->size.height(), d->size.width() * 2,
                                   (BMDPixelFormat)pixelFormat8BitYUV, bmdFrameFlagDefault, &videoFrame) != S_OK)
        return;
    void *bytes = nullptr;
    if(videoFrame->GetBytes(&bytes) == S_OK && bytes)
    {
        memcpy(bytes, uyvy.constData(), size_t(uyvy.size()));
        d->output->DisplayVideoFrameSync(videoFrame);
    }
    videoFrame->Release();
}

#else // No DeckLink support in this build (other systems)

QStringList DeckLinkOutput::devices()
{
    return QStringList();
}

bool DeckLinkOutput::start(const QString &device, int format, QString *error)
{
    Q_UNUSED(device);
    Q_UNUSED(format);
    *error = QCoreApplication::translate("DeckLinkOutput", "Blackmagic cards are supported only on Windows.");
    return false;
}

void DeckLinkOutput::stop()
{
}

bool DeckLinkOutput::isRunning() const
{
    return false;
}

void DeckLinkOutput::showFrame(const QImage &frame)
{
    Q_UNUSED(frame);
}

#endif

QSize DeckLinkOutput::frameSize() const
{
    return d->size;
}
