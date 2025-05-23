// 封装模型加载和基础处理（如 elevation 和 Z 拉伸等）
#pragma once

#include <vtkSmartPointer.h>
#include <vtkPolyData.h>
#include <vtkActor.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkElevationFilter.h>
#include <vtkOBJReader.h>
#include <vtkPLYReader.h>
#include <QString>

class ModelPipelineBuilder
{
public:
    enum class ModelType
    {
        UNKNOWN,
        PLY,
        OBJ
    };

    ModelPipelineBuilder();

    bool loadModel(const QString &filePath);
    void setZAxisScale(double scale);
    vtkSmartPointer<vtkActor> getActor() const;
    vtkSmartPointer<vtkPolyData> getProcessedPolyData() const;
    ModelType getModelType() const;
    vtkSmartPointer<vtkTransformPolyDataFilter> getTransformFilter() const;
    vtkSmartPointer<vtkElevationFilter> getElevationFilter() const;
    // 加载obj文件的点线面数据
    vtkSmartPointer<vtkActor> getSurfaceActor() const { return surfaceActor_; }
    vtkSmartPointer<vtkActor> getWireframeActor() const { return wireframeActor_; }
    vtkSmartPointer<vtkActor> getPointsActor() const { return pointsActor_; }

private:
    void updatePipeline();

private:
    ModelType modelType_ = ModelType::UNKNOWN;
    double zScale_ = 1.0;

    vtkSmartPointer<vtkPolyData> originalPolyData_;
    vtkSmartPointer<vtkPolyData> processedPolyData_;
    vtkSmartPointer<vtkActor> actor_;

    vtkSmartPointer<vtkTransformPolyDataFilter> transformFilter_;
    vtkSmartPointer<vtkElevationFilter> elevationFilter_;

    // 加载obj文件的点线面数据
    vtkSmartPointer<vtkPolyData> processedSurfacePolyData_;
    vtkSmartPointer<vtkPolyData> processedPointPolyData_;
    vtkSmartPointer<vtkActor> surfaceActor_;
    vtkSmartPointer<vtkActor> wireframeActor_;
    vtkSmartPointer<vtkActor> pointsActor_;
};
