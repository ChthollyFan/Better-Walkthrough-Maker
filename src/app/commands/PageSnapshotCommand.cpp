/**
 * @file PageSnapshotCommand.cpp
 * @author zhangweimu
 * @brief 页面组件快照撤销命令实现。
 */
#include "app/commands/PageSnapshotCommand.h"

#include "core/Project.h"
#include "editor/CanvasScene.h"

namespace bwm {

PageSnapshotCommand::PageSnapshotCommand(Page* pPage, const QVector<Component>& rBefore,
                                         const QVector<Component>& rAfter, const QString& strText,
                                         CanvasScene* pScene)
    : QUndoCommand(strText)
    , m_pPage(pPage)
    , m_vecBefore(rBefore)
    , m_vecAfter(rAfter)
    , m_pScene(pScene)
{
}

void PageSnapshotCommand::undo()
{
    apply(m_vecBefore);
}

void PageSnapshotCommand::redo()
{
    apply(m_vecAfter);
}

void PageSnapshotCommand::apply(const QVector<Component>& rComponents)
{
    // 模型已是目标状态时无需重建（如 QUndoStack::push 时 redo 与现态一致）
    if(m_pPage->vecComponents == rComponents) {
        return;
    }
    m_pPage->vecComponents = rComponents;
    m_pScene->loadPage(*m_pPage);
    emit m_pScene->componentsChanged();   // 触发主窗口同步与图层刷新
}

} // namespace bwm
