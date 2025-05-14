cmake_minimum_required(VERSION 3.14)
project(MyApp)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON) 

# Qt
set(Qt5_DIR "C:/SoftWare_install/Qt/Qt5.12.12/5.12.12/msvc2017_64/lib/cmake/Qt5")
find_package(Qt5 REQUIRED COMPONENTS Widgets)

# VTK
find_package(VTK REQUIRED PATHS "D:/mytestworkspace/vtktest/vtk-8.2.0/lib/cmake/vtk-8.2")
if(NOT VTK_FOUND)
    message(FATAL_ERROR "VTK not found!")
endif()
include(${VTK_USE_FILE})

# Print VTK libraries for debugging
message(STATUS "VTK_LIBRARIES: ${VTK_LIBRARIES}")

# 添加源文件和头文件
set(SOURCES
    main.cpp
    ThreeDViewWidget.cpp
    ScaleBarController.cpp
    # 其他源文件
)

set(HEADERS
    ThreeDViewWidget.h
    ScaleBarController.h
    # 其他头文件
)

# 添加可执行文件，同时包含源文件和头文件
add_executable(MyApp ${SOURCES} ${HEADERS})

target_link_libraries(MyApp
    Qt5::Widgets
    ${VTK_LIBRARIES}
)
