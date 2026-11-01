#ifndef TIMELINETOKENBASE_H
#define TIMELINETOKENBASE_H

#include <QColor>
#include <QQuickPaintedItem>

// Base class for the tokens drawn on the timeline.
//
// It owns everything that is common to every token: the look (color, radius),
// the pixel <-> time conversion (secondInPixel), the selection state, the
// drag / resize mouse handling and the generic paint routine.
//
// Concrete tokens only have to describe how their selection state is backed
// (selected / setSelected) and, optionally, what label to show in the center.
class TimeLineTokenBase : public QQuickPaintedItem
{
    Q_OBJECT
    QML_UNAVAILABLE
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged)
    Q_PROPERTY(qreal radius READ radius WRITE setRadius NOTIFY radiusChanged)
    Q_PROPERTY(qreal secondInPixel READ secondInPixel WRITE setSecondInPixel NOTIFY secondInPixelChanged FINAL)
    Q_PROPERTY(qreal duration READ duration NOTIFY widthChanged FINAL)

public:
    enum class DragHandle
    {
        None,
        Start,
        End,
        Middle
    };

    explicit TimeLineTokenBase(QQuickItem* parent= nullptr);
    ~TimeLineTokenBase() override= default;

    QColor color() const;
    void setColor(const QColor& color);

    qreal radius() const;
    void setRadius(qreal newRadius);

    qreal secondInPixel() const;
    void setSecondInPixel(qreal newPixelToSecond);

    qreal duration() const;

    // Selection state. The base keeps it in a simple member; tokens backed by
    // a model object (e.g. a ClipInfo) override these to forward the state.
    virtual bool selected() const;
    virtual void setSelected(bool newSelected);

    void paint(QPainter* painter) override;

    // Label drawn in the center of the token.
    virtual QString labelText() const;

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;

    // Draws the start / end resize handles.
    void paintHandles(QPainter* painter, const QRectF& rect) const;

signals:
    void colorChanged();
    void selectedChanged();
    void radiusChanged();
    void secondInPixelChanged();
    void durationChanged();
    void contextualMenuRequested();
    void editionRequested();
    void timeChanged();

private:
    QColor m_color= Qt::red;
    qreal m_radius{4.0};
    qreal m_secondInPixel;
    bool m_selected{false};
    DragHandle m_dragHandle= DragHandle::None;
    QPointF m_initialClick;
};

#endif // TIMELINETOKENBASE_H
