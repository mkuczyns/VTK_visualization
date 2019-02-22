cmake_minimum_required(VERSION 2.8)

PROJECT(vtkVisualization)

find_package(VTK REQUIRED)
include(${VTK_USE_FILE})

add_executable(vtkVisualization MACOSX_BUNDLE vtkVisualization.cxx helperFunctions.cxx )

if(VTK_LIBRARIES)
  target_link_libraries(vtkVisualization ${VTK_LIBRARIES})
else()
  target_link_libraries(vtkVisualization vtkHybrid vtkWidgets)
endif()