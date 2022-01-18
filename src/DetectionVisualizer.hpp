#ifndef DETECTIONVISUALIZER_H
#define DETECTIONVISUALIZER_H


#include <iostream>
#include <vector>
#include <fstream>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <cxxopts.hpp>

#include <opencv2/opencv.hpp>

#define OPENCV
#include "yolo_v2_class.hpp"

#include "Window.hpp"

class DetectionVisualizer
{
public:
  /** 
   * Parses command line arguments and sets values of variables like
   * cameraID, videofilepath, userspecifiedresolution, namesfile, cfgfile, weightsfile
   * @param argc, argv are main function parameters
   * @return EXIT_SUCCESS if executed successfully
   */
  int parseArguments(int argc, char* argv[]);

  /** 
   * Runs the detection and visualization via interaction with Window class
   * @param argc, argv are main function parameters
   * @return EXIT_SUCCESS if executed successfully
   */
  int run(void);
    
private:
  std::string windowname = "Darknet YOLO demo";

  Window mainwindow;
  cv::VideoCapture capture;
  
  int cameraID = -1;
  std::string videofilepath = "";
  cv::Size userspecifiedresolution{0, 0};
  
  std::string namesfile = "";
  std::string cfgfile = "";
  std::string weightsfile = "";
  
  std::vector<std::string> objectnames;

  ImVec2 frameratetextsize; 
  const ImU32 selectioncolor = ImColor(ImVec4(1.0f, 1.0f, 0.4f, 1.0f));
  const float cornerroundingfactor = 10.0f;
  const float perimeterthickness = 8.0f;
  
  /**
   * Initiates video capture from specified camera along with setting proper resolution of frames to be read from capture object
   * @return EXIT_SUCCESS if executed successfully
   */
  int cameraInputInit(void);

  /**
   * Initiates video capture from specified video file along with scaling down frames to be read from capture object if they won't fit the screen
   * @return EXIT_SUCCESS if executed successfully
   */
  int videoInputInit(void);

  /**
   * Opens a file specified in namesfile variable and loads its contents into objectnames vector.
   * @return EXIT_SUCCESS if executed successfully
   */ 
  int openNamesFile(void);

  /**
   * Runs a loop which detects objects in each frame and displays result.
   * @return EXIT_SUCCESS if executed successfully
   */
  int detectDisplayLoop(void);
};

#endif
