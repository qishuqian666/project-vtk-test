#include "ModelPinelineBuilder.h"

#include <vtkPLYReader.h>
#include <vtkOBJReader.h>
#include <vtkTransform.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkElevationFilter.h>
#include <vtkVertexGlyphFilter.h>
#include <vtkPolyDataMapper.h>
#include <vtkLookupTable.h>
#include <vtkProperty.h>
#include <qDebug>

static vtkSmartPointer<vtkLookupTable> createJetLookupTable(double min, double max)
{
    auto lut = vtkSmartPointer<vtkLookupTable>::New();
    lut->SetTableRange(min, max);
    lut->SetHueRange(0.66667, 0.0); // Jet 色带：蓝到红
    lut->Build();
    return lut;
}

ModelPipelineBuilder::ModelPipelineBuilder()
{
    actor_ = vtkSmartPointer<vtkActor>::New();
}

bool ModelPipelineBuilder::loadModel(const QString &filePath)
{
    std::string ext = filePath.section('.', -1).toLower().toStdString();

    if (ext == "ply")
    {
        auto reader = vtkSmartPointer<vtkPLYReader>::New();
        reader->SetFileName(filePath.toStdString().c_str());
        reader->Update();
        originalPolyData_ = reader->GetOutput();
        modelType_ = ModelType::PLY;
    }
    else if (ext == "obj")
    {
        auto reader = vtkSmartPointer<vtkOBJReader>::New();
        reader->SetFileName(filePath.toStdString().c_str());
        reader->Update();
        originalPolyData_ = reader->GetOutput();
        modelType_ = ModelType::OBJ;
    }
    else
    {
        modelType_ = ModelType::UNKNOWN;
        return false;
    }

    if (!originalPolyData_ || originalPolyData_->GetNumberOfPoints() == 0)
        return false;

    setZAxisScale(1.0); // 默认拉伸为 1.0
    return true;
}

void ModelPipelineBuilder::setZAxisScale(double scale)
{
    zScale_ = scale;
    updatePipeline();
}

vtkSmartPointer<vtkActor> ModelPipelineBuilder::getActor() const
{
    return actor_;
}

vtkSmartPointer<vtkPolyData> ModelPipelineBuilder::getProcessedPolyData() const
{
    if (modelType_ == ModelType::OBJ)
    {
        return processedSurfacePolyData_;
    }
    else
    {
        return processedPolyData_;
    }
}

ModelPipelineBuilder::ModelType ModelPipelineBuilder::getModelType() const
{
    return modelType_;
}

