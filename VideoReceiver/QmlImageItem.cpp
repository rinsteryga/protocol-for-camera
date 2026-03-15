#include "QmlImageItem.hpp"

#include <QPainter>

void QmlImageItem::paint(QPainter *painter) {
    if (m_image.isNull()) return;
    QImage scaled = m_image.scaled(boundingRect().size().toSize(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    int x = (boundingRect().width() - scaled.width()) / 2;
    int y = (boundingRect().height() - scaled.height()) / 2;
    painter->drawImage(x, y, scaled);
}
