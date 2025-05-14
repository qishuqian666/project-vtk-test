#include "ScaleBarController.h"
#include <vtkTextProperty.h>
#include <vtkCoordinate.h>
#include <vtkCamera.h>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <vtkProperty2D.h>
#include <qDebug>

ScaleBarController::ScaleBarController(vtkSmartPointer<vtkRenderer> renderer,
                                       vtkSmartPointer<vtkRenderWindow> renderWindow,
                                       vtkSmartPointer<vtkRenderWindowInteractor> interactor)
    : renderer_(renderer), renderWindow_(renderWindow), interactor_(interactor)
{
    qDebug() << "[ScaleBar]ScaleBarController()";
    CreateScaleBarActors();

    interactionCallback_ = vtkSmartPointer<vtkCallbackCommand>::New();
    interactionCallback_->SetCallback(ScaleBarController::OnInteractionEvent);
    interactionCallback_->SetClientData(this);
    // interactor_->AddObserver(vtkCommand::InteractionEvent, interactionCallback_);
    interactor_->AddObserver(vtkCommand::MouseWheelForwardEvent, interactionCallback_);
    interactor_->AddObserver(vtkCommand::MouseWheelBackwardEvent, interactionCallback_);
}

void ScaleBarController::CreateScaleBarActors()
{
    // 比例尺线条
    lineSource_ = vtkSmartPointer<vtkLineSource>::New();
    lineMapper_ = vtkSmartPointer<vtkPolyDataMapper2D>::New();
    lineMapper_->SetInputConnection(lineSource_->GetOutputPort());

    lineActor_ = vtkSmartPointer<vtkActor2D>::New();
    lineActor_->SetMapper(lineMapper_);
    lineActor_->GetProperty()->SetColor(1.0, 1.0, 1.0); // 白色
    lineActor_->GetPositionCoordinate()->SetCoordinateSystemToNormalizedDisplay();
    lineActor_->SetPosition(0.75, 0.05); // 可调整位置

    // 文本
    scaleText_ = vtkSmartPointer<vtkTextActor>::New();
    scaleText_->GetTextProperty()->SetFontSize(16);
    scaleText_->GetTextProperty()->SetColor(1.0, 1.0, 1.0);
    scaleText_->GetPositionCoordinate()->SetCoordinateSystemToNormalizedDisplay();
    scaleText_->SetPosition(0.78, 0.01); // 文本位于线下方

    renderer_->AddActor2D(lineActor_);
    renderer_->AddActor2D(scaleText_);
    qDebug() << "[scaleText_:]" << scaleText_;
    qDebug() << "[ScaleBar] 成功添加线条和文本到Renderer";

    UpdateScaleBar();
}

void ScaleBarController::UpdateScaleBar()
{
    double scale = GetCurrentScaleFactor(); // 每像素对应 world units
    double worldLength = scale * pixelLength_;

    // 跳变策略处理
    worldLength = ComputeRoundedLength(worldLength);
    // 如果长度小于 0.1m，固定在0.1m
    if (worldLength < 0.1)
        return;

    double pixelPerWorld = 1.0 / scale;
    double barPixel = worldLength * pixelPerWorld;

    qDebug() << "[ScaleBar] 当前 scale:" << scale
             << " 计算 worldLength:" << worldLength
             << " 显示 barPixel:" << barPixel;

    // 更新线条
    lineSource_->SetPoint1(0, 0, 0);
    lineSource_->SetPoint2(barPixel, 0, 0);
    lineSource_->Modified();

    // 更新文本
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << worldLength << " m";
    scaleText_->SetInput(oss.str().c_str());

    renderWindow_->Render();
}

void ScaleBarController::ReAddToRenderer()
{
    renderer_->AddActor2D(lineActor_);
    renderer_->AddActor2D(scaleText_);
    qDebug() << "[ScaleBar] 重新添加比例尺到 Renderer";
}

double ScaleBarController::GetCurrentScaleFactor()
{
    int *size = renderWindow_->GetSize();
    if (!size[0] || !size[1])
        return 1.0;

    vtkCamera *cam = renderer_->GetActiveCamera();
    double viewAngle = cam->GetViewAngle();         // degrees
    double parallelScale = cam->GetParallelScale(); // for parallel projection

    double result;
    if (cam->GetParallelProjection())
    {
        result = (2.0 * parallelScale) / size[1]; // world units per pixel
        qDebug() << "[ScaleBar] 使用 Parallel 投影" << result;
    }
    else
    {
        double viewHeight = 2.0 * tan(vtkMath::RadiansFromDegrees(viewAngle) / 2.0) * cam->GetDistance();
        result = viewHeight / size[1];
        qDebug() << "[ScaleBar] 使用 Perspective 投影" << result
                 << " CameraDistance:" << cam->GetDistance();
    }
    return result;
}

double ScaleBarController::ComputeRoundedLength(double length)
{
    if (length < 1.0)
        return std::round(length * 20.0) / 20.0; // step 0.05
    if (length < 10.0)
        return std::round(length * 2.0) / 2.0; // step 0.5
    if (length < 100.0)
        return std::round(length / 5.0) * 5.0; // step 5
    if (length < 1000.0)
        return std::round(length / 50.0) * 50.0; // step 50
    return std::round(length / 100.0) * 100.0;   // step 100
}

void ScaleBarController::OnInteractionEvent(vtkObject *caller, unsigned long eid,
                                            void *clientdata, void *)
{
    auto self = static_cast<ScaleBarController *>(clientdata); // 获取当前比例尺长度
    double scale = self->GetCurrentScaleFactor();
    double worldLength = scale * self->pixelLength_;
    double roundedLength = self->ComputeRoundedLength(worldLength);
    vtkCamera *cam = self->renderer_->GetActiveCamera();

    // 禁止进一步放大
    if (eid == vtkCommand::MouseWheelForwardEvent && roundedLength <= 0.1)
    {
        if (cam->GetParallelProjection())
        {
            cam->SetParallelScale(self->lastValidParallelScale_);
        }
        else
        {
            cam->SetDistance(self->lastValidCameraDistance_);
        }
        self->renderWindow_->Render();
        qDebug() << "[ScaleBar] Zoom limit reached. Reverted to last valid state.";
        return;
    }

    // 记录合法的相机状态
    if (cam->GetParallelProjection())
    {
        self->lastValidParallelScale_ = cam->GetParallelScale();
    }
    else
    {
        self->lastValidCameraDistance_ = cam->GetDistance();
    }
    self->UpdateScaleBar();
}