void ModelPipelineBuilder::updatePipeline()
{
    if (!originalPolyData_)
        return;

    double bounds[6];
    originalPolyData_->GetBounds(bounds);
    double center[3] = {
        (bounds[0] + bounds[1]) * 0.5,
        (bounds[2] + bounds[3]) * 0.5,
        (bounds[4] + bounds[5]) * 0.5};

    // 中心对齐 + Z缩放
    auto transform = vtkSmartPointer<vtkTransform>::New();
    transform->Translate(-center[0], -center[1], -center[2]);
    transform->Scale(1.0, 1.0, zScale_);

    transformFilter_ = vtkSmartPointer<vtkTransformPolyDataFilter>::New();
    transformFilter_->SetInputData(originalPolyData_);
    transformFilter_->SetTransform(transform);
    transformFilter_->Update();

    // 修正 elevation 使用的 Z 范围（基于 transform 后的 bounds）
    double transformedBounds[6];
    transformFilter_->GetOutput()->GetBounds(transformedBounds);
    double lowZ = transformedBounds[4];
    double highZ = transformedBounds[5];

    // Elevation 着色
    elevationFilter_ = vtkSmartPointer<vtkElevationFilter>::New();
    elevationFilter_->SetInputConnection(transformFilter_->GetOutputPort());
    elevationFilter_->SetLowPoint(0, 0, lowZ);
    elevationFilter_->SetHighPoint(0, 0, highZ);
    elevationFilter_->Update();

    if (modelType_ == ModelType::OBJ)
    {
        // ================== 通用参数 ==================
        auto basePolyData = vtkPolyData::SafeDownCast(elevationFilter_->GetOutput());
        if (!basePolyData)
        {
            qDebug() << "Failed to cast elevation output to vtkPolyData";
            return;
        }
        double *scalarRange = elevationFilter_->GetOutput()->GetScalarRange();
        auto lut = createJetLookupTable(scalarRange[0], scalarRange[1]);
        // 保存用于后续计算（如 BoundingBox、Clipper）
        processedSurfacePolyData_ = basePolyData;

        // ========== 面 ==========
        auto surfaceMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        surfaceMapper->SetInputData(basePolyData);
        surfaceMapper->SetScalarRange(scalarRange);
        surfaceMapper->SetLookupTable(lut);
        surfaceMapper->SetColorModeToMapScalars();
        surfaceMapper->ScalarVisibilityOn();

        surfaceActor_ = vtkSmartPointer<vtkActor>::New();
        surfaceActor_->SetMapper(surfaceMapper);
        surfaceActor_->GetProperty()->SetOpacity(1.0);
        surfaceActor_->GetProperty()->SetRepresentationToSurface();

        // ========== 线 ==========
        auto wireframeMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        wireframeMapper->SetInputData(basePolyData);
        wireframeMapper->SetScalarRange(scalarRange);
        wireframeMapper->SetLookupTable(lut);
        wireframeMapper->SetColorModeToMapScalars();
        wireframeMapper->ScalarVisibilityOn();

        wireframeActor_ = vtkSmartPointer<vtkActor>::New();
        wireframeActor_->SetMapper(wireframeMapper);
        wireframeActor_->GetProperty()->SetRepresentationToWireframe();
        wireframeActor_->GetProperty()->SetColor(0.2, 0.2, 0.2);
        wireframeActor_->GetProperty()->SetLineWidth(1.0);

        // ========== 点 ==========
        auto glyphFilter = vtkSmartPointer<vtkVertexGlyphFilter>::New();
        glyphFilter->SetInputData(basePolyData);
        glyphFilter->Update();

        processedPointPolyData_ = vtkSmartPointer<vtkPolyData>::New();
        processedPointPolyData_->ShallowCopy(glyphFilter->GetOutput());

        auto pointsMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        pointsMapper->SetInputData(processedPointPolyData_);
        pointsMapper->SetScalarRange(processedPointPolyData_->GetScalarRange());
        pointsMapper->SetLookupTable(lut);
        pointsMapper->SetColorModeToMapScalars();
        pointsMapper->ScalarVisibilityOn();

        pointsActor_ = vtkSmartPointer<vtkActor>::New();
        pointsActor_->SetMapper(pointsMapper);
        pointsActor_->GetProperty()->SetRepresentationToPoints();
        pointsActor_->GetProperty()->SetPointSize(1.0);
        pointsActor_->GetProperty()->SetColor(0.0, 0.0, 1.0);

        // 默认主 actor 设为面（用于 Clipper、Slice 等）
        actor_ = surfaceActor_;
    }
    else if (modelType_ == ModelType::PLY)
    {
        // PLY 模型统一用 elevation + glyph 后的点数据表示
        auto glyphFilter = vtkSmartPointer<vtkVertexGlyphFilter>::New();
        glyphFilter->SetInputConnection(elevationFilter_->GetOutputPort());
        glyphFilter->Update();

        processedPointPolyData_ = vtkSmartPointer<vtkPolyData>::New();
        processedPointPolyData_->ShallowCopy(glyphFilter->GetOutput());

        auto lut = createJetLookupTable(
            processedPointPolyData_->GetScalarRange()[0],
            processedPointPolyData_->GetScalarRange()[1]);

        auto mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        mapper->SetInputData(processedPointPolyData_);
        mapper->SetScalarRange(processedPointPolyData_->GetScalarRange());
        mapper->SetLookupTable(lut);
        mapper->SetColorModeToMapScalars();
        mapper->ScalarVisibilityOn();

        actor_ = vtkSmartPointer<vtkActor>::New();
        actor_->SetMapper(mapper);

        // 保存为主数据（用于 BoundingBox、Clipper 等）
        processedSurfacePolyData_ = processedPointPolyData_;
    }
}

vtkSmartPointer<vtkTransformPolyDataFilter> ModelPipelineBuilder::getTransformFilter() const
{
    return transformFilter_;
}

vtkSmartPointer<vtkElevationFilter> ModelPipelineBuilder::getElevationFilter() const
{
    return elevationFilter_;
}
