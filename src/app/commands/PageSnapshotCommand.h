/**
 * @file PageSnapshotCommand.h
 * @author zhangweimu
 * @brief 基于页面组件快照的撤销命令。
 *
 * 此命令在编辑事务（移动/缩放/对齐/粘贴等）开始时记录组件列表快照，
 * 结束时记录新状态。undo/redo 时整体恢复组件列表并重建场景。
 *
 * 从原 MainWindow.cpp 的匿名命名空间中提取为独立类，
 * 便于 MainWindow 和未来其他编辑器复用。
 */
#ifndef BWM_APP_COMMANDS_PAGESNAPSHOTCOMMAND_H
#define BWM_APP_COMMANDS_PAGESNAPSHOTCOMMAND_H

#include <QUndoCommand>
#include <QVector>

#include "core/Component.h"

namespace bwm {

struct Page;
class CanvasScene;

/**
 * @brief 页面组件快照撤销命令。
 *
 * 记录编辑前后的组件列表，undo/redo 时恢复并通知场景重建。
 * 适用于整体性编辑（对齐、分布、粘贴、删除等），不适用于
 * 需要细粒度合并的连续操作（如逐像素拖动）。
 */
class PageSnapshotCommand : public QUndoCommand
{
public:
    /**
     * @param pPage     目标页面
     * @param rBefore   编辑前的组件列表快照
     * @param rAfter    编辑后的组件列表快照
     * @param strText   命令描述（显示在撤销菜单）
     * @param pScene    画布场景（undo/redo 时调用 loadPage 重建）
     */
    PageSnapshotCommand(Page* pPage, const QVector<Component>& rBefore,
                        const QVector<Component>& rAfter, const QString& strText,
                        CanvasScene* pScene);

    void undo() override;
    void redo() override;

private:
    /**
     * @brief 应用指定快照到页面并重建场景。
     * @param rComponents  要恢复的组件列表
     *
     * 若模型已是目标状态则跳过（避免 QUndoStack::push 时 redo 与现态重复）。
     */
    void apply(const QVector<Component>& rComponents);

    Page* m_pPage;                  ///< 目标页面
    QVector<Component> m_vecBefore; ///< 编辑前快照
    QVector<Component> m_vecAfter;  ///< 编辑后快照
    CanvasScene* m_pScene;          ///< 画布场景
};

} // namespace bwm

#endif // BWM_APP_COMMANDS_PAGESNAPSHOTCOMMAND_H
