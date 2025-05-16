
/**
 * @file ScaleBarController.h
 * @brief 该头文件定义了 ScaleBarController 类，用于管理并绘制屏幕固定像素长度的比例尺。
 * @details 该类负责处理与比例尺相关的各种操作，包括创建比例尺元素、更新显示内容、
 *          监听用户交互事件等。同时，它还处理不同投影模式下的相机状态，确保比例尺
 *          能正确反映当前场景的缩放比例。
 * @author qtree
 * @date 2025年5月14日
 */

#ifndef SCALEBARCONTROLLER_H
#define SCALEBARCONTROLLER_H

#include <vtkSmartPointer.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkTextActor.h>
#include <vtkLineSource.h>
#include <vtkPolyDataMapper2D.h>
#include <vtkActor2D.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkCallbackCommand.h>

// 管理并绘制屏幕固定像素长度的比例尺控制器
class ScaleBarController
{
public:
    // 构造函数，初始化渲染器、窗口和交互器
    ScaleBarController(vtkSmartPointer<vtkRenderer> renderer,
                       vtkSmartPointer<vtkRenderWindow> renderWindow,
                       vtkSmartPointer<vtkRenderWindowInteractor> interactor);

    // 更新比例尺显示内容（长度、位置、标签等）
    void UpdateScaleBar();

    // 当比例尺需要重新添加回 renderer 时调用（如清空或重建渲染场景后）
    void ReAddToRenderer();

private:
    // 交互事件的静态回调函数，用于处理缩放限制、比例尺更新等逻辑
    static void OnInteractionEvent(vtkObject *caller, unsigned long eid,
                                   void *clientdata, void *calldata);

    // 创建比例尺的 VTK 对象，包括线段和文字
    void CreateScaleBarActors();

    // 获取当前屏幕下 1 像素对应多少 world 单位（单位：world units / pixel）
    double GetCurrentScaleFactor();

    // 根据传入的真实比例长度，计算美观的跳变长度（如：0.5 → 1 → 5 → 10...）
    double ComputeRoundedLength(double length);

    // -------------------------
    // VTK 渲染相关对象
    // -------------------------
    vtkSmartPointer<vtkRenderer> renderer_;                 // 场景渲染器
    vtkSmartPointer<vtkRenderWindow> renderWindow_;         // 渲染窗口
    vtkSmartPointer<vtkRenderWindowInteractor> interactor_; // 用户交互器

    // -------------------------
    // 比例尺的绘制元素
    // -------------------------
    vtkSmartPointer<vtkLineSource> lineSource_;       // 比例尺的线段
    vtkSmartPointer<vtkPolyDataMapper2D> lineMapper_; // 2D 映射器
    vtkSmartPointer<vtkActor2D> lineActor_;           // 2D 绘制 actor

    vtkSmartPointer<vtkTextActor> scaleText_; // 比例尺文字标签（显示数值）

    vtkSmartPointer<vtkCallbackCommand> interactionCallback_; // 鼠标缩放事件监听回调

    // -------------------------
    // 状态控制参数
    // -------------------------
    const int pixelLength_ = 200; // 比例尺在屏幕上固定显示的像素长度（单位 px）

    double lastValidParallelScale_ = 1.0;  // 上一次合法 Parallel 投影的相机缩放值
    double lastValidCameraDistance_ = 1.0; // 上一次合法 Perspective 投影的相机距离
};

#endif // SCALEBARCONTROLLER_H
