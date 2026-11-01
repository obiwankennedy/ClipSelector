#include "timelinenode.h"
#include <QQuickWindow>
#include <QSGGeometry>
#include <QSGGeometryNode>
#include <QSGTextNode>
#include <QTextLayout>
#include <cmath>

TimeLineNode::TimeLineNode()
{
    m_node= new QSGGeometryNode();
    m_material= new QSGFlatColorMaterial();
    m_node->setMaterial(m_material);
    m_node->setFlag(QSGNode::OwnsMaterial, true);
    m_node->setFlag(QSGNode::OwnsGeometry, true);
    QSGGeometry* axisGeom= new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), 4);
    axisGeom->setDrawingMode(QSGGeometry::DrawTriangleStrip);
    m_node->setGeometry(axisGeom);
    appendChildNode(m_node);
}

void TimeLineNode::updateNode(qreal width, qreal height, const QColor& color)
{
    m_material->setColor(color);

    // set geometry
    QSGGeometry::Point2D* axisVerts= m_node->geometry()->vertexDataAsPoint2D();
    axisVerts[0].set(0.0, 0.0);
    axisVerts[1].set(0.0, height);
    axisVerts[2].set(width, 0.0);
    axisVerts[3].set(width, height);

    markDirty(QSGNode::DirtyGeometry | QSGNode::DirtyMaterial);
}