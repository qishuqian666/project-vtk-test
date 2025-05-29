#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#include "MeasurementController.h"
#include <vtkPointPicker.h>
#include <vtkTextProperty.h>
#include <vtkProperty.h>
#include <vtkMath.h>
#include <cmath>
#include <vtkRenderWindow.h>
#include <vtkSphereSource.h>
#include <vtkProperty2D.h>
#include <QDebug>

MeasurementController::MeasurementController(vtkRenderer *renderer, vtkRenderWindowInteractor *interactor)
    : renderer_(renderer), interactor_(interactor)
{
    qDebug() << "[MeasurementController] Entering constructor";
    textActor_ = vtkSmartPointer<vtkTextActor>::New();
    textActor_->SetDisplayPosition(20, 20);
    textActor_->GetTextProperty()->SetFontSize(16);
    textActor_->GetTextProperty()->SetColor(0.0, 0.0, 0.0);           // 黑字
    textActor_->GetTextProperty()->SetBackgroundColor(1.0, 1.0, 1.0); // 白底
    textActor_->GetTextProperty()->SetBackgroundOpacity(0.8);         // 半透明背景
    textActor_->GetTextProperty()->SetFrame(1);                       // 开启边框
    textActor_->GetTextProperty()->SetFrameColor(1.0, 0.0, 0.0);      // 红框
    textActor_->SetVisibility(0);                                     // 初始隐藏
    renderer_->AddActor2D(textActor_);
}

void MeasurementController::setMode(MeasurementMode mode)
{
    // clearMeasurements();
    mode_ = mode;
    if (mode_ == MeasurementMode::None)
    {
        clearMeasurements();
    }
}

void MeasurementController::clearMeasurements()
{
    pickedPoints_.clear();
    for (auto &actor : pointMarkers_)
    {
        renderer_->RemoveActor(actor);
    }
    pointMarkers_.clear();
    textActor_->SetInput("");
    render();
}

void MeasurementController::onLeftButtonPressed()
{
    if (mode_ == MeasurementMode::None)
    {
        qDebug() << "[MeasurementController] Current mode is None. Click ignored.";
        return;
    }

    int x, y;
    interactor_->GetEventPosition(x, y);
    qDebug() << "[MeasurementController] Mouse clicked at: (" << x << "," << y << ")";

    auto picker = vtkSmartPointer<vtkPointPicker>::New();
    if (!picker->Pick(x, y, 0, renderer_))
    {
        qDebug() << "[MeasurementController] Point picking failed. No valid geometry hit.";
        return;
    }
    double pos[3];
    picker->GetPickPosition(pos);
    qDebug() << "[MeasurementController] Point picked at: ("
             << pos[0] << "," << pos[1] << "," << pos[2] << ")";

    // 清除逻辑根据当前点的数量判断
    switch (mode_)
    {
    case MeasurementMode::Point:
        pickedPoints_.clear();
        clearAllMarkers(); // 清除之前的可视化标记
        break;

    case MeasurementMode::Line:
        if (pickedPoints_.size() >= 2)
        {
            pickedPoints_.clear();
            clearAllMarkers();
            qDebug() << "[MeasurementController] Line mode - previous 2 points cleared.";
        }
        break;

    case MeasurementMode::Triangle:
        if (pickedPoints_.size() >= 3)
        {
            pickedPoints_.clear();
            clearAllMarkers();
            qDebug() << "[MeasurementController] Triangle mode - previous 3 points cleared.";
        }
        break;

    default:
        break;
    }

    // 添加当前点击的点
    pickedPoints_.emplace_back(std::array<double, 3>{pos[0], pos[1], pos[2]});
    qDebug() << "[MeasurementController] Picked point count:" << pickedPoints_.size();

    // 添加可视化标记
    addPointMarker(pos);

    // 当达到点数要求时，执行测量逻辑
    if ((mode_ == MeasurementMode::Line && pickedPoints_.size() == 2) ||
        (mode_ == MeasurementMode::Triangle && pickedPoints_.size() == 3))
    {
        qDebug() << "[MeasurementController] Required number of points reached. Updating measurement display.";
        updateMeasurementDisplay();
    }
    updateTextActor();
}

