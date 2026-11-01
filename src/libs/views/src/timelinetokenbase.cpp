#include "timelinetokenbase.h"

#include <QLineF>
#include <QPainter>
#include <QtDebug>

namespace
{
constexpr int HandleRadius{6};
constexpr int DragMargin{8};
} // namespace

TimeLineTokenBase::TimeLineTokenBase(QQuickItem* parent) : QQuickPaintedItem(parent)
{
    setFlag(ItemHasContents, true);
    setAcceptedMouseButtons(Qt::AllButtons);
    setAntialiasing(true);
}

QColor TimeLineTokenBase::color() const
{
    return m_color;
}

void TimeLineTokenBase::setColor(const QColor& color)
{
    m_color= color;
    emit colorChanged();
    update();
}

qreal TimeLineTokenBase::radius() const
{
    return m_radius;
}

void TimeLineTokenBase::setRadius(qreal newRadius)
{
    if(qFuzzyCompare(m_radius, newRadius))
        return;
    m_radius= newRadius;
    emit radiusChanged();
}

qreal TimeLineTokenBase::secondInPixel() const
{
    return m_secondInPixel;
}

void TimeLineTokenBase::setSecondInPixel(qreal newPixelToSecond)
{
    if(qFuzzyCompare(m_secondInPixel, newPixelToSecond))
        return;
    m_secondInPixel= newPixelToSecond;
    emit secondInPixelChanged();
}

qreal TimeLineTokenBase::duration() const
{
    if(m_secondInPixel <= 0.0)
        return 0.0;
    return width() / m_secondInPixel;
}

bool TimeLineTokenBase::selected() const
{
    return m_selected;
}

void TimeLineTokenBase::setSelected(bool newSelected)
{
    if(m_selected == newSelected)
        return;
    m_selected= newSelected;
    emit selectedChanged();
    update();
}

QString TimeLineTokenBase::labelText() const
{
    return QString("%1s").arg(duration(), 0, 'f', 2);
}

void TimeLineTokenBase::paint(QPainter* painter)
{
    painter->save();

    painter->setRenderHint(QPainter::Antialiasing, true);

    const QRectF rect= boundingRect();

    // Filled rounded rectangle.
    painter->setPen(Qt::NoPen);
    painter->setBrush(m_color);
    painter->drawRoundedRect(rect, m_radius, m_radius);

    if(selected())
        paintHandles(painter, rect);

    painter->setPen(m_color.lightness() > 127 ? Qt::black : Qt::white);
    painter->drawText(rect, Qt::AlignCenter, labelText());

    painter->restore();
}

void TimeLineTokenBase::paintHandles(QPainter* painter, const QRectF& rect) const
{
    const qreal handleRadius= 6.0;
    const QRectF startHandle(rect.left() - handleRadius, rect.center().y() - handleRadius, 2.0 * handleRadius,
                             2.0 * handleRadius);

    const QRectF endHandle(rect.right() - handleRadius, rect.center().y() - handleRadius, 2.0 * handleRadius,
                           2.0 * handleRadius);

    // White handles with a blue outline.
    painter->setBrush(Qt::white);
    painter->setPen(QPen(QColor("#2563EB"), 1.5));

    painter->drawEllipse(startHandle);
    painter->drawEllipse(endHandle);
}

void TimeLineTokenBase::mousePressEvent(QMouseEvent* event)
{
    if(event->button() == Qt::RightButton)
    {
        emit contextualMenuRequested();
    }
    if(event->button() != Qt::LeftButton)
    {
        event->ignore();
        return;
    }

    const QPointF pos= event->position();
    const QRectF rect= boundingRect();

    if(selected())
    {
        // Only start resizing when a selected handle is clicked.
        const qreal cy= rect.center().y();
        const qreal hitRadius= HandleRadius + DragMargin;
        m_initialClick= pos;

        const QPointF start(rect.left(), cy);
        const QPointF end(rect.right(), cy);

        if(QLineF(pos, start).length() <= hitRadius)
        {
            m_dragHandle= DragHandle::Start;
            event->accept();
            return;
        }

        if(QLineF(pos, end).length() <= hitRadius)
        {
            m_dragHandle= DragHandle::End;
            event->accept();
            return;
        }

        m_dragHandle= DragHandle::Middle;
        event->accept();
        return;
    }

    // A normal click selects the item.
    if(rect.contains(pos))
    {
        setSelected(true);
        m_dragHandle= DragHandle::None;
        event->accept();
        return;
    }

    // Clicking outside the rectangle deselects it.
    setSelected(false);
    m_dragHandle= DragHandle::None;
    event->accept();
}

void TimeLineTokenBase::mouseMoveEvent(QMouseEvent* event)
{
    if(m_dragHandle == DragHandle::None)
    {
        event->ignore();
        return;
    }

    const qreal x= event->position().x();

    if(m_dragHandle == DragHandle::Start)
    {
        if(x >= width())
            return;
        auto offset= x - m_initialClick.x();
        setX(this->x() + offset);
        setWidth(width() + (offset * -1));
        emit timeChanged();
    }
    else if(m_dragHandle == DragHandle::End)
    {
        auto offset= x - m_initialClick.x();
        setWidth(width() + offset);
        m_initialClick= event->position();
        emit timeChanged();
    }
    else if(m_dragHandle == DragHandle::Middle)
    {
        auto offset= x - m_initialClick.x();
        setX(this->x() + offset);
        // Push the new position back to the model (begin/end) so the effects
        // (e.g. the comment audio start position) follow the moved token.
        emit timeChanged();
    }

    update();
    event->accept();
}

void TimeLineTokenBase::mouseReleaseEvent(QMouseEvent* event)
{
    if(event->button() == Qt::LeftButton)
    {
        m_dragHandle= DragHandle::None;
        event->accept();
        return;
    }

    event->ignore();
}

void TimeLineTokenBase::mouseDoubleClickEvent(QMouseEvent* event)
{
    if(event->button() == Qt::LeftButton)
    {
        emit editionRequested();
        event->accept();
        return;
    }

    event->ignore();
}
