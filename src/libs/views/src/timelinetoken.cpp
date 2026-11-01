#include "timelinetoken.h"

TimeLineToken::TimeLineToken(QQuickItem* parent) : TimeLineTokenBase(parent) {}

bool TimeLineToken::selected() const
{
    return m_clipInfo ? m_clipInfo->selected() : false;
}

void TimeLineToken::setSelected(bool newSelected)
{
    if(m_clipInfo)
        m_clipInfo->setSelected(newSelected);
}

ClipInfo* TimeLineToken::clipInfo() const
{
    return m_clipInfo;
}

void TimeLineToken::setClipInfo(ClipInfo* newClipInfo)
{
    if(m_clipInfo == newClipInfo)
        return;
    if(m_clipInfo)
        disconnect(m_clipInfo, nullptr, this, nullptr);

    m_clipInfo= newClipInfo;
    emit clipInfoChanged();

    connect(m_clipInfo, &ClipInfo::selectedChanged, this, [this]() { update(); });
}
