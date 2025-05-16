#include "BoxClipperController.h"

#include <vtkBoxWidget.h>
#include <vtkPlanes.h>
#include <vtkPolyData.h>
#include <vtkClipPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkCommand.h>
#include <vtkProperty.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>

BoxClipperController::BoxClipperController(vtkRenderWindowInteractor *interactor, vtkRenderer *renderer)
    : interactor(interactor), renderer(renderer)
{
    originalActor = nullptr;
    // 裁剪用平面集（由 BoxWidget 生成）
    clipPlanes = vtkSmartPointer<vtkPlanes>::New();
    // 裁剪器：使用 box widget 的平面集进行裁剪
    clipper = vtkSmartPointer<vtkClipPolyData>::New();
    // 映射器连接裁剪器的输出
    vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputConnection(clipper->GetOutputPort());
    // 显示裁剪后模型的 actor
    clippedActor = vtkSmartPointer<vtkActor>::New();
    clippedActor->SetMapper(mapper);

    // 初始化 box widget 控件
    boxWidget = vtkSmartPointer<vtkBoxWidget>::New();
    boxWidget->SetInteractor(interactor); // 控制器绑定
    boxWidget->SetPlaceFactor(1.0);       // 控制缩放比例
    boxWidget->HandlesOn();               // 显示操作手柄

    // 添加裁剪更新事件监听器
    vtkSmartPointer<vtkCallbackCommand> callback = vtkSmartPointer<vtkCallbackCommand>::New();
    callback->SetClientData(this); // 把当前类传入回调中
    callback->SetCallback([](vtkObject *caller, unsigned long, void *clientData, void *callData)
                          {
                              auto self = static_cast<BoxClipperController *>(clientData);
                              self->UpdateClipping(); // 当 box widget 改变时更新裁剪
                          });

    boxWidget->AddObserver(vtkCommand::InteractionEvent, callback); // 绑定观察者
}
// 设置原始输入数据与原始 actor（不拥有生命周期）
void BoxClipperController::SetInputDataAndReplaceOriginal(vtkSmartPointer<vtkPolyData> input, vtkActor *original)
{
    originalActor = original; // 不用智能指针，不控制生命周期
    inputData = input;

    clipper->SetInputData(inputData);
    boxWidget->SetInputData(inputData);
    boxWidget->PlaceWidget();

    UpdateClipping(); // 更新 planes 和裁剪结果
}

void BoxClipperController::SetEnabled(bool enabled)
{
    boxWidget->SetEnabled(enabled ? 1 : 0); // 开启或关闭 box 控件
    if (enabled)
    {
        if (originalActor && renderer->HasViewProp(originalActor))
        {
            renderer->RemoveActor(originalActor); // 移除原始 actor
        }
        renderer->AddActor(clippedActor); // 启用裁剪后显示 actor
    }
    else
    {
        renderer->RemoveActor(clippedActor);
        if (originalActor)
        {
            renderer->AddActor(originalActor);
        }
    }
    renderer->GetRenderWindow()->Render();
}

void BoxClipperController::UpdateClipping()
{
    vtkSmartPointer<vtkPlanes> planes = vtkSmartPointer<vtkPlanes>::New();
    boxWidget->GetPlanes(planes);     // 获取当前 box widget 对应的平面
    clipper->SetClipFunction(planes); // 使用这些平面裁剪
    clipper->InsideOutOn();           // 保留 box 内部数据
    clipper->Update();                // 更新裁剪结果
}

vtkSmartPointer<vtkActor> BoxClipperController::GetClippedActor()
{
    return clippedActor;
}
