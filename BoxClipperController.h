/**
 * @file BoxClipperController.h
 * @brief 该头文件定义了 BoxClipperController 类，用于管理基于盒子的网格数据裁剪操作。
 * @details 该类提供了使用 vtkBoxWidget 对网格数据进行裁剪的功能，允许用户通过交互方式调整裁剪盒子的位置和大小，
 *          并实时更新裁剪结果。
 * @author qtree
 * @date 2025年5月16日
 */
#ifndef BOXCLIPPERCONTROLLER_H
#define BOXCLIPPERCONTROLLER_H

#include <vtkSmartPointer.h>
#include <vtkCallbackCommand.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkObjectBase.h>

class vtkRenderWindowInteractor;
class vtkBoxWidget;
class vtkPlanes;
class vtkClipPolyData;
class vtkPolyData;
class vtkActor;
class vtkRenderer;

/**
 * @class BoxClipperController
 * @brief 用于管理基于盒子的网格数据裁剪操作的控制器类。
 *
 * 该类提供了使用 vtkBoxWidget 对网格数据进行裁剪的功能，允许用户通过交互方式调整裁剪盒子的位置和大小，
 * 并实时更新裁剪结果。
 */
class BoxClipperController
{
public:
    /**
     * @brief 构造函数。
     *
     * 初始化 BoxClipperController 实例，并关联渲染窗口交互器和渲染器。
     *
     * @param interactor 用于处理用户交互的渲染窗口交互器。
     * @param renderer 用于渲染裁剪结果的渲染器。
     */
    BoxClipperController(vtkRenderWindowInteractor *interactor, vtkRenderer *renderer);

    /**
     * @brief 设置输入数据并替换原始的 Actor。
     *
     * 该方法设置用于裁剪的输入网格数据，并将原始的 Actor 替换为裁剪后的 Actor。
     *
     * @param input 用于裁剪的输入网格数据。
     * @param originalActor 原始的 Actor，将被裁剪后的 Actor 替换。
     */
    void SetInputDataAndReplaceOriginal(vtkSmartPointer<vtkPolyData> input, vtkActor *originalActor);

    /**
     * @brief 启用或禁用裁剪功能。
     *
     * 该方法控制 vtkBoxWidget 的显示和交互状态，从而启用或禁用裁剪功能。
     *
     * @param enabled 若为 true，则启用裁剪功能；若为 false，则禁用裁剪功能。
     */
    void SetEnabled(bool enabled);

    /**
     * @brief 获取裁剪后的 Actor。
     *
     * 返回经过裁剪操作后生成的 Actor。
     *
     * @return 裁剪后的 Actor 的智能指针。
     */
    vtkSmartPointer<vtkActor> GetClippedActor();

private:
    vtkSmartPointer<vtkBoxWidget> boxWidget;               ///< 用于用户交互的盒子小部件，用于定义裁剪区域
    vtkSmartPointer<vtkPlanes> clipPlanes;                 ///< 由盒子小部件定义的裁剪平面
    vtkSmartPointer<vtkClipPolyData> clipper;              ///< 用于执行裁剪操作的 VTK 过滤器
    vtkSmartPointer<vtkActor> clippedActor;                ///< 裁剪后的网格数据对应的 Actor
    vtkSmartPointer<vtkPolyData> inputData;                ///< 用于裁剪的输入网格数据
    vtkSmartPointer<vtkRenderer> renderer;                 ///< 用于渲染裁剪结果的渲染器
    vtkSmartPointer<vtkRenderWindowInteractor> interactor; ///< 用于处理用户交互的渲染窗口交互器
    vtkActor *originalActor;                               ///< 原始的 Actor，将被裁剪后的 Actor 替换

    /**
     * @brief 更新裁剪结果。
     *
     * 当盒子小部件的位置或大小发生变化时，该方法会重新执行裁剪操作并更新裁剪后的 Actor。
     */
    void UpdateClipping();

    void CopyActorAppearance(vtkActor *from, vtkActor *to);
};

#endif // BOXCLIPPERCONTROLLER_H
