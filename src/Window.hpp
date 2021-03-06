#ifndef WINDOW_H
#define WINDOW_H

#include <array>
#include <string>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <opencv2/opencv.hpp>

/**
 * Class to operate on GLFWwindow object
 */
class Window
{
  public:
    GLFWwindow* window = nullptr;

    cv::Size viewportsize {0, 0};
    cv::Size size {640, 480};
    cv::Size position {0, 0};
    
    /**
     * Destroys the ImGUI context and GLFW objects
     */
    ~Window();

    /**
     * Initiates existance of window
     * @param name is a title of window to be created
     * @return EXIT_SUCCESS if executed successfully
     */
    int init(std::string &name);
    /**
     * Initiates imgui support in OpenGL
     * @return EXIT_SUCCESS if executed successfully
     */
    int imguiInit(void);

    cv::Size getContentSize(void);
    void updateContentSize(cv::Size newcontensize);

    cv::Size getSize(void);
    float getContentAspectRatio(void);

    void setFullScreen(bool fullscreen);
  
  private:
    GLFWmonitor *monitor = nullptr;
    const char* glslversion = "#version 130";

    cv::Size contentsize {0, 0};
    float contentaspectratio;

    bool isFullScreen(void);

    virtual void resize(int width, int height);
    virtual void keyCB(int key, int action);

    static void callbackResize(GLFWwindow *window, int width, int height);
    static void callbackKeyPress(GLFWwindow *window, int key, int scancode, int action, int mods);
};


#endif
