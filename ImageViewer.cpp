#include "ImageViewer.h"

#include <QPainter>
#include <QFileDialog>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QMenu>

ImageViewer::ImageViewer(QWidget *parent)
    : QWidget(parent)
    , m_selectedPolygon(-1)
    , m_dragVertex(-1)
    , m_currentPolygon(-1)
{
    setWindowTitle(QStringLiteral("Qt5 Image + Polygons"));
    // Контекстное меню для загрузки изображения
    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &QWidget::customContextMenuRequested,
            this, [this](const QPoint &pos) {
        QMenu menu(this);
        menu.addAction(QStringLiteral("Загрузить изображение"), this, &ImageViewer::loadFile);
        menu.exec(mapToGlobal(pos));
    });
}

void ImageViewer::loadFile()
{
    QString path = QFileDialog::getOpenFileName(
        this,
        QStringLiteral("Выберите изображение"),
        QString(),
        QStringLiteral("Images (*.png *.xpm *.jpg *.bmp *.gif)"));
    if (!path.isEmpty()) {
        m_image = QImage(path);
        m_polygons.clear();
        m_selectedPolygon = -1;
        m_currentPolygon = -1;
        m_dragVertex = -1;
        update();
    }
}

void ImageViewer::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    // Фон
    p.fillRect(rect(), Qt::white);

    // Изображение
    if (!m_image.isNull()) {
        QRect r = rect();
        // Центрируем изображение, сохраняя пропорции
        QSize scaled = m_image.size().scaled(r.size(), Qt::KeepAspectRatio);
        QRect imgRect(QPoint((r.width() - scaled.width()) / 2,
                             (r.height() - scaled.height()) / 2),
                      scaled);
        p.drawImage(imgRect, m_image);

        // Сохраняем смещение и масштаб для корректной работы с координатами
        // Для простоты в этом примере считаем, что работаем в координатах виджета,
        // а изображение просто нарисовано внутри.
        // Если нужно точно по изображению — можно добавить transform.
    }

    // Полигоны
    for (int i = 0; i < m_polygons.size(); ++i) {
        const QPolygon &poly = m_polygons[i];
        if (poly.isEmpty())
            continue;

        p.setPen(i == m_selectedPolygon ? QPen(Qt::red, 3) : QPen(Qt::blue, 2));
        p.setBrush(QColor(0, 0, 255, 40));
        p.drawPolygon(poly);

        // Вершины
        p.setPen(Qt::black);
        for (const QPoint &pt : poly) {
            p.fillRect(pt.x() - 4, pt.y() - 4, 8, 8, Qt::yellow);
            p.drawRect(pt.x() - 4, pt.y() - 4, 8, 8);
        }
    }

    // Текущий рисуемый полигон
    if (m_currentPolygon >= 0 && m_currentPolygon < m_polygons.size()) {
        const QPolygon &poly = m_polygons[m_currentPolygon];
        if (!poly.isEmpty()) {
            p.setPen(QPen(Qt::darkGreen, 2, Qt::DashLine));
            p.setBrush(Qt::NoBrush);
            p.drawPolyline(poly);
        }
    }
}

int ImageViewer::hitTestVertex(const QPoint &pos) const
{
    const int hitRadius = 6;
    for (int i = 0; i < m_polygons.size(); ++i) {
        const QPolygon &poly = m_polygons[i];
        for (int v = 0; v < poly.size(); ++v) {
            QPoint pt = poly[v];
            if (QLineF(pt, pos).length() <= hitRadius) {
                return v; // упрощённо: возвращаем индекс вершины в первом найденном полигоне
                // для более точного нужно хранить и индекс полигона
            }
        }
    }
    return -1;
}

int ImageViewer::hitTestPolygon(const QPoint &pos) const
{
    for (int i = m_polygons.size() - 1; i >= 0; --i) {
        const QPolygon &poly = m_polygons[i];
        if (poly.isEmpty())
            continue;
        if (poly.containsPoint(pos, Qt::OddEvenFill))
            return i;
    }
    return -1;
}

void ImageViewer::mousePressEvent(QMouseEvent *event)
{
    if (m_image.isNull())
        return;

    QPoint pos = event->pos();

    if (event->button() == Qt::LeftButton) {
        // Сначала проверяем, попали ли в вершину существующего полигона
        int v = hitTestVertex(pos);
        if (v != -1) {
            // Начинаем перетаскивание вершины
            // Для простоты считаем, что вершина принадлежит первому найденному полигону
            // В реальном проекте нужно хранить (polyIndex, vertexIndex)
            m_dragVertex = v;
            m_lastPos = pos;
            return;
        }

        // Затем проверяем, попали ли внутрь полигона -> перемещение полигона
        int polyIdx = hitTestPolygon(pos);
        if (polyIdx != -1) {
            m_selectedPolygon = polyIdx;
            m_dragVertex = -1;
            m_lastPos = pos;
            update();
            return;
        }

        // Если не попали ни в вершину, ни в полигон — начинаем новый полигон
        m_currentPolygon = m_polygons.size();
        m_polygons.append(QPolygon());
        m_polygons.last().append(pos);
        m_selectedPolygon = -1;
        m_dragVertex = -1;
        update();
    } else if (event->button() == Qt::RightButton) {
        // Завершить текущий полигон
        if (m_currentPolygon >= 0 && m_currentPolygon < m_polygons.size()) {
            // Замыкаем полигон, если нужно (QPolygon сам по себе замкнут при отрисовке)
            m_currentPolygon = -1;
            update();
        }
    }
}

void ImageViewer::mouseMoveEvent(QMouseEvent *event)
{
    if (m_image.isNull())
        return;

    QPoint pos = event->pos();

    if (m_dragVertex != -1 && m_selectedPolygon != -1) {
        // Перемещение вершины выбранного полигона
        QPolygon &poly = m_polygons[m_selectedPolygon];
        if (m_dragVertex < poly.size()) {
            QPoint delta = pos - m_lastPos;
            poly[m_dragVertex] += delta;
            m_lastPos = pos;
            update();
        }
    } else if (m_selectedPolygon != -1 && event->buttons() & Qt::LeftButton) {
        // Перемещение выбранного полигона целиком
        QPoint delta = pos - m_lastPos;
        QPolygon &poly = m_polygons[m_selectedPolygon];
        for (int i = 0; i < poly.size(); ++i)
            poly[i] += delta;
        m_lastPos = pos;
        update();
    } else if (m_currentPolygon >= 0 && m_currentPolygon < m_polygons.size()) {
        // Добавление вершин в текущий полигон при движении с зажатой ЛКМ
        // В простом варианте добавляем вершину только по клику, здесь можно
        // добавлять "на лету" или просто обновлять последнюю точку.
        // Для простоты ничего не делаем в move, вершины добавляем в press.
    }
}

void ImageViewer::mouseReleaseEvent(QMouseEvent *)
{
    if (m_dragVertex != -1) {
        m_dragVertex = -1;
    }
}

void ImageViewer::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        // Завершить текущий полигон
        if (m_currentPolygon >= 0 && m_currentPolygon < m_polygons.size()) {
            m_currentPolygon = -1;
            update();
        }
    } else if (event->key() == Qt::Key_Delete) {
        // Удалить выбранный полигон
        if (m_selectedPolygon >= 0 && m_selectedPolygon < m_polygons.size()) {
            m_polygons.remove(m_selectedPolygon);
            m_selectedPolygon = -1;
            update();
        }
    }
    QWidget::keyPressEvent(event);
}