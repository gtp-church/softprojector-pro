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

#ifndef DECKLINKOUTPUT_HPP
#define DECKLINKOUTPUT_HPP

#include <QByteArray>
#include <QImage>
#include <QSize>
#include <QStringList>

class DeckLinkOutput
{
    // Sends pictures to the output of a Blackmagic DeckLink card (Windows, Desktop Video driver).
    // A picture stays on the output until the next one is sent.
public:
    DeckLinkOutput();
    ~DeckLinkOutput();

    static QStringList devices();
    static QStringList formats();          // Video formats offered in Settings
    static QSize formatSize(int format);

    bool start(const QString &device, int format, QString *error);
    void stop();
    bool isRunning() const;
    QSize frameSize() const;
    void showFrame(const QImage &frame);

    // 8-bit 4:2:2 YUV ("2vuy", BT.709 video levels) as DeckLink cards take it
    static QByteArray toUyvy(const QImage &frame);

private:
    class Private;
    Private *d;
};

#endif // DECKLINKOUTPUT_HPP
