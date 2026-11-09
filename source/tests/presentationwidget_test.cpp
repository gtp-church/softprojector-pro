#include <QtTest>
#include <QTemporaryDir>
#include <QMimeData>
#include <QDragEnterEvent>
#include <QDropEvent>
#include "presentationwidget.hpp"
#include "slidezoom.hpp"

class PresentationWidgetTest : public QObject
{
    Q_OBJECT
private slots:
    void zoomGeometry()
    {
        QImage image(400, 200, QImage::Format_RGB32);
        image.fill(Qt::white);
        SlideZoom editor(image, QSize(1920, 1080));
        editor.resize(400, 300);
        editor.show();
        int changes = 0;
        editor.regionChanged = [&]() { ++changes; };
        QTest::mouseClick(&editor, Qt::LeftButton, Qt::NoModifier, QPoint(50, 75));
        QVERIFY(editor.selectedRegion().isEmpty());
        QCOMPARE(changes, 0);
        QTest::mousePress(&editor, Qt::LeftButton, Qt::NoModifier, QPoint(50, 75));
        QTest::mouseRelease(&editor, Qt::LeftButton, Qt::NoModifier,
                            QPoint(50 + QApplication::startDragDistance() - 1, 75));
        QVERIFY(editor.selectedRegion().isEmpty());
        QCOMPARE(changes, 0);
        QTest::mousePress(&editor, Qt::LeftButton, Qt::NoModifier, QPoint(50, 75));
        QTest::mouseRelease(&editor, Qt::LeftButton, Qt::NoModifier, QPoint(250, 175));
        QRectF region = editor.selectedRegion();
        QVERIFY(!region.isEmpty());
        QCOMPARE(changes, 1);
        QTest::mouseClick(&editor, Qt::LeftButton, Qt::NoModifier, QPoint(10, 60));
        QCOMPARE(editor.selectedRegion(), region);
        QCOMPARE(changes, 1);
        QTest::mouseClick(&editor, Qt::LeftButton, Qt::NoModifier, QPoint(100, 100));
        QCOMPARE(editor.selectedRegion(), region);
        QCOMPARE(changes, 1);
        QCOMPARE(region.topLeft(), QPointF(50.0 / 400, 25.0 / 200));
        QVERIFY(qAbs(region.width() * 400 / (region.height() * 200) - 16.0 / 9.0) < 0.001);
        QVERIFY(QRectF(0, 0, 1, 1).contains(region));
        QRect crop = SlideZoom::cropRect(image.size(), region);
        QVERIFY(QRect(QPoint(), image.size()).contains(crop));
        QVERIFY(crop.width() < image.width());
        QPoint corner(qRound(region.right() * 400), qRound(region.bottom() * 200 + 50));
        QTest::mousePress(&editor, Qt::LeftButton, Qt::NoModifier, corner);
        QTest::mouseRelease(&editor, Qt::LeftButton, Qt::NoModifier, QPoint(399, 249));
        QCOMPARE(editor.selectedRegion().topLeft(), region.topLeft());
        region = editor.selectedRegion();
        QWheelEvent wheel(QPointF(100, 100), QPointF(100, 100), QPoint(), QPoint(0, 120),
                          Qt::NoButton, Qt::NoModifier, Qt::NoScrollPhase, false);
        QApplication::sendEvent(&editor, &wheel);
        QCOMPARE(editor.selectedRegion().topLeft(), region.topLeft());
        QVERIFY(editor.selectedRegion().width() < region.width());
        region = editor.selectedRegion();
        // Moving beyond an edge clamps the frame without changing its size.
        QPoint center(qRound(region.center().x() * 400), qRound(region.center().y() * 200 + 50));
        QTest::mousePress(&editor, Qt::LeftButton, Qt::NoModifier, center);
        QTest::mouseRelease(&editor, Qt::LeftButton, Qt::NoModifier, QPoint(399, 249));
        QCOMPARE(editor.selectedRegion().size(), region.size());
        QVERIFY(QRectF(0, 0, 1, 1).contains(editor.selectedRegion()));
        // Restoring a crop on a portrait output keeps the output aspect ratio.
        SlideZoom portrait(image, QSize(1080, 1920));
        portrait.setSelectedRegion(region);
        QRectF rotated = portrait.selectedRegion();
        QVERIFY(qAbs(rotated.width() * 400 / (rotated.height() * 200) - 9.0 / 16.0) < 0.001);
        editor.reset();
        QVERIFY(editor.selectedRegion().isEmpty());
        QCOMPARE(SlideZoom::cropRect(image.size(), editor.selectedRegion()), QRect(QPoint(), image.size()));
    }

