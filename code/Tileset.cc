#include "Tileset.h"

#include <queue>

#include <gf/ArrayRef.h>
#include <gf/Geometry.h>
#include <gf/Unused.h>
#include <gf/VectorOps.h>

namespace tlgn {

  namespace {

    constexpr gf::Vector2i top(const TileSettings& settings, int i) {
      gf::unused(settings);
      return { i, 0 };
    }

    constexpr gf::Vector2i bottom(const TileSettings& settings, int i) {
      return { i, settings.size - 1 };
    }

    constexpr gf::Vector2i left(const TileSettings& settings, int i) {
      gf::unused(settings);
      return { 0, i };
    }

    constexpr gf::Vector2i right(const TileSettings& settings, int i) {
      return { settings.size - 1, i };
    }

    constexpr gf::Vector2i cornerTopLeft(const TileSettings& settings) {
      gf::unused(settings);
      return { 0, 0 };
    }

    constexpr gf::Vector2i cornerTopRight(const TileSettings& settings) {
      return { settings.size - 1, 0 };
    }

    constexpr gf::Vector2i cornerBottomLeft(const TileSettings& settings) {
      return { 0, settings.size - 1 };
    }

    constexpr gf::Vector2i cornerBottomRight(const TileSettings& settings) {
      return { settings.size - 1, settings.size - 1 };
    }


    void fillBiomeFrom(Pixels& pixels, gf::Vector2i pos, gf::Id biome) {
      pixels(pos) = biome;

      std::queue<gf::Vector2i> q;
      q.push(pos);

      while (!q.empty()) {
        auto curr = q.front();

        assert(pixels(curr) == biome);

        for (auto next : pixels.get4NeighborsRange(curr)) {
          gf::Id nextBiome = pixels(next);

          if (nextBiome == gf::InvalidId) {
            pixels(next) = biome;
            q.push(next);
          }
        }

        q.pop();
      }

    }


    std::vector<gf::Vector2i> makeLine(const TileSettings& settings, gf::ArrayRef<gf::Vector2i> points, gf::Random& random) {
      constexpr unsigned GenerationIterations = 2;
      constexpr float InitialFactor = 0.5f;
      constexpr float ReductionFactor = 0.6f;

      // generate random line points

      std::vector<gf::Vector2i> tmp;

      for (std::size_t i = 0; i < points.getSize() - 1; ++i) {
        auto line = gf::midpointDisplacement1D(points[i], points[i + 1], random, GenerationIterations, InitialFactor, ReductionFactor);
        tmp.insert(tmp.end(), line.begin(), line.end());
        tmp.pop_back();
      }

      tmp.push_back(points[points.getSize() - 1]);

      // normalize

      for (auto& point : tmp) {
        point = gf::clamp(point, 0, settings.size - 1);
      }

      // compute final line

      std::vector<gf::Vector2i> out;

      for (std::size_t i = 0; i < tmp.size() - 1; ++i) {
        auto line = gf::generateLine(tmp[i], tmp[i + 1]);
        out.insert(out.end(), line.begin(), line.end());
      }

      out.push_back(points[points.getSize() - 1]);

      return out;
    }

    /*
     * Two Corner Wang Tileset generators
     */

    Tile generateFull(const TileSettings& settings, gf::Id biome) {
      Tile tile(settings, biome);
      tile.terrain[TerrainTopLeft] = tile.terrain[TerrainTopRight] = tile.terrain[TerrainBottomLeft] = tile.terrain[TerrainBottomRight] = biome;
      return tile;
    }

    enum class Split {
      Horizontal, // left + right
      Vertical,   // top + bottom
    };

