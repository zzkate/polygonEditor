#include "ImageViewer.h"

#include <QPainter>
#include <QFileDialog>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QMenu>
#include <QVBoxLayout>

ImageViewer::ImageViewer(QWidget *parent)
    : QWidget(parent)
    , m_loadButton(nullptr)
    , m_selectedPolygon(-1)
    , m_dragVertex(-1)
    , m_currentPolygon(-1)
{
    setWindowTitle(QStringLiteral("Qt5 Image + Polygons"));

    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(5, 5, 5, 5);
    layout->setSpacing(5);

    m_loadButton = new QPushButton(QStringLiteral("Загрузить изображение"), this);
    m_loadButton->setDefault(false);
    m_loadButton->setAutoDefault(false);
    m_loadButton->setFocusPolicy(Qt::NoFocus);
    connect(m_loadButton, &QPushButton::clicked, this, &ImageViewer::loadFile);
    layout->addWidget(m_loadButton);

    layout->addStretch();
    setLayout(layout);
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
        //p.setBrush(QBrush(Qt::green, Qt::SolidPattern));
        p.setBrush(QColor(0, 0, 255, 40));
        p.drawPolygon(poly);

        // Вершины
        p.setPen(Qt::black);
        for (int j = 0; j < poly.size(); ++j) {
            auto pt = poly[j];
            bool isSelected = m_selectedPolygon == i && m_dragVertex == j;
            p.fillRect(pt.x() - 4, pt.y() - 4, 8, 8, isSelected ? Qt::red : Qt::yellow);
            p.drawRect(pt.x() - 4, pt.y() - 4, 8, 8);
        }
    }

    // {
    //     p.setRenderHint(QPainter::Antialiasing); // Smooth edges

    //     // 1. Define styling
    //     p.setPen(QPen(Qt::black, 2));            // Border color & thickness
    //     p.setBrush(QBrush(Qt::green, Qt::SolidPattern)); // Fill color & pattern

    //     // 2. Define the polygon points
    //     QPolygon polygon;
    //     polygon << QPoint(50, 50)
    //             << QPoint(150, 20)
    //             << QPoint(250, 80)
    //             << QPoint(180, 180)
    //             << QPoint(80, 150);

    //     // 3. Draw the polygon
    //     p.drawPolygon(polygon);
    // }

//     // Текущий рисуемый полигон
//     if (m_currentPolygon >= 0 && m_currentPolygon < m_polygons.size()) {
//         const QPolygon &poly = m_polygons[m_currentPolygon];
//         if (!poly.isEmpty()) {
//             p.setPen(QPen(Qt::darkGreen, 2, Qt::DashLine));
//             // QColor color(255, 0, 0); // Red
//             // color.setAlpha(128);
//             p.setBrush(Qt::NoBrush);
//             p.drawPolyline(poly);
//         }
//     }
 }

std::pair<int, int> ImageViewer::hitTestVertex(const QPoint &pos) const
{
    const int hitRadius = 6;
    for (int i = 0; i < m_polygons.size(); ++i) {
        const QPolygon &poly = m_polygons[i];
        for (int v = 0; v < poly.size(); ++v) {
            QPoint pt = poly[v];
            if (QLineF(pt, pos).length() <= hitRadius) {
                return std::make_pair(i, v);
            }
        }
    }
    //qDebug() << "no vertex under cursor";
    return std::make_pair(-1, -1);
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
        auto v = hitTestVertex(pos);
        if (v.first != -1 && v.second != -1) {
            // Начинаем перетаскивание вершины
            m_selectedPolygon = v.first;
            m_dragVertex = v.second;
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
        if (m_currentPolygon == -1){
            m_currentPolygon = m_polygons.size();
            m_polygons.append(QPolygon());
        }
        m_selectedPolygon = -1;
        m_dragVertex = -1;
        m_polygons.last().append(pos);
        update();
    } else if (event->button() == Qt::RightButton) {
        closePolygon();
    }
}

void ImageViewer::closePolygon(){
    if (m_currentPolygon < 0 && m_currentPolygon > m_polygons.size())
        return;

    qDebug() << "end of poly";

    // Завершить текущий полигон
    // Замыкаем полигон, если нужно (QPolygon сам по себе замкнут при отрисовке)
    m_selectedPolygon = m_currentPolygon;
    m_currentPolygon = -1;
    update();
}

void ImageViewer::mouseMoveEvent(QMouseEvent *event)
{
    if (m_image.isNull())
        return;

    QPoint pos = event->pos();

    if (m_dragVertex != -1 && m_selectedPolygon != -1) {
        //qDebug() << "start move vertex!";
        // Перемещение вершины выбранного полигона
        QPolygon &poly = m_polygons[m_selectedPolygon];
        if (m_dragVertex < poly.size()) {
            QPoint delta = pos - m_lastPos;
            poly[m_dragVertex] += delta;
            m_lastPos = pos;
            update();
        }
    } else if (m_selectedPolygon != -1 && event->buttons() & Qt::LeftButton) {
        qDebug() << "move selected poly";
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
        closePolygon();
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