#include <Densiteye/Densiteye.hpp>
#include <functional>

namespace Densiteye
{
  void ForEveryImageXY(sf::Image const &img, auto onXY)
  {
    for (int y = 0; y < img.getSize().y; ++y)
    {
      for (int x = 0; x < img.getSize().x; ++x)
      {
        onXY(x, y);
      }
    }
  }

  void FloodLayering(
    sf::Image const &img,
    int &maxLayerOut,
    std::vector<int> &floodLayersOut,
    auto colorCheckFunc,
    auto edgeCompCheckFunc)
  {
    floodLayersOut.resize(img.getSize().x * img.getSize().y);
    bool layerAdded = true;

    for (int layer = 1; layerAdded; ++layer)
    {
      layerAdded = false;
      fmt::print("layer {}\n", layer);

      std::vector<int> updatingFloodLayers = floodLayersOut;
      
      ForEveryImageXY(img, [&](int x, int y)
      {
        auto color = img.getPixel(x, y);
        int index = y * img.getSize().x + x;
        auto &layers = updatingFloodLayers[index];
        
        if (layers > 0 || colorCheckFunc(color))
          return;

        bool edgePixel =
        (y >= 1 ? edgeCompCheckFunc(floodLayersOut, x, y - 1) : true) ||
        (y < img.getSize().y - 1 ? edgeCompCheckFunc(floodLayersOut, x, y + 1) : true) ||
        (x >= 1 ? edgeCompCheckFunc(floodLayersOut, x - 1, y) : true) ||
        (x < img.getSize().x - 1 ? edgeCompCheckFunc(floodLayersOut, x + 1, y) : true);

        if (edgePixel)
        {
          layerAdded = true;
          layers = layer;
        }
      });

      floodLayersOut = std::move(updatingFloodLayers);
      maxLayerOut = layer;
    }
  }

  auto FloodLayerRender(
    sf::Image const &img, 
    int maxLayer, 
    std::vector<int> const &floodLayers, 
    sf::Color const &color)
  {
    sf::Image imgOut;
    imgOut.create(img.getSize().x, img.getSize().y);

    float colorLayerMultiplierR = ((float)color.r / maxLayer);
    float colorLayerMultiplierG = ((float)color.g / maxLayer);
    float colorLayerMultiplierB = ((float)color.b / maxLayer);

    auto layerToColor = [&](int layer)
    {
      return sf::Color
      {
        static_cast<std::uint8_t>((float)layer * colorLayerMultiplierR),
        static_cast<std::uint8_t>((float)layer * colorLayerMultiplierG),
        static_cast<std::uint8_t>((float)layer * colorLayerMultiplierB)
      };
    };

    ForEveryImageXY(imgOut, [&](int x, int y)
    {
      int layer = floodLayers[y * imgOut.getSize().x + x];

      sf::Color color = layer > 0 ?
        layerToColor(layer) :
        sf::Color::Transparent;

      imgOut.setPixel(x, y, color);
    });
    
    return imgOut;
  }

  auto PeakExtractionRender(
    sf::Image const &img,
    std::vector<int> const &floodLayers,
    auto neighborFunc)
  {
    sf::Image imgOut = img;

    ForEveryImageXY(imgOut, [&](int x, int y)
    {
      imgOut.setPixel(x, y, sf::Color::Transparent);
      int floodLayer = floodLayers[y * imgOut.getSize().x + x];

      if (floodLayer <= 0)
        return;

      int tipCount = 0;

      tipCount += neighborFunc(x-1, y) <= floodLayer ? 1 : 0;
      tipCount += neighborFunc(x+1, y) <= floodLayer ? 1 : 0;
      tipCount += neighborFunc(x, y+1) <= floodLayer ? 1 : 0;
      tipCount += neighborFunc(x, y-1) <= floodLayer ? 1 : 0;

      if (tipCount == 4)
        imgOut.setPixel(x, y, sf::Color::White);
    });

    return imgOut;
  }

  bool TryValidateArgs(Arguments const &args)
  {
    if (!std::filesystem::exists(args.GetInputFilePath()))
    {
      fmt::print("Input file path does not exist: {}\n", args.GetInputFilePath().string());
      return false;
    }

    if (!std::filesystem::exists(args.GetOutputFolder()))
    {
      // Try to create before failing
      if (!std::filesystem::create_directories(args.GetOutputFolder()))
      {
        fmt::print("Output folder does not exist and could not be created: {}\n", args.GetOutputFolder().string());
        return false;
      }
    }

    return true;
  }