void MeasurementController::addPointMarker(const double pos[3])
{
    auto sphere = vtkSmartPointer<vtkSphereSource>::New();
    double center[3] = {pos[0], pos[1], pos[2]};
    sphere->SetCenter(center);
    sphere->SetRadius(1.0);

    auto mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputConnection(sphere->GetOutputPort());

    auto actor = vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(mapper);
    actor->GetProperty()->SetColor(1, 0, 0); // 红色球体
    pointMarkers_.push_back(actor);
    renderer_->AddActor(actor);

    render();
}

void MeasurementController::updateMeasurementDisplay()
{
    if (mode_ == MeasurementMode::Line && pickedPoints_.size() >= 2)
    {
        renderLine(pickedPoints_[0].data(), pickedPoints_[1].data());
        // textActor_->SetInput(formatDistanceInfo(pickedPoints_[0].data(), pickedPoints_[1].data()).toUtf8().data());
    }
    else if (mode_ == MeasurementMode::Triangle && pickedPoints_.size() >= 3)
    {
        renderTriangle(pickedPoints_[0].data(), pickedPoints_[1].data(), pickedPoints_[2].data());
        // textActor_->SetInput(formatTriangleInfo(pickedPoints_[0].data(), pickedPoints_[1].data(), pickedPoints_[2].data()).toUtf8().data());
    }

    render();
}

void MeasurementController::renderLine(const double p1[3], const double p2[3])
{
    qDebug() << "[MeasurementController] Entering renderLine function";
    auto line = vtkSmartPointer<vtkLineSource>::New();
    double pt1[3] = {p1[0], p1[1], p1[2]}; // 非 const 拷贝
    double pt2[3] = {p2[0], p2[1], p2[2]}; // 非 const 拷贝
    line->SetPoint1(pt1);
    line->SetPoint2(pt2);

    auto mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputConnection(line->GetOutputPort());

    // ⭐关键：启用拓扑偏移，让线绘制时偏移一点点，避免被遮挡（VTK 8.2 的做法）
    vtkMapper::SetResolveCoincidentTopologyToPolygonOffset();
    mapper->SetResolveCoincidentTopology(true);
    mapper->SetResolveCoincidentTopologyPolygonOffsetParameters(1.0, 1.0); // 偏移强度

    auto actor = vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(mapper);
    actor->GetProperty()->SetColor(1, 0, 0);
    actor->GetProperty()->SetLineWidth(2.0);
    actor->GetProperty()->SetColor(1, 0, 0);  // 红色
    actor->GetProperty()->SetLineWidth(2.0);  // 线宽
    actor->GetProperty()->SetLighting(false); // 可选，关闭光照影响
    // 可选：强制不透明，避免透明影响排序
    actor->GetProperty()->SetOpacity(1.0);

    pointMarkers_.push_back(actor);
    renderer_->AddActor(actor);
}

void MeasurementController::renderTriangle(const double p1[3], const double p2[3], const double p3[3])
{
    renderLine(p1, p2);
    renderLine(p2, p3);
    renderLine(p3, p1);
}

QString MeasurementController::formatDistanceInfo(const double *p1, const double *p2)
{
    double dx = p2[0] - p1[0];
    double dy = p2[1] - p1[1];
    double dz = p2[2] - p1[2];
    double dxy = std::sqrt(dx * dx + dy * dy);
    double dxyz = computeDistance(p1, p2);

    return QString("Distance: %1\nΔX: %2  ΔY: %3  ΔZ: %4\nΔXY: %5")
        .arg(dxyz)
        .arg(dx)
        .arg(dy)
        .arg(dz)
        .arg(dxy);
}

