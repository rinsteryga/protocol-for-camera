/**
 * @file QmlImageItem.hpp
 * @brief Пользовательский элемент QML для аппаратного рендеринга объектов QImage.
 */
#pragma once

#include <QQuickPaintedItem>
#include <QImage>

/**
 * @brief Класс для интеграции C++ QImage в графический стек Qt Quick.
 * Наследуется от QQuickPaintedItem, обеспечивает прямую отрисовку изображения
 * в графе сцены (Scene Graph) без необходимости использования QQuickImageProvider.
 */
class QmlImageItem : public QQuickPaintedItem {
    Q_OBJECT

    /**
     * @brief Свойство изображения, доступное из QML для биндинга.
     */
    Q_PROPERTY(QImage image READ image WRITE setImage NOTIFY imageChanged)

public:
    /**
     * @brief Конструктор элемента.
     * @param parent Указатель на родительский визуальный элемент.
     */
    explicit QmlImageItem(QQuickItem *parent = nullptr) : QQuickPaintedItem(parent) {}

    /**
     * @brief Геттер текущего кадра.
     * @return Ссылка на объект QImage.
     */
    QImage image() const { return m_image; }

    /**
     * @brief Сеттер кадра.
     * Обновляет внутреннее изображение, вызывает сигнал изменения и инициирует асинхронную перерисовку.
     * @param img Новый видеокадр.
     */
    void setImage(const QImage &img) {
        m_image = img;
        emit imageChanged();
        update();
    }

    /**
     * @brief Переопределенный виртуальный метод отрисовки компонента.
     * @param painter Указатель на экземпляр QPainter для выполнения графических примитивов.
     */
    void paint(QPainter *painter) override;

signals:
    /**
     * @brief Сигнал, испускаемый при поступлении нового кадра из C++ логики.
     */
    void imageChanged();

private:
    QImage m_image; ///< Внутреннее хранилище текущего отображаемого кадра.
};
