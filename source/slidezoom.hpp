#ifndef SLIDEZOOM_HPP
#define SLIDEZOOM_HPP

#include <QWidget>
#include <QImage>
#include <functional>

// Frame coordinates are stored relative to the original, auto-oriented image.
class SlideZoom : public QWidget
{
public:
    SlideZoom(const QImage &image, const QSize &outputSize, QWidget *parent = nullptr);
    std::function<void()> regionChanged;
    void setImage(const QImage &picture, const QSize &outputSize);
    QRectF selectedRegion() const;
    void setSelectedRegion(const QRectF &region);
    void reset();
    static QRect cropRect(const QSize &imageSize, const QRectF &region);
protected:
    void paintEvent(QPaintEvent *) override;
    void mousePressEvent(QMouseEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;
    void mouseReleaseEvent(QMouseEvent *) override;
    void wheelEvent(QWheelEvent *) override;
private:
    QRectF imageRect() const;
    QPointF imagePoint(const QPointF &point) const;
    QRectF anchoredFrame(const QPointF &corner, qreal width) const;
    QRectF fittedFrame(const QPointF &center, qreal width) const;
    QImage image;
    qreal ratio;
    QRectF frame;
    QRectF originalFrame;
    QPointF anchor;
    enum Mode { Idle, Select, Move, Resize } mode = Idle;
};
#endif
