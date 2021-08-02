#include "DetectionVisualizer.hpp"

int main(int argc, char *argv[])
{
  DetectionVisualizer vis;
  if (EXIT_SUCCESS != vis.parseArguments(argc, argv))
  {
      return EXIT_FAILURE;
  } 
  vis.run();
  return EXIT_SUCCESS;
}

