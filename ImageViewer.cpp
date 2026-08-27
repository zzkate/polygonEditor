#include "ImageViewer.h"

#include <QFileDialog>
#include <QImage>
#include <QKeyEvent>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QPixmap>
#include <QPoint>
#include <QPolygon>
#include <QPushButton>
#include <QVBoxLayout>
#include <QVector>

constexpr auto ptWidth = 4, ptHeight = 4;
constexpr auto contentMargin = 5;
constexpr auto space = 5;

ImageViewer::ImageViewer(QWidget* parent)
    : QWidget(parent),
      m_loadButton(nullptr),
      m_selectedPolygon(-1),
      m_dragVertex(-1),
      m_currentPolygon(-1) {
  setWindowTitle(QStringLiteral("Qt5 Image + Polygons"));

  auto layout = new QVBoxLayout(this);
  layout->setContentsMargins(contentMargin, contentMargin, contentMargin,
                             contentMargin);
  layout->setSpacing(space);

  m_loadButton = new QPushButton(QStringLiteral("Загрузить изображение"), this);
  m_loadButton->setDefault(false);
  m_loadButton->setAutoDefault(false);
  m_loadButton->setFocusPolicy(Qt::NoFocus);
  connect(m_loadButton, &QPushButton::clicked, this, &ImageViewer::loadFile);
  layout->addWidget(m_loadButton);

  layout->addStretch();
  setLayout(layout);
  setMaximumSize(1200, 800);
  setMinimumSize(1200, 800);
  setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
}

void ImageViewer::loadFile() {
  QString path = QFileDialog::getOpenFileName(
      this, QStringLiteral("Выберите изображение"), QString(),
      QStringLiteral("Images (*.png *.xpm *.jpg *.bmp *.gif)"));
  if (!path.isEmpty()) {
    m_image = QImage(path);
    reset();
    update();
  }
}

void ImageViewer::reset() {
  m_polygons.clear();
  m_selectedPolygon = -1;
  m_currentPolygon = -1;
  m_dragVertex = -1;
}

void ImageViewer::paintEvent(QPaintEvent*) {
  QPainter p(this);
  p.setRenderHint(QPainter::Antialiasing, true);
  p.fillRect(rect(), Qt::white);

  if (!drawImage(p))
    return;

  drawCurrentPolygon();
  drawPolygons(p);
}

void ImageViewer::drawPolygons(QPainter& p) {
  for (int i = 0; i < m_polygons.size(); ++i) {
    auto& poly = m_polygons[i];
    if (poly.p.isEmpty())
        continue;

    // draw holes on original image
    p.setBrush(Qt::white);
    p.drawPolygon(poly.p);

    // draw moved poly with image puzzle from original
    p.setPen(i == m_selectedPolygon ? QPen(Qt::red, 3) : QPen(Qt::blue, 2));
    assert(poly.pix.get() != nullptr);
    QBrush brush;
    brush.setTexture(*poly.pix.get());
    QTransform transform = QTransform::fromTranslate(poly.moveV.x(), poly.moveV.y());
    brush.setTransform(transform);
    p.setBrush(brush);
    p.drawPolygon(poly.p.translated(poly.moveV));

    // draw vertexes on original poly
    p.setPen(Qt::black);
    for (int j = 0; j < poly.p.size(); ++j) {
      const auto& pt = poly.p[j];
      bool isSelected = m_selectedPolygon == i && m_dragVertex == j;
      const auto r = QRect(pt.x() - ptWidth, pt.y() - ptHeight, 2 * ptWidth, 2 * ptHeight);
      p.fillRect(r, isSelected ? Qt::red : Qt::yellow);
      p.drawRect(r);
    }
  }
}

bool ImageViewer::drawImage(QPainter& p) {
  if (m_image.isNull()) return false;
  const auto& r = rect();
  // Центрируем изображение, сохраняя пропорции
  const auto scaled = m_image.size().scaled(r.size(), Qt::KeepAspectRatio);
  const QRect imgRect(QPoint((r.width() - scaled.width()) / 2,
                             (r.height() - scaled.height()) / 2),
                      scaled);
  p.drawImage(imgRect, m_image);
  return true;
}

void ImageViewer::drawCurrentPolygon() {
  if (!isCurrentPolygonValid())
    return;

  auto& poly = m_polygons[m_currentPolygon];
  if (poly.p.isEmpty())
    return;

  const auto& r = rect();
  const auto scaled = m_image.size().scaled(r.size(), Qt::KeepAspectRatio);
  const QRect imgRect(QPoint((r.width() - scaled.width()) / 2,
                             (r.height() - scaled.height()) / 2),
                      scaled);
  QImage scaledImage = m_image.scaled(imgRect.size(), Qt::IgnoreAspectRatio,
                                      Qt::SmoothTransformation);

  poly.pix = std::make_shared<QPixmap>(rect().size());
  QPainter p(poly.pix.get());
  p.setPen(QPen(Qt::darkGreen, 2, Qt::DashLine));
  QBrush brush(scaledImage);
  QTransform transform = QTransform::fromTranslate(
      (r.width() - scaled.width()) / 2, (r.height() - scaled.height()) / 2);
  brush.setTransform(transform);
  p.setBrush(brush);
  p.drawPolygon(poly.p);
}

