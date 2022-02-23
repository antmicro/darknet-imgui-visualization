#include "DetectionVisualizer.hpp"

ThreadedDetector::ThreadedDetector(std::string& cfgfile, std::string& weightsfile) :
  detector(cfgfile, weightsfile)
{}

void ThreadedDetector::setFrame(cv::Mat newframe)
{
  std::lock_guard<std::mutex> guard(framemutex);
  frame = newframe.clone();
}

cv::Mat ThreadedDetector::getFrame()
{
  std::lock_guard<std::mutex> guard(framemutex);
  return frame;
}

void ThreadedDetector::setDetectedObjects(std::vector<bbox_t> detected)
{
  std::lock_guard<std::mutex> guard(detectedobjectsmutex);
  detectedobjects = detected;
}

std::vector<bbox_t> ThreadedDetector::getDetectedObjects()
{
  std::lock_guard<std::mutex> guard(detectedobjectsmutex);
  return detectedobjects;
}

bool ThreadedDetector::isRunning()
{
  return running;
}

void ThreadedDetector::startThread()
{
  running = true;
  thr = std::thread([this] { this->detectLoop(); });
}

void ThreadedDetector::detectLoop()
{
  double starttimer;
  while(true)
  {
    starttimer = glfwGetTime();
    cv::Mat frame = getFrame();
    if(!frame.empty())
    {
      std::vector<bbox_t> detected = detector.detect(frame);
      setDetectedObjects(detected);
    }
    inferencetime = glfwGetTime() - starttimer;
  }
}

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
    ("f,fullscreen", "puts window in fullscreen mode", cxxopts::value<bool>(fullscreen))
    ("n,names-file", "path to the file with names of detected objects, \e[1mrequired\e[0m", cxxopts::value<std::string>(namesfile))
    ("c,cfg-file", "path to the file with configuration, \e[1mrequired\e[0m", cxxopts::value<std::string>(cfgfile))
    ("w,weights-file", "path to the file with weights, \e[1mrequired\e[0m", cxxopts::value<std::string>(weightsfile))
    ("t,confidence-threshold", "starting confidence threshold of detected object", cxxopts::value<float>(threshold));

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
  ThreadedDetector detector(cfgfile, weightsfile);
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
  std::string filterclass;
  double overallstarttimestamp = glfwGetTime();
  double detectionstarttimestamp = 0.0;
  double finishtimestamp = glfwGetTime();

  ImGuiIO& io = ImGui::GetIO();
  ImFontConfig mainconfig, filterconfig;
  mainconfig.SizePixels = fontsize;
  io.Fonts-> AddFontDefault(&mainconfig);
  filterconfig.SizePixels = filterfontsize;
  ImFont* filterfont = io.Fonts->AddFontDefault(&filterconfig);

  while(glfwWindowShouldClose(mainwindow.window) == 0 && glfwGetKey(mainwindow.window, GLFW_KEY_ESCAPE) != GLFW_PRESS)
  {

    overallstarttimestamp = glfwGetTime();

    sprintf(frameratetext, 
        "%.1f / %.1f fps",
        1000.0/detectionstarttimestamp/1000.0, 
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

    detector.setFrame(frame);
    if(!detector.isRunning())
    {
      detector.startThread();
    }
    std::vector<bbox_t> detected_objects = detector.getDetectedObjects();

    detectionstarttimestamp = detector.inferencetime;

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

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, frame.cols, frame.rows, 0, GL_RGBA, GL_UNSIGNED_BYTE, frame.data);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

    ImGui::Image(reinterpret_cast<void*>(static_cast<intptr_t>(textureID)), ImVec2(frame.cols, frame.rows));
    ImDrawList* drawlist = ImGui::GetWindowDrawList();
    
    ImGui::End();
    ImGui::PushFont(filterfont);
    ImGui::Begin("Filter");

    ImGui::InputText("Class name", &filterclass, ImGuiInputTextFlags_CallbackCharFilter,
      [](ImGuiInputTextCallbackData* d) -> int {
        ImWchar c = d->EventChar;
        return !(std::isalpha(c) || c == ' ');
    });
    ImGui::SliderFloat("Probability threshold", &threshold, 0.0f, 1.0f);

    ImGui::BeginChild("scrolling");
    ImGui::BeginTable("Detections", 2);
    ImGui::TableSetupColumn("Class", ImGuiTableColumnFlags_WidthStretch);
    ImGui::TableSetupColumn("Certainty");
    ImGui::TableHeadersRow();

    ImGui::PopFont();

    for (bbox_t object : detected_objects) {
      ImU32 color = objectcolors[object.obj_id];
      std::string objectclass = objectnames[object.obj_id];
      ImVec4 listitemcolor;
      std::string text = objectnames[object.obj_id] + " (" + std::to_string(100 * object.prob) + "%)";
      
      std::transform(objectclass.begin(), objectclass.end(), objectclass.begin(),
              [](unsigned char c){ return std::tolower(c); }
      );
      std::transform(filterclass.begin(), filterclass.end(), filterclass.begin(),
              [](unsigned char c){ return std::tolower(c); }
      );

      if(object.prob >= threshold && 
              objectclass.find(filterclass) != std::string::npos) {
        ImVec2 upperleftcorner(
            object.x + imguiwindowposition.width,
            object.y + imguiwindowposition.height);
        ImVec2 lowerrightcorner(
            upperleftcorner.x + object.w,
            upperleftcorner.y + object.h);
  
        drawlist -> AddRect(
            upperleftcorner,
            lowerrightcorner,
            color,
            cornerroundingfactor,
            0,
            perimeterthickness); 
  
        ImVec2 textposition(
            upperleftcorner.x + cornerroundingfactor,
            upperleftcorner.y - fontsize - cornerroundingfactor);
        drawlist -> AddText(
            textposition,
            color,
            text.c_str()
            );

        listitemcolor = ImGui::ColorConvertU32ToFloat4(color);
      }
      else
      {
        listitemcolor = hiddenobjectcolor;
      }

      ImGui::PushFont(filterfont);
      ImGui::TableNextColumn();
      ImGui::TextColored(listitemcolor, "%s", objectclass.c_str());
      ImGui::TableNextColumn();
      ImGui::TextColored(listitemcolor, "%f", object.prob*100);
      ImGui::PopFont();

    }

    drawlist -> AddText(
        ImVec2 (
          imguiwindowposition.width + mainwindow.viewportsize.width - ImGui::CalcTextSize(frameratetext).x - cornerroundingfactor,
          imguiwindowposition.height + mainwindow.viewportsize.height - ImGui::CalcTextSize(frameratetext).y - cornerroundingfactor),
        frameratecolor,
        frameratetext
        );

    ImGui::EndTable();
    ImGui::EndChild();    
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
  
  if (mainwindow.init(windowname, fullscreen) != EXIT_SUCCESS)
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
 
  std::mt19937 rng(seed);
  std::uniform_real_distribution<float> dis(0.0, 1.0);
  for(int i = 0; i < objectnames.size(); i++)
    objectcolors.push_back(ImColor(ImVec4(dis(rng), dis(rng), dis(rng), 1.0f)));

  glGenTextures(1, &textureID);
  glBindTexture(GL_TEXTURE_2D, textureID);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

  detectDisplayLoop();

  glDeleteTextures(1, &textureID);

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
