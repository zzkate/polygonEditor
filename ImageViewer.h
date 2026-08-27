#ifndef IMAGEVIEWER_H
#define IMAGEVIEWER_H

#include <QWidget>
#include <QImage>
#include <QPolygon>
#include <QVector>
#include <QPoint>

class ImageViewer : public QWidget
{
    Q_OBJECT
public:
    explicit ImageViewer(QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    void loadFile();
    int hitTestVertex(const QPoint &pos) const;
    int hitTestPolygon(const QPoint &pos) const;

    QImage m_image;
    QVector<QPolygon> m_polygons;
    int m_selectedPolygon;       // индекс выбранного полигона, -1 если нет
    int m_dragVertex;            // индекс перетаскиваемой вершины, -1 если нет
    int m_currentPolygon;        // индекс рисуемого полигона, -1 если не рисуем
    QPoint m_lastPos;
};

#endif // IMAGEVIEWER_H