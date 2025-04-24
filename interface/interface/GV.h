#ifndef GV_H
#define GV_H

#include <QGraphicsView>
#include <QMouseEvent>

class GV : public QGraphicsView {
    Q_OBJECT

public:
    GV(QWidget* parent = nullptr) : QGraphicsView(parent) {
        setMouseTracking(true);
    }

signals:
    void mouseMoved(const QPointF& pos);
    void mousePressed(const QPointF& pos);

protected:
    void mouseMoveEvent(QMouseEvent* event) override {
        QPointF scenePos = mapToScene(event->pos());
        QPointF normalizedPos = normalizeCoordinates(scenePos);
        emit mouseMoved(normalizedPos);
        QGraphicsView::mouseMoveEvent(event);
    }
    void mousePressEvent(QMouseEvent* event) override {
        QPointF scenePos = mapToScene(event->pos());
        QPointF normalizedPos = normalizeCoordinates(scenePos);
        emit mousePressed(normalizedPos); 
        QGraphicsView::mousePressEvent(event);
    }

private:
    QPointF normalizeCoordinates(const QPointF& pos) const {
        if (!scene()) {
            return QPointF();
        }
        QRectF sceneRect = scene()->sceneRect();
        double normalizedX = (pos.x() - sceneRect.left()) / sceneRect.width();
        double normalizedY = (pos.y() - sceneRect.top()) / sceneRect.height();
        return QPointF(normalizedX, normalizedY);
    }
};

#endif // GV_H
