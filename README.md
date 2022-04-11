# Object detection visualization with ImGui

Copyright (c) 2020-2022 [Antmicro](https://www.antmicro.com)

Fast, OpenGL-based [darket](https://github.com/AlexeyAB/darknet) YOLO object detection visualization with [Dear ImGui](https://github.com/ocornut/imgui).

## Project dependencies

* Darknet shared library - follow the build instructions for the [darknet](https://github.com/AlexeyAB/darknet) framework - this project requires building the `libdarknet.so` shared library
* OpenCV 4.5.2
* GLFW 3
* GLEW
* GLVND (recommended)
* Git LFS

## Building the project

* Clone the repository:
```
git clone --recursive https://github.com/antmicro/darknet-imgui-visualization
git lfs pull
```
* Run CMake (`LIBDARKNET_PATH` should point to the `libdarknet.so` file in the filesystem, `CMAKE_CXX_FLAGS` should add the `include` directory from the `darknet` repository to the list of include directories):
```
mkdir build
cd build
cmake -DLIBDARKNET_PATH=<path-to-libdarknet.so> -DCMAKE_CXX_FLAGS="-I<path-to-darknet-include-dir>" ..
```
* Build the project:
```
make -j`nproc`
```

After a successful build, the `darknet-imgui-visualization` binary should appear in the `build` directory.

## Running the application

To run the application for a video file with the YOLOv4 object detection model, run (in the project's main directory):
```
./build/darknet-imgui-visualization --video-file <path-to-mp4-file> --names-file ./data/coco.names --cfg-file ./data/yolov4.cfg --weights-file ./data/yolov4.weights --fullscreen
```

To run the application for the camera stream, run:
```
./build/darknet-imgui-visualization --camera-id 0 --names-file ./data/coco.names --cfg-file ./data/yolov4.cfg --weights-file ./data/yolov4.weights --fullscreen
```
The `--camera-id` is the ID of the camera in the system.

For more options and flags, check:
```
./build/darknet-imgui-visualization -h
```
