#include <SFML/Graphics.hpp>
#include <Densiteye/Densiteye.hpp>
#include <fmt/core.h>
#include <vector>
#include <cmath>
#include <cstdint>

int main()
{
  Densiteye::Arguments args = 
    Densiteye::Arguments::Build()
      .InputFilePath("test.png")
      .OutputFolder("./output_test")
      .OutputName("test");

  Densiteye::Densiteye(args);

  return 0;
}