std::pair<int, int> ImageViewer::hitTestVertex(const QPoint& pos) const {
  const int hitRadius = 6;
  for (int i = 0; i < m_polygons.size(); ++i) {
    auto& poly = m_polygons[i];
    for (int v = 0; v < poly.p.size(); ++v) {
      QPoint pt = poly.p[v];
      if (QLineF(pt, pos).length() <= hitRadius) {
        return std::make_pair(i, v);
      }
    }
  }
  return std::make_pair(-1, -1);
}

int ImageViewer::hitTestPolygon(const QPoint& pos) const {
  for (int i = 0; i < m_polygons.size(); ++i) {
    const auto& poly = m_polygons[i].p.translated(m_polygons[i].moveV);
    if (poly.isEmpty())
        continue;
    if (poly.containsPoint(pos, Qt::OddEvenFill))
        return i;
  }
  return -1;
}

bool ImageViewer::isVertexSelected(const QPoint& pos) {
  const auto& v = hitTestVertex(pos);
  if (v.first == -1 || v.second == -1)
    return false;
  m_selectedPolygon = v.first;
  m_dragVertex = v.second;
  m_lastPos = pos;
  return true;
}

bool ImageViewer::isPolygonSelected(const QPoint& pos) {
  const auto& polyIdx = hitTestPolygon(pos);
  if (polyIdx == -1)
    return false;
  m_selectedPolygon = polyIdx;
  m_dragVertex = -1;
  m_lastPos = pos;
  return true;
}

void ImageViewer::mousePressEvent(QMouseEvent* event) {
  if (m_image.isNull()) return;

  const auto& pos = event->pos();
  if (event->button() == Qt::LeftButton) {
    if (!isVertexSelected(pos))
      if (!isPolygonSelected(pos))
        processCurrentPolygon(pos);
    update();

  } else if (event->button() == Qt::RightButton) {
    closePolygon();
  }
}

void ImageViewer::processCurrentPolygon(const QPoint& pos) {
  if (m_currentPolygon == -1 || m_polygons.size() == 0) {
    m_currentPolygon = m_polygons.size();
    m_polygons.append(Poly());
  }
  m_selectedPolygon = -1;
  m_dragVertex = -1;
  m_polygons.last().p.append(pos);
}

bool ImageViewer::isCurrentPolygonValid() const {
  return (m_currentPolygon >= 0) && (m_currentPolygon < m_polygons.size());
}

void ImageViewer::closePolygon() {
  if (!isCurrentPolygonValid())
    return;

  m_selectedPolygon = m_currentPolygon;
  m_currentPolygon = -1;
  update();
}

void ImageViewer::mouseMoveEvent(QMouseEvent* event) {
  if (m_image.isNull())
    return;

  if (event->buttons() & Qt::LeftButton) {
    const auto& pos = event->pos();
    moveVertex(pos);
    movePolygon(pos);
    update();
  }
}

void ImageViewer::moveVertex(const QPoint& pos) {
  if (m_dragVertex == -1 || !isSelectedPolygonValid())
    return;

  auto& poly = m_polygons[m_selectedPolygon];
  if (m_dragVertex < poly.p.size()) {
    QPoint delta = pos - m_lastPos;
    poly.p[m_dragVertex] += delta;
    m_lastPos = pos;
  }
}

void ImageViewer::movePolygon(const QPoint& pos) {
  if (!isSelectedPolygonValid())
    return;

  QPoint delta = pos - m_lastPos;
  auto& poly = m_polygons[m_selectedPolygon];
  poly.moveV += delta;
  m_lastPos = pos;
}

void ImageViewer::mouseReleaseEvent(QMouseEvent*) {
  if (m_dragVertex != -1)
    m_dragVertex = -1;
}

void ImageViewer::keyPressEvent(QKeyEvent* event) {
  if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
    closePolygon();
  } else if (event->key() == Qt::Key_Delete) {
    dropPolygon();
  }
  QWidget::keyPressEvent(event);
}

bool ImageViewer::isSelectedPolygonValid() const {
  return (m_selectedPolygon >= 0) && (m_selectedPolygon < m_polygons.size());
}

void ImageViewer::dropPolygon() {
  if (!isSelectedPolygonValid())
    return;
  m_polygons.remove(m_selectedPolygon);
  m_selectedPolygon = -1;
  m_currentPolygon = -1;
  update();
}