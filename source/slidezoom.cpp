#include "slidezoom.hpp"
#include <QMouseEvent>
#include <QWheelEvent>
#include <QPainter>
#include <QPainterPath>
#include <cmath>

SlideZoom::SlideZoom(const QImage &picture, const QSize &outputSize, QWidget *parent)
    : QWidget(parent), image(picture), ratio(outputSize.height() > 0
        ? qreal(outputSize.width()) / outputSize.height() : 16.0 / 9.0)
{
    if(ratio <= 0) ratio = 16.0 / 9.0;
    setMinimumSize(320, 240);
    setMouseTracking(true);
}

void SlideZoom::setImage(const QImage &picture, const QSize &outputSize)
{
    image = picture;
    ratio = outputSize.width() > 0 && outputSize.height() > 0
            ? qreal(outputSize.width()) / outputSize.height() : 16.0 / 9.0;
    frame = QRectF();
    mode = Idle;
    update();
}

QRectF SlideZoom::anchoredFrame(const QPointF &corner, qreal requestedWidth) const
{
    qreal maximum = qMin(image.width() - corner.x(), (image.height() - corner.y()) * ratio);
    qreal w = qBound(qMin(16.0, maximum), requestedWidth, maximum);
    return QRectF(corner, QSizeF(w, w / ratio));
}

QRectF SlideZoom::imageRect() const
{
    QSizeF size = image.size();
    size.scale(contentsRect().size(), Qt::KeepAspectRatio);
    return QRectF(QPointF((width() - size.width()) / 2, (height() - size.height()) / 2), size);
}

QPointF SlideZoom::imagePoint(const QPointF &point) const
{
    QRectF box = imageRect();
    return QPointF(qBound(0.0, (point.x() - box.x()) / box.width(), 1.0) * image.width(),
                   qBound(0.0, (point.y() - box.y()) / box.height(), 1.0) * image.height());
}

QRectF SlideZoom::fittedFrame(const QPointF &center, qreal requestedWidth) const
{
    qreal maximum = qMin(qreal(image.width()), image.height() * ratio);
    qreal w = qBound(qMin(16.0, maximum), requestedWidth, maximum);
    qreal h = w / ratio;
    QPointF topLeft(qBound(0.0, center.x() - w / 2, image.width() - w),
                    qBound(0.0, center.y() - h / 2, image.height() - h));
    return QRectF(topLeft, QSizeF(w, h));
}

QRectF SlideZoom::selectedRegion() const
{
    if(frame.isEmpty() || image.isNull()) return QRectF();
    return QRectF(frame.x() / image.width(), frame.y() / image.height(),
                  frame.width() / image.width(), frame.height() / image.height());
}

void SlideZoom::setSelectedRegion(const QRectF &region)
{
    if(region.isEmpty() || image.isNull()) { frame = QRectF(); update(); return; }
    frame = fittedFrame(QPointF(region.center().x() * image.width(), region.center().y() * image.height()),
                        region.width() * image.width());
    update();
}

void SlideZoom::reset() { frame = QRectF(); mode = Idle; update(); if(regionChanged) regionChanged(); }

QRect SlideZoom::cropRect(const QSize &size, const QRectF &region)
{
    if(region.isEmpty()) return QRect(QPoint(), size);
    QRect result(qRound(region.x() * size.width()), qRound(region.y() * size.height()),
                 qMax(1, qRound(region.width() * size.width())), qMax(1, qRound(region.height() * size.height())));
    return result.intersected(QRect(QPoint(), size));
}

void SlideZoom::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.fillRect(rect(), Qt::black);
    if(image.isNull()) return;
    QRectF box = imageRect();
    p.setRenderHint(QPainter::SmoothPixmapTransform);
    p.drawImage(box, image);
    if(frame.isEmpty()) return;
    qreal scale = box.width() / image.width();
    QRectF visible(box.topLeft() + frame.topLeft() * scale, frame.size() * scale);
    QPainterPath shade;
    shade.addRect(box);
    shade.addRect(visible);
    p.fillPath(shade, QColor(0, 0, 0, 100));
    p.setPen(QPen(QColor(255, 200, 40), 2));
    p.drawRect(visible);
    p.fillRect(QRectF(visible.bottomRight() - QPointF(5, 5), QSizeF(10, 10)), QColor(255, 200, 40));
}

void SlideZoom::mousePressEvent(QMouseEvent *event)
{
    if(event->button() != Qt::LeftButton || image.isNull() || !imageRect().contains(event->localPos())) return;
    anchor = imagePoint(event->localPos());
    originalFrame = frame;
    qreal handle = 12 * image.width() / imageRect().width();
    if(!frame.isEmpty() && QLineF(anchor, frame.bottomRight()).length() <= handle)
        mode = Resize;
    else if(frame.contains(anchor))
        mode = Move;
    else
        mode = Select;
}

void SlideZoom::mouseMoveEvent(QMouseEvent *event)
{
    if(mode == Idle || image.isNull()) return;
    QPointF point = imagePoint(event->localPos());
    if(mode == Move)
        frame = fittedFrame(originalFrame.center() + point - anchor, originalFrame.width());
    else if(mode == Resize)
    {
        QPointF corner = originalFrame.topLeft();
        qreal w = qMax(point.x() - corner.x(), (point.y() - corner.y()) * ratio);
        frame = anchoredFrame(corner, w);
    }
    else
    {
        frame = anchoredFrame(anchor, qMax(point.x() - anchor.x(), (point.y() - anchor.y()) * ratio));
    }
    update();
}

void SlideZoom::mouseReleaseEvent(QMouseEvent *event)
{
    if(event->button() == Qt::LeftButton) { mouseMoveEvent(event); mode = Idle; if(regionChanged) regionChanged(); }
}

void SlideZoom::wheelEvent(QWheelEvent *event)
{
    if(frame.isEmpty()) { event->ignore(); return; }
    frame = anchoredFrame(frame.topLeft(), frame.width() * std::pow(1.0015, -event->angleDelta().y()));
    update();
    if(regionChanged) regionChanged();
    event->accept();
}
