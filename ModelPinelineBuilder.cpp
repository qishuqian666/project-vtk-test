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
    // 清空旧状态
    resetState();

    // 变换（中心对齐 + Z拉伸）
    applyTransform();

    // 着色（按 Z 高度生成 scalar）
    applyElevationColoring();

    // 按模型类型构建渲染管线
    if (modelType_ == ModelType::OBJ)
        setupOBJPipeline();
    else if (modelType_ == ModelType::PLY)
        setupPLYPipeline();
}

void ModelPipelineBuilder::resetState()
{
    transformFilter_ = nullptr;
    elevationFilter_ = nullptr;
    processedSurfacePolyData_ = nullptr;
    processedPointPolyData_ = nullptr;
}

void ModelPipelineBuilder::applyTransform()
{
    double bounds[6];
    originalPolyData_->GetBounds(bounds);
    double center[3] = {
        (bounds[0] + bounds[1]) * 0.5,
        (bounds[2] + bounds[3]) * 0.5,
        (bounds[4] + bounds[5]) * 0.5};

    auto transform = vtkSmartPointer<vtkTransform>::New();
    transform->Translate(-center[0], -center[1], -center[2]);
    transform->Scale(1.0, 1.0, zScale_);

    transformFilter_ = vtkSmartPointer<vtkTransformPolyDataFilter>::New();
    transformFilter_->SetInputData(originalPolyData_);
    transformFilter_->SetTransform(transform);
    transformFilter_->Update();
}

void ModelPipelineBuilder::applyElevationColoring()
{
    // 使用变换后的数据计算 elevation
    double bounds[6];
    transformFilter_->GetOutput()->GetBounds(bounds);
    double lowZ = bounds[4];
    double highZ = bounds[5];

    elevationFilter_ = vtkSmartPointer<vtkElevationFilter>::New();
    elevationFilter_->SetInputConnection(transformFilter_->GetOutputPort());
    elevationFilter_->SetLowPoint(0, 0, lowZ);
    elevationFilter_->SetHighPoint(0, 0, highZ);
    elevationFilter_->Update();
}

void ModelPipelineBuilder::setupOBJPipeline()
{
    auto basePolyData = vtkPolyData::SafeDownCast(elevationFilter_->GetOutput());
    if (!basePolyData)
        return;

    double *scalarRange = basePolyData->GetScalarRange();
    auto lut = createJetLookupTable(scalarRange[0], scalarRange[1]);

    processedSurfacePolyData_ = basePolyData;

    // ---------- 面 ----------
    if (!surfaceActor_)
        surfaceActor_ = vtkSmartPointer<vtkActor>::New();

    auto surfaceMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    surfaceMapper->SetInputData(basePolyData);
    surfaceMapper->SetScalarRange(scalarRange);
    surfaceMapper->SetLookupTable(lut);
    surfaceMapper->SetColorModeToMapScalars();
    surfaceMapper->ScalarVisibilityOn();

    surfaceActor_->SetMapper(surfaceMapper);
    surfaceActor_->GetProperty()->SetOpacity(1.0);
    surfaceActor_->GetProperty()->SetRepresentationToSurface();
    surfaceActor_->GetProperty()->LightingOff(); // 纯色不受光照

    // ---------- 线 ----------
    if (!wireframeActor_)
        wireframeActor_ = vtkSmartPointer<vtkActor>::New();

    auto wireframeMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    wireframeMapper->SetInputData(basePolyData);
    wireframeMapper->SetScalarRange(scalarRange);
    wireframeMapper->SetLookupTable(lut);
    wireframeMapper->SetColorModeToMapScalars();
    wireframeMapper->ScalarVisibilityOn();

    wireframeActor_->SetMapper(wireframeMapper);
    wireframeActor_->GetProperty()->SetRepresentationToWireframe();
    wireframeActor_->GetProperty()->SetColor(0.2, 0.2, 0.2);
    wireframeActor_->GetProperty()->SetLineWidth(1.0);
    wireframeActor_->GetProperty()->LightingOff();

    // ---------- 点 ----------
    auto glyphFilter = vtkSmartPointer<vtkVertexGlyphFilter>::New();
    glyphFilter->SetInputData(basePolyData);
    glyphFilter->Update();

    auto outputPolyData = glyphFilter->GetOutput();
    if (!outputPolyData || outputPolyData->GetNumberOfPoints() == 0)
        return;

    processedPointPolyData_ = vtkSmartPointer<vtkPolyData>::New();
    processedPointPolyData_->ShallowCopy(outputPolyData);

    if (!pointsActor_)
        pointsActor_ = vtkSmartPointer<vtkActor>::New();

    auto pointsMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    pointsMapper->SetInputData(processedPointPolyData_);
    pointsMapper->SetScalarRange(scalarRange);
    pointsMapper->SetLookupTable(lut);
    pointsMapper->SetColorModeToMapScalars();
    pointsMapper->ScalarVisibilityOn();

    pointsActor_->SetMapper(pointsMapper);
    pointsActor_->GetProperty()->SetRepresentationToPoints();
    pointsActor_->GetProperty()->SetPointSize(1.0);
    pointsActor_->GetProperty()->LightingOff();

    // 设置主 actor（用于 bbox、clipper）
    actor_ = surfaceActor_;
}

void ModelPipelineBuilder::setupPLYPipeline()
{
    auto glyphFilter = vtkSmartPointer<vtkVertexGlyphFilter>::New();
    glyphFilter->SetInputConnection(elevationFilter_->GetOutputPort());
    glyphFilter->Update();

    auto polyData = glyphFilter->GetOutput();
    if (!polyData || polyData->GetNumberOfPoints() == 0)
        return;

    processedPolyData_ = vtkSmartPointer<vtkPolyData>::New();
    processedPolyData_->ShallowCopy(polyData);

    auto scalarRange = processedPolyData_->GetScalarRange();
    auto lut = createJetLookupTable(scalarRange[0], scalarRange[1]);

    auto mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputData(processedPolyData_);
    mapper->SetScalarRange(scalarRange);
    mapper->SetLookupTable(lut);
    mapper->SetColorModeToMapScalars();
    mapper->ScalarVisibilityOn();

    actor_->SetMapper(mapper);
    actor_->GetProperty()->SetRepresentationToPoints();
    actor_->GetProperty()->SetPointSize(1.0);
    actor_->GetProperty()->LightingOff(); // 确保无光照影响
}

vtkSmartPointer<vtkTransformPolyDataFilter> ModelPipelineBuilder::getTransformFilter() const
{
    return transformFilter_;
}

vtkSmartPointer<vtkElevationFilter> ModelPipelineBuilder::getElevationFilter() const
{
    return elevationFilter_;
}