    // b1 is in the left|top, b2 is in the right|bottom
    Tile generateSplit(const TileSettings& settings, gf::Id b1, gf::Id b2, Split s, gf::Random& random, const Frontier& frontier) {
      Tile tile(settings);

      gf::Vector2i start, stop;
      int half = settings.size / 2;

      switch (s) {
        case Split::Horizontal:
          start = left(settings, half + frontier.offset);
          stop = right(settings, half + frontier.offset);
          break;
        case Split::Vertical:
          start = top(settings, half + frontier.offset);
          stop = bottom(settings, half + frontier.offset);
          break;
      }

      auto line = makeLine(settings, { start, stop }, random);

      for (auto point : line) {
        tile.pixels(point) = b2;
      }

      fillBiomeFrom(tile.pixels, cornerTopLeft(settings), b1);
      fillBiomeFrom(tile.pixels, cornerBottomRight(settings), b2);

      switch (s) {
        case Split::Horizontal:
          tile.terrain[TerrainTopLeft] = tile.terrain[TerrainTopRight] = b1;
          tile.terrain[TerrainBottomLeft] = tile.terrain[TerrainBottomRight] = b2;
          break;

        case Split::Vertical:
          tile.terrain[TerrainTopLeft] = tile.terrain[TerrainBottomLeft] = b1;
          tile.terrain[TerrainTopRight] = tile.terrain[TerrainBottomRight] = b2;
          break;
      }

      if (frontier.fence) {
        tile.fences.count = 1;

        switch (s) {
          case Split::Horizontal:
            tile.fences.fence[0].d1 = gf::Direction::Left;
            tile.fences.fence[0].d2 = gf::Direction::Right;
            break;

          case Split::Vertical:
            tile.fences.fence[0].d1 = gf::Direction::Up;
            tile.fences.fence[0].d2 = gf::Direction::Down;
            break;
        }
      }

      if (frontier.border.effect != BorderEffect::None) {
        tile.borders.border[tile.borders.count++] = frontier.border;
      }

      return tile;
    }

    enum class Corner {
      TopLeft,
      TopRight,
      BottomLeft,
      BottomRight,
    };

    // b1 is in the corner, b2 is in the rest
    Tile generateCorner(const TileSettings& settings, gf::Id b1, gf::Id b2, Corner c, gf::Random& random, const Frontier& frontier) {
      Tile tile(settings);

      gf::Vector2i start, stop;
      int half = settings.size / 2;

      switch (c) {
        case Corner::TopLeft:
          start = top(settings, half - 1 + frontier.offset);
          stop = left(settings, half - 1 + frontier.offset);
          break;
        case Corner::TopRight:
          start = top(settings, half - frontier.offset);
          stop = right(settings, half - 1 + frontier.offset);
          break;
        case Corner::BottomLeft:
          start = bottom(settings, half - 1 + frontier.offset);
          stop = left(settings, half - frontier.offset);
          break;
        case Corner::BottomRight:
          start = bottom(settings, half - frontier.offset);
          stop = right(settings, half - frontier.offset);
          break;
      }

      auto line = makeLine(settings, { start, stop }, random);

      for (auto point : line) {
        tile.pixels(point) = b1;
      }

      switch (c) {
        case Corner::TopLeft:
          fillBiomeFrom(tile.pixels, cornerTopLeft(settings), b1);
          fillBiomeFrom(tile.pixels, cornerBottomRight(settings), b2);
          break;
        case Corner::TopRight:
          fillBiomeFrom(tile.pixels, cornerTopRight(settings), b1);
          fillBiomeFrom(tile.pixels, cornerBottomLeft(settings), b2);
          break;
        case Corner::BottomLeft:
          fillBiomeFrom(tile.pixels, cornerBottomLeft(settings), b1);
          fillBiomeFrom(tile.pixels, cornerTopRight(settings), b2);
          break;
        case Corner::BottomRight:
          fillBiomeFrom(tile.pixels, cornerBottomRight(settings), b1);
          fillBiomeFrom(tile.pixels, cornerTopLeft(settings), b2);
          break;
      }

      tile.terrain[TerrainTopLeft] = tile.terrain[TerrainTopRight] = tile.terrain[TerrainBottomLeft] = tile.terrain[TerrainBottomRight] = b2;

      switch (c) {
        case Corner::TopLeft:
          tile.terrain[TerrainTopLeft] = b1;
          break;
        case Corner::TopRight:
          tile.terrain[TerrainTopRight] = b1;
          break;
        case Corner::BottomLeft:
          tile.terrain[TerrainBottomLeft] = b1;
          break;
        case Corner::BottomRight:
          tile.terrain[TerrainBottomRight] = b1;
          break;
      }

      if (frontier.fence) {
        tile.fences.count = 1;

        switch (c) {
          case Corner::TopLeft:
            tile.fences.fence[0].d1 = gf::Direction::Up;
            tile.fences.fence[0].d2 = gf::Direction::Left;
            break;
          case Corner::TopRight:
            tile.fences.fence[0].d1 = gf::Direction::Up;
            tile.fences.fence[0].d2 = gf::Direction::Right;
            break;
          case Corner::BottomLeft:
            tile.fences.fence[0].d1 = gf::Direction::Left;
            tile.fences.fence[0].d2 = gf::Direction::Down;
            break;
          case Corner::BottomRight:
            tile.fences.fence[0].d1 = gf::Direction::Right;
            tile.fences.fence[0].d2 = gf::Direction::Down;
            break;
        }
      }

      if (frontier.border.effect != BorderEffect::None) {
        tile.borders.border[tile.borders.count++] = frontier.border;
      }

      return tile;
    }

