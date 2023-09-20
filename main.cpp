#include <SFML/Graphics.hpp>
#include <fmt/core.h>
#include <vector>
#include <cmath>
#include <cstdint>

int main()
{
  sf::Image img;
  img.loadFromFile("test.png");
  
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
  
  for (int layer = 1; layerAdded; ++layer)
  {
    layerAdded = false;
    fmt::print("layer {}\n", layer);

    std::vector<int> updatingFloodLayers = floodLayers;
    
    for (int y = 0; y < img.getSize().y; ++y)
    {
      for (int x = 0; x < img.getSize().x; ++x)
      {
        auto color = img.getPixel(x, y);
        int index = y * img.getSize().x + x;
        auto &layers = updatingFloodLayers[index];
        
        if (layers > 0 || color.a == 0)
          continue;

        bool edgePixel =
        (y >= 1 ? edgeCompCheck(floodLayers, x, y - 1) : true) ||
        (y < img.getSize().y - 1 ? edgeCompCheck(floodLayers, x, y + 1) : true) ||
        (x >= 1 ? edgeCompCheck(floodLayers, x - 1, y) : true) ||
        (x < img.getSize().x - 1 ? edgeCompCheck(floodLayers, x + 1, y) : true);

        if (edgePixel)
        {
          layerAdded = true;
          layers = layer;
        }
      }
    }

    floodLayers = std::move(updatingFloodLayers);

    maxLayer = layer;
  }

  // INVERSE
  
  for (int layer = 1; inverseLayerAdded; ++layer)
  {
    inverseLayerAdded = false;
    fmt::print("layer {}\n", layer);

    std::vector<int> updatingFloodLayers = inverseFloodLayers;
    
    for (int y = 0; y < img.getSize().y; ++y)
    {
      for (int x = 0; x < img.getSize().x; ++x)
      {
        auto color = img.getPixel(x, y);
        int index = y * img.getSize().x + x;
        auto &layers = updatingFloodLayers[index];
        
        if (layers > 0 || color.a != 0)
          continue;

        bool edgePixel =
        (y >= 1 ? inverseEdgeCompCheck(inverseFloodLayers, x, y - 1) : true) ||
        (y < img.getSize().y - 1 ? inverseEdgeCompCheck(inverseFloodLayers, x, y + 1) : true) ||
        (x >= 1 ? inverseEdgeCompCheck(inverseFloodLayers, x - 1, y) : true) ||
        (x < img.getSize().x - 1 ? inverseEdgeCompCheck(inverseFloodLayers, x + 1, y) : true);

        if (edgePixel)
        {
          inverseLayerAdded = true;
          layers = layer;
        }
      }
    }

    inverseFloodLayers = std::move(updatingFloodLayers);
    inverseMaxLayer = layer;
  }

  sf::Image imgOut;
  imgOut.create(img.getSize().x, img.getSize().y);

  float colorLayerMultiplier = (255.f / maxLayer);
  float invColorLayerMultiplier = (255.f / inverseMaxLayer);

  auto layerToColor = [&](int layer, float multiplier)
  {
    std::uint8_t pixelIntensity = static_cast<std::uint8_t>((float)layer * multiplier);
    return sf::Color
    {
      pixelIntensity,
      pixelIntensity,
      pixelIntensity
    };
  };
  
  for (std::size_t y = 0; y < imgOut.getSize().y; ++y)
  {
    for (std::size_t x = 0; x < imgOut.getSize().x; ++x)
    {
      int layer = floodLayers[y * imgOut.getSize().x + x];
      int invLayer = inverseFloodLayers[y * imgOut.getSize().x + x];

      sf::Color color;
      
      if (layer > 0)
      {
        std::uint8_t pixelIntensity = static_cast<std::uint8_t>((float)layer * colorLayerMultiplier);
        color = layerToColor(layer, colorLayerMultiplier);
      }
      else if (invLayer > 0)
      {
        std::uint8_t invPixelIntensity = static_cast<std::uint8_t>((float)invLayer * invColorLayerMultiplier);
        color = layerToColor(invLayer, invColorLayerMultiplier);
        color.g = 0;
        color.b = 0;
      }
      imgOut.setPixel(x, y, color);
    }
  }

  imgOut.saveToFile("out.png");

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

      
      float radius =
      std::round((float)floodLayer / 2.f);
      
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
  scaleIt.getTexture().copyToImage().saveToFile("out2.png");

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

  for (int y = 0; y < imgOut.getSize().y; ++y)
  {
    for (int x = 0; x < imgOut.getSize().x; ++x)
    {
      imgOut.setPixel(x, y, sf::Color::Transparent);
      invImgOut.setPixel(x, y, sf::Color::Transparent);

      int floodLayer = floodLayers[y * imgOut.getSize().x + x];
      int invFloodLayer = inverseFloodLayers[y * invImgOut.getSize().x + x];

      if (invFloodLayer > 0)
      {
        int tipCount = 0;

        tipCount += invNeighbor(x-1, y) <= invFloodLayer ? 1 : 0;
        tipCount += invNeighbor(x+1, y) <= invFloodLayer ? 1 : 0;
        tipCount += invNeighbor(x, y+1) <= invFloodLayer ? 1 : 0;
        tipCount += invNeighbor(x, y-1) <= invFloodLayer ? 1 : 0;

        if (tipCount == 4)
          invImgOut.setPixel(x, y, sf::Color::White);
      }
      
      if (floodLayer > 0)
      {
        int tipCount = 0;

        tipCount += neighbor(x-1, y) <= floodLayer ? 1 : 0;
        tipCount += neighbor(x+1, y) <= floodLayer ? 1 : 0;
        tipCount += neighbor(x, y+1) <= floodLayer ? 1 : 0;
        tipCount += neighbor(x, y-1) <= floodLayer ? 1 : 0;

        if (tipCount == 4)
          imgOut.setPixel(x, y, sf::Color::White);
      }
    }
  }

  imgOut.saveToFile("out3.png");
  invImgOut.saveToFile("out4.png");

  
  return 0;
}