QString MeasurementController::formatTriangleInfo(const double *A, const double *B, const double *C)
{
    double area = computeArea(A, B, C);
    double ab = computeDistance(A, B);
    double bc = computeDistance(B, C);
    double ca = computeDistance(C, A);
    double angleA = computeAngleDeg(B, A, C);
    double angleB = computeAngleDeg(A, B, C);
    double angleC = computeAngleDeg(A, C, B);

    return QString("Area: %1\nAB: %2  BC: %3  CA: %4\n∠A: %5°  ∠B: %6°  ∠C: %7°")
        .arg(area)
        .arg(ab)
        .arg(bc)
        .arg(ca)
        .arg(angleA)
        .arg(angleB)
        .arg(angleC);
}

double MeasurementController::computeDistance(const double *p1, const double *p2)
{
    return std::sqrt(vtkMath::Distance2BetweenPoints(p1, p2));
}

double MeasurementController::computeArea(const double *A, const double *B, const double *C)
{
    double AB[3], AC[3], cross[3];
    vtkMath::Subtract(B, A, AB);
    vtkMath::Subtract(C, A, AC);
    vtkMath::Cross(AB, AC, cross);
    return 0.5 * vtkMath::Norm(cross);
}

double MeasurementController::computeAngleDeg(const double *A, const double *B, const double *C)
{
    double AB[3], CB[3];
    vtkMath::Subtract(A, B, AB);
    vtkMath::Subtract(C, B, CB);
    vtkMath::Normalize(AB);
    vtkMath::Normalize(CB);
    double dot = vtkMath::Dot(AB, CB);
    return std::acos(dot) * 180.0 / vtkMath::Pi();
}

void MeasurementController::render()
{
    if (renderer_ && renderer_->GetRenderWindow())
    {
        renderer_->GetRenderWindow()->Render();
    }
}