    // b1 is in top-left and bottom-right, b2 is in top-right and bottom-left
    Tile generateCross(const TileSettings& settings, gf::Id b1, gf::Id b2, gf::Random& random, const Frontier& frontier) {
      Tile tile(settings);
      int half = settings.size / 2;

      gf::Vector2i limitTopRight[] = { top(settings, half + frontier.offset), { half, half - 1 }, right(settings, half - 1 - frontier.offset) };

      auto lineTopRight = makeLine(settings, limitTopRight, random);

      for (auto point : lineTopRight) {
        tile.pixels(point) = b2;
      }

      gf::Vector2i limitBottomLeft[] = { bottom(settings, half - 1 - frontier.offset), { half - 1, half }, left(settings, half + frontier.offset) };

      auto lineBottomLeft = makeLine(settings, limitBottomLeft, random);

      for (auto point : lineBottomLeft) {
        tile.pixels(point) = b2;
      }

      fillBiomeFrom(tile.pixels, cornerTopLeft(settings), b1);
      fillBiomeFrom(tile.pixels, cornerBottomRight(settings), b1);
      fillBiomeFrom(tile.pixels, cornerTopRight(settings), b2);
      fillBiomeFrom(tile.pixels, cornerBottomLeft(settings), b2);

      tile.terrain[TerrainTopLeft] = tile.terrain[TerrainBottomRight] = b1;
      tile.terrain[TerrainTopRight] = tile.terrain[TerrainBottomLeft] = b2;

      if (frontier.fence) {
        tile.fences.count = 2;
        tile.fences.fence[0].d1 = gf::Direction::Up;
        tile.fences.fence[0].d2 = gf::Direction::Right;
        tile.fences.fence[1].d1 = gf::Direction::Down;
        tile.fences.fence[1].d2 = gf::Direction::Left;
      }

      if (frontier.border.effect != BorderEffect::None) {
        tile.borders.border[tile.borders.count++] = frontier.border;
      }

      return tile;
    }

    /*
     * Three Corner Wang Tileset generators
     */

    // b1 is top, b2 is in bottom-left, b3 is in bottom-right
    Tile generate211(const TileSettings& settings, gf::Id b1, gf::Id b2, gf::Id b3, gf::Random& random, const Frontier& frontier12, const Frontier& frontier23, const Frontier& frontier31) {
      Tile tile(settings);
      int half = settings.size / 2;

      gf::Vector2i limitBottomLeft[] = { left(settings, half + frontier12.offset), /* { half - 1, half }, */ bottom(settings, half - 1 + frontier23.offset) };

      auto lineBottomLeft = makeLine(settings, limitBottomLeft, random);

      for (auto point : lineBottomLeft) {
        tile.pixels(point) = b2;
      }

      gf::Vector2i limitBottomRight[] = { right(settings, half - frontier31.offset), /* { half, half }, */ bottom(settings, half + frontier23.offset) };

      auto lineBottomRight = makeLine(settings, limitBottomRight, random);

      for (auto point : lineBottomRight) {
        tile.pixels(point) = b3;
      }

      fillBiomeFrom(tile.pixels, cornerTopLeft(settings), b1);
      fillBiomeFrom(tile.pixels, cornerBottomLeft(settings), b2);
      fillBiomeFrom(tile.pixels, cornerBottomRight(settings), b3);

      tile.terrain[TerrainTopLeft] = tile.terrain[TerrainTopRight] = b1;
      tile.terrain[TerrainBottomLeft] = b2;
      tile.terrain[TerrainBottomRight] = b3;

      if (frontier12.fence) {
        int current = tile.fences.count++;
        tile.fences.fence[current].d1 = gf::Direction::Left;
        tile.fences.fence[current].d2 = gf::Direction::Down;
      }

      if (frontier31.fence) {
        int current = tile.fences.count++;
        tile.fences.fence[current].d1 = gf::Direction::Right;
        tile.fences.fence[current].d2 = gf::Direction::Down;
      }


      if (frontier12.border.effect != BorderEffect::None) {
        tile.borders.border[tile.borders.count++] = frontier12.border;
      }

      if (frontier31.border.effect != BorderEffect::None) {
        tile.borders.border[tile.borders.count++] = frontier31.border;
      }

      return tile;
    }