    void closeAndDrop()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        QStringList files;
        for(int i = 0; i < 4; ++i)
        {
            QString path = dir.filePath(QString("%1.png").arg(i));
            QImage image(64, 36, QImage::Format_RGB32);
            image.fill(QColor::fromHsv(i * 60, 255, 255));
            QVERIFY(image.save(path));
            files << path;
        }
        PresentationWidget widget;
        widget.show();
        const auto lists = widget.findChildren<QListWidget *>();
        QCOMPARE(lists.size(), 2);
        QListWidget *decks = widget.findChild<QListWidget *>("listPresentations");
        QListWidget *slides = widget.findChild<QListWidget *>("listSlides");
        QVERIFY(decks);
        QVERIFY(slides);
        auto dropFiles = [](QWidget *target, const QStringList &paths) {
            QMimeData mime;
            QList<QUrl> urls;
            for(const QString &path : paths)
                urls << QUrl::fromLocalFile(path);
            mime.setUrls(urls);
            QDragEnterEvent enter(QPoint(10, 10), Qt::CopyAction, &mime, Qt::LeftButton, Qt::NoModifier);
            QApplication::sendEvent(target, &enter);
            if(!enter.isAccepted())
                return false;
            QDropEvent drop(QPointF(10, 10), Qt::CopyAction, &mime, Qt::LeftButton, Qt::NoModifier);
            QApplication::sendEvent(target, &drop);
            return drop.isAccepted();
        };
        widget.openPresentation({files[0]});
        QTRY_COMPARE(slides->count(), 1);
        widget.openPresentation({files[1]});
        QTRY_COMPARE(slides->count(), 1);
        QCOMPARE(decks->count(), 2);
        QVERIFY(QMetaObject::invokeMethod(&widget, "removeFromLibrary"));
        QCOMPARE(decks->count(), 1);
        QTRY_COMPARE(slides->count(), 1);
        QCOMPARE(decks->currentRow(), 0);
        // Verify the remaining deck is actually loaded, not merely highlighted.
        QStringList liveSources;
        connect(&widget, &PresentationWidget::sendPresentation, [&](Presentation p, int) { liveSources = p.sources; });
        QVERIFY(QMetaObject::invokeMethod(&widget, "goLive"));
        QCOMPARE(liveSources, QStringList{files[0]});
        QComboBox *output = widget.findChild<QComboBox *>("presentationOutput");
        QVERIFY(output);
        Presentation::Output sentOutput = Presentation::MainScreen;
        int outputSends = 0;
        connect(&widget, &PresentationWidget::sendPresentation, [&](Presentation p, int) {
            sentOutput = p.output;
            ++outputSends;
        });
        QCOMPARE(output->currentData().toInt(), int(Presentation::MainScreen));
        output->setCurrentIndex(1);
        QCOMPARE(outputSends, 0);
        QVERIFY(QMetaObject::invokeMethod(&widget, "goLive"));
        QCOMPARE(sentOutput, Presentation::AdditionalScreen);
        output->setCurrentIndex(0);
        QVERIFY(QMetaObject::invokeMethod(&widget, "goLive"));
        QCOMPARE(sentOutput, Presentation::MainScreen);
        QVERIFY(dropFiles(slides->viewport(), {files[2], files[3]}));
        QTRY_COMPARE(slides->count(), 3);
        QCOMPARE(decks->count(), 1);
        QVERIFY(QMetaObject::invokeMethod(&widget, "goLive"));
        QCOMPARE(liveSources, (QStringList{files[0], files[2], files[3]}));
        QVERIFY(dropFiles(slides->viewport(), {files[2]}));
        QCOMPARE(slides->count(), 3);
        QVERIFY(dropFiles(decks->viewport(), {files[1]}));
        // Close immediately, before the worker's completion is delivered.
        QVERIFY(QMetaObject::invokeMethod(&widget, "removeFromLibrary"));
        QTRY_COMPARE(slides->count(), 3);
        QVERIFY(QMetaObject::invokeMethod(&widget, "goLive"));
        QCOMPARE(liveSources, (QStringList{files[0], files[2], files[3]}));
        QVERIFY(QMetaObject::invokeMethod(&widget, "removeFromLibrary"));
        QCOMPARE(decks->count(), 0);
        QCOMPARE(slides->count(), 0);
        widget.openPresentation({files[1]});
        QVERIFY(QMetaObject::invokeMethod(&widget, "removeFromLibrary"));
        QTest::qWait(200);
        QCOMPARE(slides->count(), 0);
        widget.openPresentation({files[0]});
        QTRY_COMPARE(slides->count(), 1);
    }
};
QTEST_MAIN(PresentationWidgetTest)
#include "presentationwidget_test.moc"
