
/**
 * @file MeshSliceController.h
 * @brief 该头文件定义了 MeshSliceController 类，用于显示切面图。
 * @details 该类负责处理与网格切面相关的操作，包括创建切面、显示和隐藏切面，以及更新用于切面操作的网格数据。
 *          注意注释中提到的比例尺相关内容可能是文档编写错误，实际该类与比例尺无关。
 * @author qtree
 * @date 2025年5月14日
 */
#ifndef MESHSLICECONTROLLER_H
#define MESHSLICECONTROLLER_H

#include <vtkSmartPointer.h>
#include <vtkPolyData.h>
#include <vtkPlane.h>
#include <vtkCutter.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkRenderer.h>

/**
 * @enum SliceDirection
 * @brief 定义切面的方向枚举类型。
 */
enum SliceDirection
{
    SLICE_X, ///< 沿 X 轴方向进行切面
    SLICE_Y, ///< 沿 Y 轴方向进行切面
    SLICE_Z  ///< 沿 Z 轴方向进行切面
};

/**
 * @class MeshSliceController
 * @brief 用于管理和显示网格数据切面的类。
 *
 * 该类提供了显示、隐藏切面，以及更新网格数据的功能。
 */
class MeshSliceController
{
public:
    /**
     * @brief 构造函数。
     * @param renderer 用于渲染切面的渲染器。
     */
    MeshSliceController(vtkSmartPointer<vtkRenderer> renderer);

    /**
     * @brief 根据指定方向显示切面。
     * @param direction 切面的方向，取值为 SliceDirection 枚举类型。
     */
    void ShowSlice(SliceDirection direction);

    /**
     * @brief 隐藏当前显示的切面。
     */
    void HideSlice();

    /**
     * @brief 更新用于切面操作的网格数据。
     * @param polyData 新的网格数据。
     */
    void UpdatePolyData(vtkSmartPointer<vtkPolyData> polyData);

    /**
     * @brief 设置原始网格数据的 Actor。
     * @param actor 原始网格数据的 Actor。
     */
    void SetOriginalActor(vtkSmartPointer<vtkActor> actor);

private:
    vtkSmartPointer<vtkRenderer> renderer_; ///< 用于渲染切面的渲染器
    vtkSmartPointer<vtkPolyData> polyData_; ///< 用于切面操作的网格数据

    vtkSmartPointer<vtkPlane> slicePlane_;           ///< 定义切面的平面
    vtkSmartPointer<vtkCutter> cutter_;              ///< 用于切割网格数据的切割器
    vtkSmartPointer<vtkPolyDataMapper> sliceMapper_; ///< 切面数据的映射器
    vtkSmartPointer<vtkActor> sliceActor_;           ///< 用于显示切面的 Actor

    vtkSmartPointer<vtkActor> originalActor_; ///< 原始网格数据的 Actor
};

#endif // MESHSLICECONTROLLER_H