    Tile generate211Cross(const TileSettings& settings, gf::Id b1, gf::Id b2, gf::Id b3, gf::Random& random, const Frontier& frontier12, const Frontier& frontier31) {
      Tile tile(settings);
      int half = settings.size / 2;

      gf::Vector2i limitTopRight[] = { right(settings, half - 1 - frontier12.offset), top(settings, half + frontier12.offset) };

      auto lineTopRight = makeLine(settings, limitTopRight, random);

      for (auto point : lineTopRight) {
        tile.pixels(point) = b2;
      }

      gf::Vector2i limitBottomLeft[] = { left(settings, half - frontier31.offset), bottom(settings, half - 1 + frontier31.offset) };

      auto lineBottomLeft = makeLine(settings, limitBottomLeft, random);

      for (auto point : lineBottomLeft) {
        tile.pixels(point) = b3;
      }

      fillBiomeFrom(tile.pixels, cornerTopLeft(settings), b1);
      fillBiomeFrom(tile.pixels, cornerBottomRight(settings), b1);
      fillBiomeFrom(tile.pixels, cornerTopRight(settings), b2);
      fillBiomeFrom(tile.pixels, cornerBottomLeft(settings), b3);

      tile.terrain[TerrainTopLeft] = tile.terrain[TerrainBottomRight] = b1;
      tile.terrain[TerrainTopRight] = b2;
      tile.terrain[TerrainBottomLeft] = b3;

      if (frontier12.fence) {
        int current = tile.fences.count++;
        tile.fences.fence[current].d1 = gf::Direction::Right;
        tile.fences.fence[current].d2 = gf::Direction::Up;
      }

      if (frontier31.fence) {
        int current = tile.fences.count++;
        tile.fences.fence[current].d1 = gf::Direction::Left;
        tile.fences.fence[current].d2 = gf::Direction::Down;
      }


      if (frontier12.border.effect != BorderEffect::None) {
        tile.borders.border[tile.borders.count++] = frontier12.border;
      }

      if (frontier31.border.effect != BorderEffect::None) {
        tile.borders.border[tile.borders.count++] = frontier31.border;
      }

      return tile;
    }

  }


  /*
   *    0    1    2    3
   *  0 +----+----+----+----+
   *    |    |  ##|##  |    |
   *    |##  |  ##|####|####|
   *  1 +----+----+----+----+
   *    |##  |  ##|####|####|
   *    |  ##|####|####|##  |
   *  2 +----+----+----+----+
   *    |  ##|####|####|##  |
   *    |    |    |  ##|##  |
   *  3 +----+----+----+----+
   *    |    |    |  ##|##  |
   *    |    |  ##|##  |    |
   *    +----+----+----+----+
   *
   *    b1 = ' '
   *    b2 = '#'
   */
  Tileset generateTwoCornersWangTileset(gf::Id b1, gf::Id b2, gf::Random& random, const Database& db) {
    Tileset tileset({ 4, 4 }, gf::None);

    auto frontier = db.getFrontier(b1, b2);

    tileset({ 0, 0 }) = generateCorner(db.settings.tile, b2, b1, Corner::BottomLeft, random, frontier.inverse());
    tileset({ 0, 1 }) = generateCross(db.settings.tile, b2, b1, random, frontier.inverse());
    tileset({ 0, 2 }) = generateCorner(db.settings.tile, b2, b1, Corner::TopRight, random, frontier.inverse());
    tileset({ 0, 3 }) = generateFull(db.settings.tile, b1);

    tileset({ 1, 0 }) = generateSplit(db.settings.tile, b1, b2, Split::Vertical, random, frontier);
    tileset({ 1, 1 }) = generateCorner(db.settings.tile, b1, b2, Corner::TopLeft, random, frontier);
    tileset({ 1, 2 }) = generateSplit(db.settings.tile, b2, b1, Split::Horizontal, random, frontier.inverse());
    tileset({ 1, 3 }) = generateCorner(db.settings.tile, b2, b1, Corner::BottomRight, random, frontier.inverse());

    tileset({ 2, 0 }) = generateCorner(db.settings.tile, b1, b2, Corner::TopRight, random, frontier);
    tileset({ 2, 1 }) = generateFull(db.settings.tile, b2);
    tileset({ 2, 2 }) = generateCorner(db.settings.tile, b1, b2, Corner::BottomLeft, random, frontier);
    tileset({ 2, 3 }) = generateCross(db.settings.tile, b1, b2, random, frontier);

    tileset({ 3, 0 }) = generateSplit(db.settings.tile, b1, b2, Split::Horizontal, random, frontier);
    tileset({ 3, 1 }) = generateCorner(db.settings.tile, b1, b2, Corner::BottomRight, random, frontier);
    tileset({ 3, 2 }) = generateSplit(db.settings.tile, b2, b1, Split::Vertical, random, frontier.inverse());
    tileset({ 3, 3 }) = generateCorner(db.settings.tile, b2, b1, Corner::TopLeft, random, frontier.inverse());

    return tileset;
  }