void MeasurementController::updateTextActor()
{
    if (mode_ == MeasurementMode::None)
    {
        textActor_->SetVisibility(0);
        return;
    }

    QString text;

    if (pickedPoints_.size() == 1)
    {
        const auto &p = pickedPoints_[0];
        text = QString("Point@Local\nX1:%1    Y1:%2    Z1:%3")
                   .arg(p[0], 0, 'f', 6)
                   .arg(p[1], 0, 'f', 6)
                   .arg(p[2], 0, 'f', 6);
    }
    else if (pickedPoints_.size() == 2)
    {
        const auto &p1 = pickedPoints_[0];
        const auto &p2 = pickedPoints_[1];

        double dx = p2[0] - p1[0];
        double dy = p2[1] - p1[1];
        double dz = p2[2] - p1[2];
        double dxy = std::sqrt(dx * dx + dy * dy);
        double dxz = std::sqrt(dx * dx + dz * dz);
        double dyz = std::sqrt(dy * dy + dz * dz);
        double dist = std::sqrt(dx * dx + dy * dy + dz * dz);

        text = QString("Distance: %1\n"
                       "△X:%2   △Y:%3   △Z:%4\n"
                       "△XY:%5  △XZ:%6  △YZ:%7")
                   .arg(QString::number(dist, 'f', 6).rightJustified(12, ' '))
                   .arg(QString::number(dx, 'f', 6).rightJustified(12, ' '))
                   .arg(QString::number(dy, 'f', 6).rightJustified(12, ' '))
                   .arg(QString::number(dz, 'f', 6).rightJustified(12, ' '))
                   .arg(QString::number(dxy, 'f', 6).rightJustified(12, ' '))
                   .arg(QString::number(dxz, 'f', 6).rightJustified(12, ' '))
                   .arg(QString::number(dyz, 'f', 6).rightJustified(12, ' '));
    }
    else if (pickedPoints_.size() == 3)
    {
        const auto &A = pickedPoints_[0];
        const auto &B = pickedPoints_[1];
        const auto &C = pickedPoints_[2];

        // 向量 AB, BC, CA
        double AB[3] = {B[0] - A[0], B[1] - A[1], B[2] - A[2]};
        double BC[3] = {C[0] - B[0], C[1] - B[1], C[2] - B[2]};
        double CA[3] = {A[0] - C[0], A[1] - C[1], A[2] - C[2]};

        // 边长
        double lenAB = std::sqrt(AB[0] * AB[0] + AB[1] * AB[1] + AB[2] * AB[2]);
        double lenBC = std::sqrt(BC[0] * BC[0] + BC[1] * BC[1] + BC[2] * BC[2]);
        double lenCA = std::sqrt(CA[0] * CA[0] + CA[1] * CA[1] + CA[2] * CA[2]);

        // 向量 AC（用于法线）
        double AC[3] = {C[0] - A[0], C[1] - A[1], C[2] - A[2]};
        double N[3] = {
            AB[1] * AC[2] - AB[2] * AC[1],
            AB[2] * AC[0] - AB[0] * AC[2],
            AB[0] * AC[1] - AB[1] * AC[0]};
        double normN = std::sqrt(N[0] * N[0] + N[1] * N[1] + N[2] * N[2]);
        double area = 0.5 * normN;

        // 单位法向量
        if (normN > 1e-6)
        {
            N[0] /= normN;
            N[1] /= normN;
            N[2] /= normN;
        }

        // 角度（夹角）：使用余弦定理
        auto angle = [](const double *u, const double *v) -> double
        {
            double dot = u[0] * v[0] + u[1] * v[1] + u[2] * v[2];
            double lenU = std::sqrt(u[0] * u[0] + u[1] * u[1] + u[2] * u[2]);
            double lenV = std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
            double cosTheta = dot / (lenU * lenV);
            cosTheta = std::clamp(cosTheta, -1.0, 1.0);
            return std::acos(cosTheta) * 180.0 / M_PI;
        };

        double angleA = angle(CA, AB); // ∠CAB
        double angleB = angle(AB, BC); // ∠ABC
        double angleC = angle(BC, CA); // ∠BCA

        text = QString("Area:%1\n")
                   .arg(QString::number(area, 'f', 6).rightJustified(12, ' '));

        text += QString("AB:%1  BC:%2  CA:%3\n")
                    .arg(QString::number(lenAB, 'f', 6).rightJustified(12, ' '))
                    .arg(QString::number(lenBC, 'f', 6).rightJustified(12, ' '))
                    .arg(QString::number(lenCA, 'f', 6).rightJustified(12, ' '));

        text += QString("angle.A:%1°  angle.B:%2°  angle.C:%3°\n")
                    .arg(QString::number(angleA, 'f', 3).rightJustified(8, ' '))
                    .arg(QString::number(angleB, 'f', 3).rightJustified(8, ' '))
                    .arg(QString::number(angleC, 'f', 3).rightJustified(8, ' '));

        text += QString("Nx:%1  Ny:%2  Nz:%3")
                    .arg(QString::number(N[0], 'f', 6).rightJustified(12, ' '))
                    .arg(QString::number(N[1], 'f', 6).rightJustified(12, ' '))
                    .arg(QString::number(N[2], 'f', 6).rightJustified(12, ' '));
    }
    else
    {
        text = ""; // 超过3个点暂不支持
    }

    textActor_->SetInput(text.toUtf8().data());
    textActor_->SetDisplayPosition(20, 20);
    textActor_->SetVisibility(!text.isEmpty());
}

void MeasurementController::clearAllMarkers()
{
    for (auto actor : pointMarkers_)
    {
        renderer_->RemoveActor(actor);
    }
    pointMarkers_.clear();

    if (textActor_)
    {
        textActor_->SetVisibility(0); // 不删除，只隐藏
    }

    interactor_->GetRenderWindow()->Render();
}

void MeasurementController::ReAddActorsToRenderer()
{
    if (textActor_ && renderer_)
    {
        renderer_->AddActor2D(textActor_);
    }
}
