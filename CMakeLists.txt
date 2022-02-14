cmake_minimum_required (VERSION 3.10)
project (darknet-demo)

set (CMAKE_CXX_STANDARD 17)
set (OpenGL_GL_PREFERENCE GLVND)

find_package(OpenGL REQUIRED)
find_package(OpenCV REQUIRED)
find_package(glfw3 REQUIRED)

if (NOT DEFINED CACHE{LIBDARKNET_PATH})
  message( FATAL_ERROR "Please set variable LIBDARKNET_PATH with -DLIBDARKNET_PATH=<path-to-so-file>." )
endif()

add_library( darknet SHARED IMPORTED )
set_target_properties( darknet PROPERTIES IMPORTED_LOCATION ${LIBDARKNET_PATH} )

# Compile third-party dependencies 

include_directories(
  ${GLEW_INCLUDE_DIRS}
  ${OpenCV_INCLUDE_DIRS}
  ${CMAKE_CURRENT_SOURCE_DIR}/third-party
  ${CMAKE_CURRENT_SOURCE_DIR}/third-party/imgui
  ${CMAKE_CURRENT_SOURCE_DIR}/third-party/imgui/backends
  ${CMAKE_CURRENT_SOURCE_DIR}/third-party/imgui/misc/cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/third-party/include/
)

add_executable(${PROJECT_NAME}
  src/main.cpp
  src/Window.cpp
  src/DetectionVisualizer.cpp
  third-party/imgui/imgui.cpp
  third-party/imgui/imgui_tables.cpp
  third-party/imgui/imgui_widgets.cpp
  third-party/imgui/imgui_draw.cpp
  third-party/imgui/backends/imgui_impl_glfw.cpp
  third-party/imgui/backends/imgui_impl_opengl3.cpp
  third-party/imgui/misc/cpp/imgui_stdlib.cpp
)

target_link_libraries(${PROJECT_NAME}
  darknet
  glfw
  GLEW
  OpenGL::GL
  ${OpenCV_LIBS}
  ${CMAKE_DL_LIBS}
)

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION "bin"
)

install(FILES data/coco.names data/yolov4.cfg data/yolov4.weights
  DESTINATION share/${PROJECT_NAME}
)
