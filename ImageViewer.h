#ifndef IMAGEVIEWER_H
#define IMAGEVIEWER_H

#include <QWidget>
#include <QImage>
#include <QPolygon>
#include <QVector>
#include <QPoint>
#include <QPushButton>

struct Poly{
    QPolygon p;
    QPoint moveV;
    std::shared_ptr<QPixmap> pix = nullptr;
    QImage img;
};

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
    void reset();
    void loadFile();
    std::pair<int, int> hitTestVertex(const QPoint &pos) const;
    int hitTestPolygon(const QPoint &pos) const;
    void closePolygon();

    void drawCurrentPolygon();
    bool drawImage(QPainter& p);
    void drawPolygons(QPainter& p);

    void moveVertex(const QPoint& pos);
    void movePolygon(const QPoint& pos);

    void dropPolygon();

    bool isCurrentPolygonCorrect() const;
    bool isSelectedPolygonCorrect() const;

    QPushButton* m_loadButton;
    QImage m_image;
    //QVector<QPolygon> m_polygons;
    QVector<Poly> m_polygons;
    int m_selectedPolygon;       // индекс выбранного полигона, -1 если нет
    int m_dragVertex;            // индекс перетаскиваемой вершины, -1 если нет
    int m_currentPolygon;        // индекс рисуемого полигона, -1 если не рисуем
    QPoint m_lastPos;
};

#endif // IMAGEVIEWER_H