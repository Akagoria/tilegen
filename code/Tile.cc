#include "Tile.h"

#include <gf/Color.h>
#include <gf/Unused.h>
#include <gf/VectorOps.h>

namespace tlgn {

  namespace {

    gf::Direction rotateDirection(gf::Direction dir) {
      switch (dir) {
        case gf::Direction::Up:
          return gf::Direction::Left;
        case gf::Direction::Right:
          return gf::Direction::Up;
        case gf::Direction::Down:
          return gf::Direction::Right;
        case gf::Direction::Left:
          return gf::Direction::Down;
        default:
          assert(false);
          break;
      }

      return gf::Direction::Center;
    }

  }

  Tile::Tile(const TileSettings& settings, gf::Id biome)
  : size(settings.size)
  , spacing(settings.spacing)
  , pixels(settings.getTileSize(), biome)
  , colors(settings.getExtendedTileSize())
  , terrain({ gf::InvalidId, gf::InvalidId, gf::InvalidId, gf::InvalidId })
  , id(-1)
  {
    fences.count = 0;
    borders.count = 0;
  }

  Tile::Tile(gf::NoneType)
  : size(0)
  {
    fences.count = 0;
    borders.count = 0;
  }

  void Tile::rotate(int quarters) {
    int half = size / 2;

    for (int q = 0; q < quarters; ++q) {
      for (int i = 0; i < half; ++i) {
        int iprime = size - 1 - i;

        for (int j = 0; j < half; ++j) {
          int jprime = size - 1 - j;

          auto tmp = pixels({ i, j });
          pixels({ i, j }) = pixels({ jprime, i });
          pixels({ jprime, i }) = pixels({ iprime, jprime });
          pixels({ iprime, jprime }) = pixels({ j, iprime });
          pixels({ j, iprime }) = tmp;
        }
      }

      auto tmp = terrain[TerrainTopLeft];
      terrain[TerrainTopLeft] = terrain[TerrainTopRight];
      terrain[TerrainTopRight] = terrain[TerrainBottomRight];
      terrain[TerrainBottomRight] = terrain[TerrainBottomLeft];
      terrain[TerrainBottomLeft] = tmp;

      for (int i = 0; i < fences.count; ++i) {
        fences.fence[i].d1 = rotateDirection(fences.fence[i].d1);
        fences.fence[i].d2 = rotateDirection(fences.fence[i].d2);
      }

    }
  }

  void Tile::colorize(const std::map<gf::Id, Biome>& biomes, gf::Random& random) {
    checkPixels();
    generateColors(biomes, random);
    fillColorsBorder();
    generateBorder();
    fillColorsBorder();
  }

  void Tile::checkPixels() {
    for (auto pos : pixels.getPositionRange()) {
      gf::Id id = pixels(pos);

      if (id == gf::InvalidId) {
        auto& invalid = pixels(pos);

        for (auto next : pixels.get4NeighborsRange(pos)) {
          gf::Id nextBiome = pixels(next);

          if (nextBiome != gf::InvalidId) {
            invalid = nextBiome;
          }
        }
      }

      assert(pixels(pos) != gf::InvalidId);
    }
  }

  void Tile::generateColors(const std::map<gf::Id, Biome>& biomes, gf::Random& random) {
    for (auto pos : pixels.getPositionRange()) {
      gf::Id id = pixels(pos);

      if (id == Void) {
        colors(pos + 1) = gf::Color4f(1.0f, 1.0f, 1.0f, 0.0f);
        continue;
      }

      auto it = biomes.find(id);

      if (it == biomes.end()) {
        std::cerr << "Unknown id: " << std::hex << id << " at position " << std::dec << pos.x << ',' << pos.y << '\n';
      }

      assert(it != biomes.end());
      colors(pos + 1) = it->second.pigment.getColor(random, pos);
    }
  }

  void Tile::generateBorder() {
    Colors newColors(colors);

    for (int i = 0; i < borders.count; ++i) {
      auto& border = borders.border[i];

      if (border.effect == BorderEffect::None) {
        continue;
      }

      for (auto pos : pixels.getPositionRange()) {
        gf::Id id = pixels(pos);

        if (id == Void) {
          continue;
        }

        gf::Id other = Void;

        if (id == border.b1) {
          other = border.b2;
        } else if (id == border.b2) {
          other = border.b1;
        } else {
          continue;
        }

        int minDistance = size * 2;

        for (auto neighbor : pixels.getPositionRange()) {
          if (pixels(neighbor) != other) {
            continue;
          }

          int distance = gf::manhattanDistance(pos, neighbor);

          if (distance < minDistance) {
            minDistance = distance;
          }
        }

        auto colorPos = pos + spacing;
        auto color = colors(colorPos);

        switch (border.effect) {
          case BorderEffect::Fade:
            static constexpr int FadeDistance = 11;

            if (minDistance < FadeDistance) {
              color.a = gf::lerp(color.a, 0.0f, (FadeDistance - minDistance) / 10.0f);
            }

            break;

          case BorderEffect::Outline:
            if (minDistance <= 6) {
              color = gf::Color::darker(color, 0.2f);
            }

            break;

          case BorderEffect::Sharpen:
            static constexpr int SharpenDistance = 6;

            if (minDistance < SharpenDistance) {
              color = gf::Color::darker(color, (SharpenDistance - minDistance) * 0.05);
              color.a = gf::lerp(color.a, 1.0f, (SharpenDistance - minDistance) / 5.0f);
            }

            break;

          case BorderEffect::Blur:
            if (minDistance < 5) {
              // see https://en.wikipedia.org/wiki/Kernel_(image_processing)

              float finalCoeff = 36.0f;
              gf::Color4f finalColor = finalCoeff * colors(colorPos);

              for (auto next : colors.get24NeighborsRange(colorPos)) {
                gf::Color4f nextColor = colors(next);
                gf::Vector2i diff = gf::abs(colorPos - next);

                if (diff == gf::Vector2i(1, 0) || diff == gf::Vector2i(0, 1)) {
                  finalColor += 24.0f * nextColor;
                  finalCoeff += 24.0f;
                } else if (diff == gf::Vector2i(1, 1)) {
                  finalColor += 16.0f * nextColor;
                  finalCoeff += 16.0f;
                } else if (diff == gf::Vector2i(2, 0) || diff == gf::Vector2i(0, 2)) {
                  finalColor += 6.0f * nextColor;
                  finalCoeff += 6.0f;
                } else if (diff == gf::Vector2i(2, 1) || diff == gf::Vector2i(1, 2)) {
                  finalColor += 4.0f * nextColor;
                  finalCoeff += 4.0f;
                } else if (diff == gf::Vector2i(2, 2)) {
                  finalColor += 1.0f * nextColor;
                  finalCoeff += 1.0f;
                } else {
                  assert(false);
                }
              }

              color = finalColor / finalCoeff;
            }

            break;

          case BorderEffect::None:
            break;
        }

        newColors(colorPos) = color;
      }

    }


    colors = std::move(newColors);
  }

  void Tile::fillColorsBorder() {
    for (int i = 0; i < size; ++i) {
      // top
      colors({ 1 + i, 0 }) = colors({ 1 + i, 1 });
      // bottom
      colors({ 1 + i, size + 1 }) = colors({ 1 + i, size });
    }

    for (int i = 0; i < size / 2; ++i) {
      // left
      colors({ 0, i }) = colors({ 1, i });
      // right
      colors({ size + 1, i }) = colors({ size, i });
    }
  }

}
