#ifndef TIMELINETOKEN_H
#define TIMELINETOKEN_H

#include <QAbstractItemModel>
#include <QColor>
#include <QFont>
#include <QQuickPaintedItem>
#include <QSGNode>

#include "clipmodel.h"
#include "timelinetokenbase.h"

class TimelineNode;

class TimeLineToken : public TimeLineTokenBase
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(ClipInfo* clipInfo READ clipInfo WRITE setClipInfo NOTIFY clipInfoChanged REQUIRED FINAL)

public:
    TimeLineToken(QQuickItem* parent= nullptr);
    ~TimeLineToken() override= default;

    bool selected() const override;
    void setSelected(bool newSelected) override;

    ClipInfo* clipInfo() const;
    void setClipInfo(ClipInfo* newClipInfo);

signals:
    void clipInfoChanged();

private:
    QPointer<ClipInfo> m_clipInfo;
};

#endif // TIMELINETOKEN_H