  int Densiteye(Arguments const &args)
  {
    if (!TryValidateArgs(args))
      return 1;

    sf::Image img;
    img.loadFromFile(args.GetInputFilePath());
    
    std::vector<int> floodLayers;
    std::vector<int> inverseFloodLayers;
    floodLayers.resize(img.getSize().x * img.getSize().y);
    inverseFloodLayers.resize(img.getSize().x * img.getSize().y);

    bool layerAdded = true;
    bool inverseLayerAdded = true;
    int maxLayer = 0;
    int inverseMaxLayer = 0;

    auto edgeCompCheck = [&](std::vector<int> const &layersContainer, int x, int y)
    {
      return img.getPixel(x, y).a == 0 ||
        layersContainer[y * img.getSize().x + x] > 0; 
    };

    auto inverseEdgeCompCheck = [&](std::vector<int> const &layersContainer, int x, int y)
    {
      return img.getPixel(x, y).a != 0 ||
      layersContainer[y * img.getSize().x + x] > 0; 
    };
    
    FloodLayering(img, maxLayer, floodLayers, [](auto const &color){ return color.a == 0; }, edgeCompCheck);
    FloodLayering(img, inverseMaxLayer, inverseFloodLayers, [](auto const &color){ return color.a != 0; }, inverseEdgeCompCheck);

    sf::Image mergedImgs;
    sf::Image imgOut;
    mergedImgs.create(img.getSize().x, img.getSize().y);
    imgOut.create(img.getSize().x, img.getSize().y);

    if (!args.IsOpaqueProcessingDisabled())
    {
      imgOut = FloodLayerRender(imgOut, maxLayer, floodLayers, sf::Color::White);
      imgOut.saveToFile((args.GetOutputFolder() / (args.GetOutputName() + ".gradient-foreground.png")).string());
      mergedImgs.copy(imgOut, 0, 0);
    }

    if (!args.IsTransparentProcessingDisabled())
    {
      imgOut = FloodLayerRender(imgOut, inverseMaxLayer, inverseFloodLayers, sf::Color::Red);
      imgOut.saveToFile((args.GetOutputFolder() / (args.GetOutputName() + ".gradient-background.png")).string());
    }

    if (!args.IsOpaqueProcessingDisabled() && !args.IsTransparentProcessingDisabled())
    {
      // Set mergedImgs' transparent to imgOut's opaque
      ForEveryImageXY(imgOut, [&](int x, int y)
      {
        if (imgOut.getPixel(x, y).a != 0)
          mergedImgs.setPixel(x, y, imgOut.getPixel(x, y));
      });

      mergedImgs.saveToFile((args.GetOutputFolder() / (args.GetOutputName() + ".gradient.png")).string());
    }

    sf::ContextSettings ctxSettings;
    ctxSettings.antialiasingLevel = 4;
    
    sf::RenderTexture scaleIt;
    float div = 1;
    scaleIt.create(imgOut.getSize().x / div, imgOut.getSize().y / div, ctxSettings);

    scaleIt.clear(sf::Color::Transparent);
    
    for (int y = 0; y < imgOut.getSize().y; ++y)
    {
      for (int x = 0; x < imgOut.getSize().x; ++x)
      {
        int floodLayer = floodLayers[y * imgOut.getSize().x + x];

        // if (floodLayer > 6)
        //   continue;

        
        float radius = std::round((float)floodLayer / 2.f);
        
        sf::CircleShape circle(radius);
        circle.setOrigin(circle.getRadius() , circle.getRadius() );

        circle.setFillColor(
        {
          255, 255, 255,
          img.getPixel(x, y).a
        });
        
        circle.setPosition(x / div, y / div);
        scaleIt.draw(circle);
      }
    }
    
    for (int y = 0; y < imgOut.getSize().y; ++y)
    {
      for (int x = 0; x < imgOut.getSize().x; ++x)
      {
        int floodLayer = inverseFloodLayers[y * imgOut.getSize().x + x];

        if (floodLayer == 0)
          continue;

        
        float radius = .5f;
        //std::round((float)floodLayer / 2.f);
        
        sf::CircleShape circle(radius);
        circle.setOrigin(circle.getRadius() , circle.getRadius() );

        circle.setFillColor(sf::Color::Black);
        
        circle.setPosition(x / div, y / div);
        scaleIt.draw(circle);
      }
    }

    scaleIt.display();

    scaleIt.getTexture().copyToImage().saveToFile(
      (args.GetOutputFolder() / (args.GetOutputName() + ".scaleit.png")).string());

    auto invNeighbor=[&](int x, int y)
    {
      if (x < 0 || y < 0 || x >= imgOut.getSize().x || y >= imgOut.getSize().y)
        return 0;

      return inverseFloodLayers[y * imgOut.getSize().x + x];
    };

    auto neighbor=[&](int x, int y)
    {
      if (x < 0 || y < 0 || x >= imgOut.getSize().x || y >= imgOut.getSize().y)
        return 0;

      return floodLayers[y * imgOut.getSize().x + x];
    };
    
    sf::Image invImgOut;
    invImgOut.create(imgOut.getSize().x, imgOut.getSize().y, imgOut.getPixelsPtr());

    imgOut = PeakExtractionRender(imgOut, floodLayers, neighbor);
    invImgOut = PeakExtractionRender(invImgOut, inverseFloodLayers, invNeighbor);

    imgOut.saveToFile((args.GetOutputFolder() / (args.GetOutputName() + ".peaklines.png")).string());
    invImgOut.saveToFile((args.GetOutputFolder() / (args.GetOutputName() + ".inverse-peaklines.png")).string());

    return 0;
  }
}
