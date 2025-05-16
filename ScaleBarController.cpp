#include "ScaleBarController.h"
#include <vtkTextProperty.h>
#include <vtkCoordinate.h>
#include <vtkCamera.h>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <vtkProperty2D.h>
#include <qDebug>

// 构造函数：初始化成员变量、比例尺图元和交互事件监听器
ScaleBarController::ScaleBarController(vtkSmartPointer<vtkRenderer> renderer,
                                       vtkSmartPointer<vtkRenderWindow> renderWindow,
                                       vtkSmartPointer<vtkRenderWindowInteractor> interactor)
    : renderer_(renderer), renderWindow_(renderWindow), interactor_(interactor)
{
    // qDebug() << "[ScaleBar]ScaleBarController()";

    // 创建比例尺线条与文本
    CreateScaleBarActors();

    // 绑定交互事件（滚轮缩放）
    interactionCallback_ = vtkSmartPointer<vtkCallbackCommand>::New();
    interactionCallback_->SetCallback(ScaleBarController::OnInteractionEvent);
    interactionCallback_->SetClientData(this);
    interactor_->AddObserver(vtkCommand::MouseWheelForwardEvent, interactionCallback_);
    interactor_->AddObserver(vtkCommand::MouseWheelBackwardEvent, interactionCallback_);
}

// 创建比例尺的线条与文字 Actor，并添加到 Renderer 中
void ScaleBarController::CreateScaleBarActors()
{
    // 创建线段源
    lineSource_ = vtkSmartPointer<vtkLineSource>::New();

    // 创建 2D 映射器
    lineMapper_ = vtkSmartPointer<vtkPolyDataMapper2D>::New();
    lineMapper_->SetInputConnection(lineSource_->GetOutputPort());

    // 创建 2D Actor
    lineActor_ = vtkSmartPointer<vtkActor2D>::New();
    lineActor_->SetMapper(lineMapper_);
    lineActor_->GetProperty()->SetColor(1.0, 1.0, 1.0); // 白色线条
    lineActor_->GetPositionCoordinate()->SetCoordinateSystemToNormalizedDisplay();
    lineActor_->SetPosition(0.75, 0.05); // 屏幕右下角偏上

    // 创建比例尺文本 Actor
    scaleText_ = vtkSmartPointer<vtkTextActor>::New();
    scaleText_->GetTextProperty()->SetFontSize(16);
    scaleText_->GetTextProperty()->SetColor(1.0, 1.0, 1.0); // 白色字体
    scaleText_->GetPositionCoordinate()->SetCoordinateSystemToNormalizedDisplay();
    scaleText_->SetPosition(0.78, 0.01); // 线条下方

    // 添加到场景中
    renderer_->AddActor2D(lineActor_);
    renderer_->AddActor2D(scaleText_);
    // qDebug() << "[scaleText_:]" << scaleText_;
    // qDebug() << "[ScaleBar] 成功添加线条和文本到Renderer";

    UpdateScaleBar(); // 初始化一次比例尺
}

// 更新比例尺的显示内容（根据当前缩放）
void ScaleBarController::UpdateScaleBar()
{
    double scale = GetCurrentScaleFactor();    // world units / pixel
    double worldLength = scale * pixelLength_; // 当前比例尺实际长度（单位：米）

    worldLength = ComputeRoundedLength(worldLength); // 美化跳变长度
    if (worldLength < 0.1)
        return; // 小于 0.1 米不再更新

    double pixelPerWorld = 1.0 / scale;            // pixel/m
    double barPixel = worldLength * pixelPerWorld; // 实际比例尺长度（像素）

    // qDebug() << "[ScaleBar] 当前 scale:" << scale
    //          << " 计算 worldLength:" << worldLength
    //          << " 显示 barPixel:" << barPixel;

    // 更新线段位置
    lineSource_->SetPoint1(0, 0, 0);
    lineSource_->SetPoint2(barPixel, 0, 0); // 以像素长度为单位画横线
    lineSource_->Modified();

    // 更新显示文本
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << worldLength << " m";
    scaleText_->SetInput(oss.str().c_str());

    renderWindow_->Render();
}

// 场景清除重建后，重新添加比例尺图元
void ScaleBarController::ReAddToRenderer()
{
    renderer_->AddActor2D(lineActor_);
    renderer_->AddActor2D(scaleText_);
    // qDebug() << "[ScaleBar] 重新添加比例尺到 Renderer";
}

// 获取当前屏幕上 1 像素对应的 world 单位（米）
double ScaleBarController::GetCurrentScaleFactor()
{
    int *size = renderWindow_->GetSize();
    if (!size[0] || !size[1])
        return 1.0; // 防止除 0

    vtkCamera *cam = renderer_->GetActiveCamera();
    double viewAngle = cam->GetViewAngle();         // 透视角度
    double parallelScale = cam->GetParallelScale(); // 正交缩放

    double result;
    if (cam->GetParallelProjection())
    {
        result = (2.0 * parallelScale) / size[1]; // 正交模式计算方式
                                                  // qDebug() << "[ScaleBar] 使用 Parallel 投影" << result;
    }
    else
    {
        double viewHeight = 2.0 * tan(vtkMath::RadiansFromDegrees(viewAngle) / 2.0) * cam->GetDistance();
        result = viewHeight / size[1]; // 透视模式：根据视角和距离计算 view height
        // qDebug() << "[ScaleBar] 使用 Perspective 投影" << result
        //          << " CameraDistance:" << cam->GetDistance();
    }
    return result;
}

// 根据当前比例尺的 world 长度，计算美观跳变的长度
double ScaleBarController::ComputeRoundedLength(double length)
{
    if (length < 1.0)
        return std::round(length * 20.0) / 20.0; // 0.05 步长
    if (length < 10.0)
        return std::round(length * 2.0) / 2.0; // 0.5 步长
    if (length < 100.0)
        return std::round(length / 5.0) * 5.0; // 5 步长
    if (length < 1000.0)
        return std::round(length / 50.0) * 50.0; // 50 步长
    return std::round(length / 100.0) * 100.0;   // 100 步长
}

// 处理交互事件（滚轮缩放），检测是否超出缩放下限
void ScaleBarController::OnInteractionEvent(vtkObject *caller, unsigned long eid,
                                            void *clientdata, void *)
{
    auto self = static_cast<ScaleBarController *>(clientdata); // 当前控制器
    double scale = self->GetCurrentScaleFactor();
    double worldLength = scale * self->pixelLength_;
    double roundedLength = self->ComputeRoundedLength(worldLength);
    vtkCamera *cam = self->renderer_->GetActiveCamera();

    // 若继续放大会导致比例尺长度 < 0.1m，则强行回滚相机状态
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
        self->renderWindow_->Render(); // 强制刷新场景
        // qDebug() << "[ScaleBar] Zoom limit reached. Reverted to last valid state.";
        return;
    }

    // 当前比例尺合法，记录状态作为可回退点
    if (cam->GetParallelProjection())
    {
        self->lastValidParallelScale_ = cam->GetParallelScale();
    }
    else
    {
        self->lastValidCameraDistance_ = cam->GetDistance();
    }

    // 正常更新比例尺显示
    self->UpdateScaleBar();
}