  /*

  4 * 3 *
  +----+
  |####|
  |**  |
  +----+

  4 * 3 *
  +----+
  |####|
  |  **|
  +----+

  4 * 3 *
  +----+
  |##**|
  |  ##|
  +----+

  */
  Tileset generateThreeCornersWangTileset(gf::Id b1, gf::Id b2, gf::Id b3, gf::Random& random, const Database& db) {
    Tileset tileset({ 9, 4 }, gf::None);

    auto frontier12 = db.getFrontier(b1, b2);
    auto frontier23 = db.getFrontier(b2, b3);
    auto frontier31 = db.getFrontier(b3, b1);

    for (int q = 0; q < 4; ++q) {
      tileset({ 0, q }) = generate211(db.settings.tile, b1, b2, b3, random, frontier12, frontier23, frontier31);
      tileset({ 0, q }).rotate(q);
    }

    for (int q = 0; q < 4; ++q) {
      tileset({ 1, q }) = generate211(db.settings.tile, b2, b3, b1, random, frontier23, frontier31, frontier12);
      tileset({ 1, q }).rotate(q);
    }

    for (int q = 0; q < 4; ++q) {
      tileset({ 2, q }) = generate211(db.settings.tile, b3, b1, b2, random, frontier31, frontier12, frontier23);
      tileset({ 2, q }).rotate(q);
    }

    for (int q = 0; q < 4; ++q) {
      tileset({ 3, q }) = generate211(db.settings.tile, b1, b3, b2, random, frontier31.inverse(), frontier23.inverse(), frontier12.inverse());
      tileset({ 3, q }).rotate(q);
    }

    for (int q = 0; q < 4; ++q) {
      tileset({ 4, q }) = generate211(db.settings.tile, b3, b2, b1, random, frontier23.inverse(), frontier12.inverse(), frontier31.inverse());
      tileset({ 4, q }).rotate(q);
    }

    for (int q = 0; q < 4; ++q) {
      tileset({ 5, q }) = generate211(db.settings.tile, b2, b1, b3, random, frontier12.inverse(), frontier31.inverse(), frontier23.inverse());
      tileset({ 5, q }).rotate(q);
    }

    for (int q = 0; q < 4; ++q) {
      tileset({ 6, q }) = generate211Cross(db.settings.tile, b1, b2, b3, random, frontier12, frontier31);
      tileset({ 6, q }).rotate(q);
    }

    for (int q = 0; q < 4; ++q) {
      tileset({ 7, q }) = generate211Cross(db.settings.tile, b2, b3, b1, random, frontier23, frontier12);
      tileset({ 7, q }).rotate(q);
    }

    for (int q = 0; q < 4; ++q) {
      tileset({ 8, q }) = generate211Cross(db.settings.tile, b3, b1, b2, random, frontier31, frontier23);
      tileset({ 8, q }).rotate(q);
    }

    return tileset;
  }


  Tileset generatePlainTileset(gf::Id b0, const Database& db) {
    Tileset tileset({ 4, 4 }, gf::None);

    for (int i = 0; i < 4; ++i) {
      for (int j = 0; j < 4; ++j) {
        tileset({ i, j }) = generateFull(db.settings.tile, b0);
      }
    }

    return tileset;
  }

}


