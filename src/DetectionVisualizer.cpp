#include "DetectionVisualizer.hpp"

int DetectionVisualizer::parseArguments(int argc, char* argv[])
{
  try
  {
    cxxopts::Options options(argv[0], "Video feed visualizer");

    options.add_options()
    ("h,help", "Prints help")
    ("v,video-file", "path to the video file, \e[1mrequired*\e[0m", cxxopts::value<std::string>(videofilepath))
    ("i,camera-id", "number of camera in the system, \e[1mrequired*\e[0m", cxxopts::value<int>(cameraID))
    ("width", "sets input resolution width", cxxopts::value<int>(userspecifiedresolution.width))
    ("height", "sets input resolution height", cxxopts::value<int>(userspecifiedresolution.height))
    ("n,names-file", "path to the file with names of detected objects, \e[1mrequired\e[0m", cxxopts::value<std::string>(namesfile))
    ("c,cfg-file", "path to the file with configuration, \e[1mrequired\e[0m", cxxopts::value<std::string>(cfgfile))
    ("w,weights-file", "path to the file with weights, \e[1mrequired\e[0m", cxxopts::value<std::string>(weightsfile));

    auto result = options.parse(argc, argv);
    
    if (result.count("help")) 
    {
      std::cout << options.help({""}) << std::endl;
      std::cout << "\e[1m*\e[0m - precisely one of two starred arguments is required" << std::endl;
      std::cout << "During application runtime F key toggles between fullscreen and window mode." << std::endl << std::endl;
      return EXIT_FAILURE;
    }
  }
  catch (const cxxopts::OptionException& e)
  {
    std::cout << "error parsing options: " << e.what() << std::endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

int DetectionVisualizer::openNamesFile()
{
  std::vector<std::string> lines;

  if (namesfile == "")
  {
    std::cout << "Please supply a file with names for detected objects." << std::endl;
    std::cout << "Use --help to print usage." << std::endl << std::endl;
    return EXIT_FAILURE;
  }
  std::ifstream file(namesfile);
  
  std::string line;
  while(getline(file, line))
    objectnames.push_back(line);
  return EXIT_SUCCESS;
}

int DetectionVisualizer::cameraInputInit()
{
  int apiID = cv::CAP_ANY;
  capture.open("/dev/video" + std::to_string(cameraID));

  if(!capture.isOpened()) {
    perror("Failed to initiate camera capture");
    return EXIT_FAILURE;
  }

  cv::Size originalresolution{
    static_cast<int>(capture.get(cv::CAP_PROP_FRAME_WIDTH)),
      static_cast<int>(capture.get(cv::CAP_PROP_FRAME_HEIGHT))
  };
  float originalaspectratio = (float)originalresolution.width / originalresolution.height;

  if (userspecifiedresolution.width != 0 || userspecifiedresolution.height != 0)
  {
    cv::Size capturedframe = userspecifiedresolution;
    if (userspecifiedresolution.height == 0)
    {
      capturedframe.height = userspecifiedresolution.width * 1/originalaspectratio;
    }
    if (userspecifiedresolution.width == 0)
    {
      capturedframe.width = userspecifiedresolution.height * originalaspectratio;
    }

    std::cout << "\nAsking the camera for " << capturedframe.width << " x " << capturedframe.height << " stream... ";

    if(!capture.set(cv::CAP_PROP_FRAME_HEIGHT, capturedframe.height))
    {
      perror("Failed to set height of frame to be captured");
      return EXIT_FAILURE;
    }
    if(!capture.set(cv::CAP_PROP_FRAME_WIDTH, capturedframe.width))
    {
      perror("Failed to set width of frame to be captured");
      return EXIT_FAILURE;
    }
  }

  originalresolution.width = capture.get(cv::CAP_PROP_FRAME_WIDTH);
  originalresolution.height = capture.get(cv::CAP_PROP_FRAME_HEIGHT);
  originalaspectratio = (float)originalresolution.width / originalresolution.height;

  mainwindow.setContentSize(originalresolution);
  mainwindow.calculateContentAspectRatio();
  mainwindow.setSize(originalresolution);

  std::cout << "Got " << originalresolution.width << " x " << originalresolution.height << "." << std::endl << std::endl;
  return EXIT_SUCCESS;
}

int DetectionVisualizer::videoInputInit()
{
  capture.open("filesrc location=" + videofilepath + " ! decodebin ! videoconvert ! appsink" , cv::CAP_GSTREAMER);
  if(!capture.isOpened()) {
    perror("Failed to initiate video file capture");   
    return EXIT_FAILURE;
  }

  cv::Size designatedresolution{
    static_cast<int>(capture.get(cv::CAP_PROP_FRAME_WIDTH)),
    static_cast<int>(capture.get(cv::CAP_PROP_FRAME_HEIGHT))
  };

  mainwindow.setContentSize(designatedresolution);
  mainwindow.calculateContentAspectRatio();

  if (userspecifiedresolution.width != 0 && userspecifiedresolution.height != 0)
  {
    designatedresolution = userspecifiedresolution;
  }
  else if (userspecifiedresolution.width != 0 && userspecifiedresolution.height == 0)
  {
    designatedresolution = {
      userspecifiedresolution.width,
      static_cast<int>((float)userspecifiedresolution.width * 1.0f/mainwindow.getContentAspectRatio())
    };
  }
  else if (userspecifiedresolution.height != 0 && userspecifiedresolution.width == 0)
  {
    designatedresolution = {
      static_cast<int>((float)userspecifiedresolution.height * mainwindow.getContentAspectRatio()),
      userspecifiedresolution.height
    };
  }
  mainwindow.setSize(designatedresolution);
  
  return EXIT_SUCCESS;
}

int DetectionVisualizer::detectDisplayLoop()
{
  Detector detector(cfgfile, weightsfile);
  cv::Mat frame;

  ImGuiWindowFlags windowflags = 0;
  windowflags |= ImGuiWindowFlags_NoTitleBar;
  windowflags |= ImGuiWindowFlags_NoResize;
  windowflags |= ImGuiWindowFlags_NoMove;
  windowflags |= ImGuiWindowFlags_NoScrollbar;
  windowflags |= ImGuiWindowFlags_NoScrollWithMouse;
  windowflags |= ImGuiWindowFlags_NoCollapse;
  windowflags |= ImGuiWindowFlags_NoDecoration;
  windowflags |= ImGuiWindowFlags_NoNav;
  windowflags |= ImGuiWindowFlags_NoBackground;
  windowflags |= ImGuiWindowFlags_NoInputs;

  char frameratetext[20];
  double overallstarttimestamp = glfwGetTime();
  double detectionstarttimestamp = 0.0;
  double finishtimestamp = glfwGetTime();

  while(glfwWindowShouldClose(mainwindow.window) == 0 && glfwGetKey(mainwindow.window, GLFW_KEY_ESCAPE) != GLFW_PRESS)
  {

    overallstarttimestamp = glfwGetTime();

    sprintf(frameratetext, 
        "%.1f / %.1f fps",
        1000.0/double(detectionstarttimestamp - finishtimestamp)/1000.0, 
        1000.0/double(overallstarttimestamp - finishtimestamp)/1000.0);
    finishtimestamp = glfwGetTime();

    capture.read(frame);
    if(frame.empty())
    {
      perror("Failed to read next frame from video capture object");
      break;
    }

    cv::resize(frame, frame, cv::Size(mainwindow.viewportsize.width, mainwindow.viewportsize.height), cv::INTER_LINEAR);
    cvtColor(frame, frame, cv::COLOR_BGR2RGBA);

    std::vector<bbox_t> detected_objects;
    detected_objects = detector.detect(frame);

    detectionstarttimestamp = glfwGetTime();

    glfwPollEvents();
    glClearColor(0.0f, 0.0f, 0.4f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    cv::Size imguiwindowposition {
      mainwindow.size.width/2 - mainwindow.viewportsize.width/2,
      mainwindow.size.height/2 - mainwindow.viewportsize.height/2};

    ImGui::SetNextWindowPos(ImVec2(imguiwindowposition.width, imguiwindowposition.height));
    ImGui::SetNextWindowSize(ImVec2(mainwindow.viewportsize.width, mainwindow.viewportsize.height));
    if (!ImGui::Begin("stream window", NULL, windowflags))
    {
      perror("Failed to initiate ImGui");
      ImGui::End();
      return EXIT_SUCCESS;
    }

    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, frame.cols, frame.rows, 0, GL_RGBA, GL_UNSIGNED_BYTE, frame.data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

    ImGui::Image(reinterpret_cast<void*>(static_cast<intptr_t>(textureID)), ImVec2(frame.cols, frame.rows));
    ImDrawList* drawlist = ImGui::GetWindowDrawList();


    for (bbox_t object : detected_objects) {
      ImVec2 upperleftcorner(
          object.x + imguiwindowposition.width,
          object.y + imguiwindowposition.height);
      ImVec2 lowerrightcorner(
          upperleftcorner.x + object.w,
          upperleftcorner.y + object.h);
      std::string text = objectnames[object.obj_id] + " (" + std::to_string(100 * object.prob) + "%)";

      drawlist -> AddRect(
          upperleftcorner,
          lowerrightcorner,
          selectioncolor,
          cornerroundingfactor,
          0,
          perimeterthickness); 

      ImVec2 textposition(
          upperleftcorner.x + cornerroundingfactor,
          upperleftcorner.y + cornerroundingfactor);
      drawlist -> AddText(
          textposition,
          selectioncolor,
          text.c_str()
          );
    }

    drawlist -> AddText(
        ImVec2 (
          imguiwindowposition.width + mainwindow.viewportsize.width - ImGui::CalcTextSize(frameratetext).x - cornerroundingfactor,
          imguiwindowposition.height + mainwindow.viewportsize.height - ImGui::CalcTextSize(frameratetext).y - cornerroundingfactor),
        selectioncolor,
        frameratetext
        );
    
    ImGui::End();
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(mainwindow.window);
  }
  return EXIT_SUCCESS;
}

int DetectionVisualizer::run()
{
  if (openNamesFile() != EXIT_SUCCESS)
  {
    return EXIT_FAILURE;
  }
    
  if (cameraID >= 0)
  {
    if ("" == videofilepath)
    {
      std::cout << "openning camera no. " << cameraID << std::endl;
      if (cameraInputInit() != EXIT_SUCCESS)
      {
        return EXIT_FAILURE;
      }
    }
    else
    {
      std::cout << "Too many parameters\n";
      std::cout << "Use --help to print usage." << std::endl << std::endl;
      return EXIT_SUCCESS;
    }
  }
  else
  {
    if ("" == videofilepath)
    {
      std::cout << "Correct video source parameters not specified\n";
      std::cout << "Use --help to print usage." << std::endl << std::endl;
      return EXIT_SUCCESS;
    }
    else
    {
      std::cout << "openning videofile: " << videofilepath << std::endl;
      if (videoInputInit() != EXIT_SUCCESS)
      {
        return EXIT_FAILURE;
      }
    }
  }
  
  if (mainwindow.init(windowname) != EXIT_SUCCESS)
  {
   return EXIT_FAILURE;
  }
  if (mainwindow.imguiInit() != EXIT_SUCCESS)
  {
   return EXIT_FAILURE;
  }
  
  if (cfgfile == "" || weightsfile == "")
  {
    std::cout << "Wrong aruments\n";
    std::cout << "Use --help to print usage." << std::endl << std::endl;
    return EXIT_FAILURE;
  }
  
  detectDisplayLoop();

  glfwTerminate();
  return EXIT_SUCCESS;
}

int DetectionVisualizer::cameraInputInit()
{
  int apiID = cv::CAP_ANY;
  capture.open("/dev/video" + std::to_string(cameraID));

  if(!capture.isOpened()) {
    perror("Failed to initiate camera capture");
    return EXIT_FAILURE;
  }

  cv::Size originalresolution{
    static_cast<int>(capture.get(cv::CAP_PROP_FRAME_WIDTH)),
      static_cast<int>(capture.get(cv::CAP_PROP_FRAME_HEIGHT))
  };
  float originalaspectratio = (float)originalresolution.width / originalresolution.height;

  if (userspecifiedresolution.width != 0 || userspecifiedresolution.height != 0)
  {
    cv::Size capturedframe = userspecifiedresolution;
    if (userspecifiedresolution.height == 0)
    {
      capturedframe.height = userspecifiedresolution.width * 1/originalaspectratio;
    }
    if (userspecifiedresolution.width == 0)
    {
      capturedframe.width = userspecifiedresolution.height * originalaspectratio;
    }

    std::cout << "\nAsking the camera for " << capturedframe.width << " x " << capturedframe.height << " stream... ";

    if(!capture.set(cv::CAP_PROP_FRAME_HEIGHT, capturedframe.height))
    {
      perror("Failed to set height of frame to be captured");
      return EXIT_FAILURE;
    }
    if(!capture.set(cv::CAP_PROP_FRAME_WIDTH, capturedframe.width))
    {
      perror("Failed to set width of frame to be captured");
      return EXIT_FAILURE;
    }
  }

  originalresolution.width = capture.get(cv::CAP_PROP_FRAME_WIDTH);
  originalresolution.height = capture.get(cv::CAP_PROP_FRAME_HEIGHT);

  mainwindow.setContentSize(originalresolution);
  mainwindow.calculateContentAspectRatio();
  mainwindow.setSize(originalresolution);

  std::cout << "Got " << originalresolution.width << " x " << originalresolution.height << "." << std::endl << std::endl;
  return EXIT_SUCCESS;
}

int DetectionVisualizer::videoInputInit()
{
  capture.open("filesrc location=" + videofilepath + " ! decodebin ! videoconvert ! appsink" , cv::CAP_GSTREAMER);
  if(!capture.isOpened()) {
    perror("Failed to initiate video file capture");   
    return EXIT_FAILURE;
  }

  cv::Size designatedresolution{
    static_cast<int>(capture.get(cv::CAP_PROP_FRAME_WIDTH)),
    static_cast<int>(capture.get(cv::CAP_PROP_FRAME_HEIGHT))
  };

  mainwindow.setContentSize(designatedresolution);
  mainwindow.calculateContentAspectRatio();

  if (userspecifiedresolution.width != 0 && userspecifiedresolution.height != 0)
  {
    designatedresolution = userspecifiedresolution;
  }
  else if (userspecifiedresolution.width != 0 && userspecifiedresolution.height == 0)
  {
    designatedresolution = {
      userspecifiedresolution.width,
      static_cast<int>((float)userspecifiedresolution.width * 1.0f/mainwindow.getContentAspectRatio())
    };
  }
  else if (userspecifiedresolution.height != 0 && userspecifiedresolution.width == 0)
  {
    designatedresolution = {
      static_cast<int>((float)userspecifiedresolution.height * mainwindow.getContentAspectRatio()),
      userspecifiedresolution.height
    };
  }
  mainwindow.setSize(designatedresolution);
  
  return EXIT_SUCCESS;
}