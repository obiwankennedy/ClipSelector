#ifndef TIMELINENODE_H
#define TIMELINENODE_H

#include <QColor>
#include <QFont>
#include <QSGFlatColorMaterial>
#include <QSGNode>

class QSGTextNode;
class QQuickWindow;

class TimeLineNode : public QSGNode
{
public:
    TimeLineNode();
    ~TimeLineNode() override= default;

    void updateNode(qreal width, qreal height, const QColor& color);

private:
    QSGGeometryNode* m_node;
    QSGFlatColorMaterial* m_material;
};

#endif // TIMELINENODE_H
