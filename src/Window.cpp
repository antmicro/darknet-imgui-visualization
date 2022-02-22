#include "Window.hpp"

void Window::resize(int width, int height)
{
  size.width = width;
  size.height = height;

  cv::Size heightcontrainted {static_cast<int>(contentaspectratio * size.height), size.height};
  cv::Size widthconstrainted {size.width, static_cast<int>(1.0f/contentaspectratio * size.width)};
  if (size.width < widthconstrainted.width && size.height < widthconstrainted.height)
  {
    viewportsize = widthconstrainted;
  }
  else
    if (size.width < heightcontrainted.width)
    {
      viewportsize = widthconstrainted;
    }
    else 
    {
      viewportsize = heightcontrainted;
    }
}

void Window::keyCB(int key, int action)
{
  if (key == GLFW_KEY_F && action == GLFW_PRESS)
  {
    setFullScreen(!isFullScreen());
  }
}

cv::Size Window::getContentSize(void)
{
  return contentsize;
}
void Window::updateContentSize(cv::Size newcontensize)
{
  contentsize = newcontensize; 
  contentaspectratio = (float) contentsize.width / contentsize.height;
  resize(contentsize.width, contentsize.height);
}

cv::Size Window::getSize(void)
{
  return size;
}

float Window::getContentAspectRatio(void)
{
  return contentaspectratio;
}


// Private functions:

// Init now not private:
int Window::init(std::string &name)
{
  if (!glfwInit())
  {
    glfwTerminate();
    perror("Failed to initiate GLFW");
    return EXIT_FAILURE;
  }
  GLFWmonitor **mons;
  int count;
  mons = glfwGetMonitors(&count);
  monitor = mons[0];

  const GLFWvidmode *mode = glfwGetVideoMode(monitor);
  glfwWindowHint(GLFW_AUTO_ICONIFY, GL_FALSE);

  window = glfwCreateWindow(size.width, size.height, (char*)name.c_str(), NULL, nullptr);  

  if (window == nullptr)
  {
    perror("Failed to create GLFW window");
    glfwTerminate();
    return EXIT_FAILURE;
  }
  glfwMakeContextCurrent(window);

  if (glewInit() != GLEW_OK)
  {
    perror("Failed to initiate GLEW");
    glfwTerminate();
    return EXIT_FAILURE;
  }

  glfwSetWindowUserPointer(window, this);
  glfwSetWindowSizeCallback(window, Window::callbackResize);
  glfwSetKeyCallback(window, Window::callbackKeyPress);

  glfwGetWindowPos(window, (int*)&position.width, (int*)&position.height);
  
  glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);

  glfwPollEvents();
  return EXIT_SUCCESS;
}

int Window::imguiInit(void)
{
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();

  ImGuiIO &io = ImGui::GetIO();
  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init(glslversion);

  ImGuiStyle * style = &ImGui::GetStyle();
  style->WindowPadding = ImVec2{0,0};

  return EXIT_SUCCESS;
}

void Window::callbackResize(GLFWwindow *window, int cx, int cy)
{
  void *ptr = glfwGetWindowUserPointer(window);
  if (Window *winPtr = static_cast<Window *>(ptr))
  {
    winPtr->resize(cx, cy);
  }
}

void Window::callbackKeyPress(GLFWwindow *window, int key, int scancode, int action, int mods)
{
  void *ptr = glfwGetWindowUserPointer(window);
  if (Window *winPtr = static_cast<Window *>(ptr))
  {
    winPtr->keyCB(key, action);
  }
}

bool Window::isFullScreen(void)
{
  return glfwGetWindowMonitor(window) != nullptr;
}

void Window::setFullScreen(bool fullscreen)
{
  if (isFullScreen() == fullscreen)
  {
    return;
  }
  if (fullscreen)
  {
    glfwGetWindowPos(window, (int*) &position.width, (int*) &position.height);
    glfwGetWindowSize(window, (int*) &size.width, (int*) &size.height);

    const GLFWvidmode *mode = glfwGetVideoMode(monitor);
    glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, 0);
  }
  else
  {
    glfwSetWindowMonitor(window, nullptr, position.width, position.height, size.width, size.height, 0);
    glfwShowWindow(window);
  }
  glfwGetFramebufferSize(window, (int*) &size.width, (int*) &size.height);
  resize(size.width, size.height);
}

Window::~Window()
{
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
                  
  glfwDestroyWindow(window);
  glfwTerminate();
}  
