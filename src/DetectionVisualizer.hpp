#ifndef DETECTIONVISUALIZER_H
#define DETECTIONVISUALIZER_H


#include <iostream>
#include <vector>
#include <fstream>
#include <random>
#include <cctype>
#include <thread>
#include <mutex>
#include <atomic>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "imgui_stdlib.h"

#include <cxxopts.hpp>

#include <opencv2/opencv.hpp>

#define OPENCV
#include "yolo_v2_class.hpp"

#include "Window.hpp"

/**
 * Wrapper for YOLO detector that runs inference in separate thread
 */
class ThreadedDetector
{
public:
 /**
  * Creates and runs YOLO detector in new thread
  * @param cfgfile - path to the config file defining model
  * @param weightsfile - path to the file containing weights
  */
  ThreadedDetector(std::string& cfgfile, std::string& weightsfile);
  
  /**
   * Stops and destroys running thread 
   */
  ~ThreadedDetector();

  /**
   * Atomically sets frame.
   *
   * @param newframe new frame to detect
   */
  void setFrame(cv::Mat newframe);

  /**
   * Returns the frame (thread-safe).
   *
   * @return current frame
   */
  cv::Mat getFrame(void);

  /**
   * Updates detection results.
   *
   * @param detected found objects
   */
  void setDetectedObjects(std::vector<bbox_t> detected);

  /**
   * Returns detected objects
   *
   * @return detected objects
   */
  std::vector<bbox_t> getDetectedObjects();

  /**
   * Tells if the detection is running.
   *
   * @return true if detection is still running
   */
  bool isRunning();

  /**
   * Starts the detection thread
   */
  void startThread();
  
  std::atomic<double> inferencetime;

private:
  void detectLoop();

  std::mutex framemutex;
  std::mutex detectedobjectsmutex;

  Detector detector;
  std::thread thr;

  cv::Mat frame;
  std::vector<bbox_t> detectedobjects;
  std::atomic<bool> running = false;
};


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
  bool fullscreen = false;  

  int cameraID = -1;
  std::string videofilepath = "";
  cv::Size userspecifiedresolution{0, 0};
  
  std::string namesfile = "";
  std::string cfgfile = "";
  std::string weightsfile = "";
  
  std::vector<std::string> objectnames;
  std::vector<ImU32> objectcolors;

  GLuint textureID;
  ImVec2 frameratetextsize; 
  const ImU32 frameratecolor = ImColor(ImVec4(1.0f, 1.0f, 0.4f, 1.0f));
  const ImVec4 hiddenobjectcolor = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
  const float cornerroundingfactor = 10.0f;
  const float perimeterthickness = 8.0f;
  const float fontsize = 25.0f;
  const float filterfontsize = 15.0f;
  float threshold = 0.2f;

  const int seed = 12345;

  /**
   * Initiates video capture from specified camera along with setting proper resolution of frames to be read from capture object
   */
  void cameraInputInit(void);

  /**
   * Initiates video capture from specified video file along with scaling down frames to be read from capture object if they won't fit the screen
   */
  void videoInputInit(void);

  /**
   * Opens a file specified in namesfile variable and loads its contents into objectnames vector.
   */ 
  void openNamesFile(void);

  /**
   * Runs a loop which detects objects in each frame and displays result.
   */
  void detectDisplayLoop(void);

  void errorDisplayLoop(std::string errorstring);
};

#endif
