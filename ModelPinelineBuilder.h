/**
 * @file ModelPinelineBuilder.h
 * @brief 该头文件定义了 ModelPipelineBuilder 类，用于封装模型加载和基础处理（如 elevation 和 Z 拉伸等）。
 * @details 该类提供了加载 PLY 和 OBJ 格式模型的功能，并且支持对模型进行 Z 轴拉伸、Elevation 着色等处理，还能获取处理后的模型数据和 Actor。
 * @author 高子奇
 * @date 2025年5月23日
 */
#pragma once

#include <vtkSmartPointer.h>
#include <vtkPolyData.h>
#include <vtkActor.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkElevationFilter.h>
#include <vtkOBJReader.h>
#include <vtkPLYReader.h>
#include <QString>

/**
 * @class ModelPipelineBuilder
 * @brief 封装模型加载和基础处理的类，支持 PLY 和 OBJ 格式模型的加载，并能对模型进行 Z 轴拉伸和 Elevation 着色等处理。
 */
class ModelPipelineBuilder
{
public:
    /**
     * @enum ModelType
     * @brief 定义支持的模型类型枚举。
     */
    enum class ModelType
    {
        UNKNOWN, ///< 未知模型类型
        PLY,     ///< PLY 格式模型
        OBJ      ///< OBJ 格式模型
    };

    /**
     * @brief 构造函数，初始化类的成员变量。
     */
    ModelPipelineBuilder();

    /**
     * @brief 加载指定路径的模型文件。
     * @param filePath 模型文件的路径。
     * @return 如果模型加载成功返回 true，否则返回 false。
     */
    bool loadModel(const QString &filePath);

    /**
     * @brief 设置模型在 Z 轴上的拉伸比例。
     * @param scale Z 轴的拉伸比例。
     */
    void setZAxisScale(double scale);

    /**
     * @brief 获取处理后的模型对应的 Actor。
     * @return 处理后的模型的 Actor 智能指针。
     */
    vtkSmartPointer<vtkActor> getActor() const;

    /**
     * @brief 获取处理后的多边形数据。
     * @return 处理后的多边形数据的智能指针。
     */
    vtkSmartPointer<vtkPolyData> getProcessedPolyData() const;

    /**
     * @brief 获取当前加载模型的类型。
     * @return 当前加载模型的类型，为 ModelType 枚举值。
     */
    ModelType getModelType() const;

    /**
     * @brief 获取用于对模型进行变换的过滤器。
     * @return 变换过滤器的智能指针。
     */
    vtkSmartPointer<vtkTransformPolyDataFilter> getTransformFilter() const;

    /**
     * @brief 获取用于对模型进行 Elevation 着色的过滤器。
     * @return Elevation 过滤器的智能指针。
     */
    vtkSmartPointer<vtkElevationFilter> getElevationFilter() const;

    /**
     * @brief 获取 OBJ 文件的面数据对应的 Actor。
     * @return OBJ 文件面数据的 Actor 智能指针。
     */
    vtkSmartPointer<vtkActor> getSurfaceActor() const { return surfaceActor_; }

    /**
     * @brief 获取 OBJ 文件的线框数据对应的 Actor。
     * @return OBJ 文件线框数据的 Actor 智能指针。
     */
    vtkSmartPointer<vtkActor> getWireframeActor() const { return wireframeActor_; }

    /**
     * @brief 获取 OBJ 文件的点数据对应的 Actor。
     * @return OBJ 文件点数据的 Actor 智能指针。
     */
    vtkSmartPointer<vtkActor> getPointsActor() const { return pointsActor_; }

private:
    /**
     * @brief 更新模型处理流程，包括变换、Elevation 着色等操作。
     * 当模型数据或 Z 轴拉伸比例发生变化时，调用此方法更新处理结果。
     */
    void updatePipeline();

    // 清空旧状态
    void resetState();
    // 变换（中心对齐 + Z拉伸）
    void applyTransform();
    // 着色（按 Z 高度生成 scalar）
    void applyElevationColoring();
    void setupOBJPipeline();
    void setupPLYPipeline();

private:
    ModelType modelType_ = ModelType::UNKNOWN; ///< 当前加载模型的类型，默认为未知类型
    double zScale_ = 1.0;                      ///< 模型在 Z 轴上的拉伸比例，默认为 1.0

    vtkSmartPointer<vtkPolyData> originalPolyData_;  ///< 原始的多边形数据，即加载的模型数据
    vtkSmartPointer<vtkPolyData> processedPolyData_; ///< 处理后的多边形数据
    vtkSmartPointer<vtkActor> actor_;                ///< 处理后的模型对应的 Actor

    vtkSmartPointer<vtkTransformPolyDataFilter> transformFilter_; ///< 用于对模型进行变换的过滤器
    vtkSmartPointer<vtkElevationFilter> elevationFilter_;         ///< 用于对模型进行 Elevation 着色的过滤器

    // 加载obj文件的点线面数据
    vtkSmartPointer<vtkPolyData> processedSurfacePolyData_; ///< 处理后的 OBJ 文件面数据
    vtkSmartPointer<vtkPolyData> processedPointPolyData_;   ///< 处理后的 OBJ 文件点数据
    vtkSmartPointer<vtkActor> surfaceActor_;                ///< OBJ 文件面数据对应的 Actor
    vtkSmartPointer<vtkActor> wireframeActor_;              ///< OBJ 文件线框数据对应的 Actor
    vtkSmartPointer<vtkActor> pointsActor_;                 ///< OBJ 文件点数据对应的 Actor
};
