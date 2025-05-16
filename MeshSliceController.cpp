#include "MeshSliceController.h"
#include <vtkProperty.h>
#include <vtkBoundingBox.h>
#include <iostream> // 添加标准输出

MeshSliceController::MeshSliceController(vtkSmartPointer<vtkRenderer> renderer)
    : renderer_(renderer)
{
    std::cout << "[MeshSliceController] Constructor called." << std::endl;

    slicePlane_ = vtkSmartPointer<vtkPlane>::New();
    cutter_ = vtkSmartPointer<vtkCutter>::New();
    cutter_->SetCutFunction(slicePlane_);

    sliceMapper_ = vtkSmartPointer<vtkPolyDataMapper>::New();
    sliceMapper_->SetInputConnection(cutter_->GetOutputPort());

    sliceActor_ = vtkSmartPointer<vtkActor>::New();
    sliceActor_->SetMapper(sliceMapper_);
    sliceActor_->GetProperty()->SetColor(1.0, 0.0, 0.0); // 红色切面
    sliceActor_->GetProperty()->SetLineWidth(2.0);
    sliceActor_->GetProperty()->SetOpacity(1.0);
}

void MeshSliceController::ShowSlice(SliceDirection direction)
{
    if (!polyData_)
    {
        std::cerr << "[MeshSliceController] polyData_ is null. Cannot show slice." << std::endl;
        return;
    }
    if (originalActor_)
    {
        std::cout << "[ShowSlice] Hiding original actor." << std::endl;
        originalActor_->VisibilityOff();
    }

    std::cout << "[MeshSliceController] ShowSlice called." << std::endl;
    double bounds[6];
    polyData_->GetBounds(bounds);

    double center[3] = {
        (bounds[0] + bounds[1]) / 2.0,
        (bounds[2] + bounds[3]) / 2.0,
        (bounds[4] + bounds[5]) / 2.0};

    std::cout << "PolyData bounds: ["
              << bounds[0] << ", " << bounds[1] << ", "
              << bounds[2] << ", " << bounds[3] << ", "
              << bounds[4] << ", " << bounds[5] << "]" << std::endl;

    switch (direction)
    {
    case SLICE_X:
        std::cout << "Slice along X-axis." << std::endl;
        slicePlane_->SetOrigin(center[0], center[1], center[2]);
        slicePlane_->SetNormal(1.0, 0.0, 0.0);
        break;
    case SLICE_Y:
        std::cout << "Slice along Y-axis." << std::endl;
        slicePlane_->SetOrigin(center[0], center[1], center[2]);
        slicePlane_->SetNormal(0.0, 1.0, 0.0);
        break;
    case SLICE_Z:
        std::cout << "Slice along Z-axis." << std::endl;
        slicePlane_->SetOrigin(center[0], center[1], center[2]);
        slicePlane_->SetNormal(0.0, 0.0, 1.0);
        break;
    default:
        std::cerr << "Unknown slice direction!" << std::endl;
        return;
    }
    std::cout << "Origin: (" << center[0] << ", " << center[1] << ", " << center[2] << ")" << std::endl;

    cutter_->Update();
    vtkSmartPointer<vtkPolyData> sliceOutput = cutter_->GetOutput();
    std::cout << "Number of points in slice: " << sliceOutput->GetNumberOfPoints() << std::endl;
    std::cout << "Number of cells in slice: " << sliceOutput->GetNumberOfCells() << std::endl;

    if (!renderer_->HasViewProp(sliceActor_))
    {
        std::cout << "[MeshSliceController] Adding slice actor to renderer." << std::endl;
        renderer_->AddActor(sliceActor_);
    }
    else
    {
        std::cout << "[ShowSlice] Slice actor already exists in renderer." << std::endl;
    }

    sliceMapper_->SetInputConnection(cutter_->GetOutputPort());
    sliceMapper_->Update();
    renderer_->Render();
}

void MeshSliceController::HideSlice()
{
    if (originalActor_)
    {
        std::cout << "[HideSlice] Restoring original actor visibility." << std::endl;
        originalActor_->VisibilityOn();
    }
    if (renderer_->HasViewProp(sliceActor_))
    {
        std::cout << "[HideSlice] Removing slice actor." << std::endl;
        renderer_->RemoveActor(sliceActor_);
        renderer_->Render();
    }
}

void MeshSliceController::UpdatePolyData(vtkSmartPointer<vtkPolyData> polyData)
{
    std::cout << "[UpdatePolyData] Updating slice polyData, number of points: "
              << (polyData ? polyData->GetNumberOfPoints() : 0) << std::endl;
    polyData_ = polyData;
    cutter_->SetInputData(polyData_); // 更新切割输入
    cutter_->Update();
}

void MeshSliceController::SetOriginalActor(vtkSmartPointer<vtkActor> actor)
{
    originalActor_ = actor;
}
