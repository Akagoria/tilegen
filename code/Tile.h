#ifndef TLGN_TILE_H
#define TLGN_TILE_H

#include <gf/Array2D.h>
#include <gf/Direction.h>
#include <gf/Id.h>
#include <gf/Vector.h>

#include "Settings.h"
#include "Biomes.h"

namespace tlgn {

  using Pixels = gf::Array2D<gf::Id, int>;
  using Colors = gf::Array2D<gf::Color4f, int>;


  struct Fence {
    gf::Direction d1;
    gf::Direction d2;
  };

  struct Fences {
    int count = 0;
    Fence fence[2];
  };

  struct Borders {
    int count = 0;
    Border border[2];
  };

  constexpr std::size_t TerrainTopLeft = 0;
  constexpr std::size_t TerrainTopRight = 1;
  constexpr std::size_t TerrainBottomLeft = 2;
  constexpr std::size_t TerrainBottomRight = 3;

  struct Tile {
    Tile(const TileSettings& settings, gf::Id biome = gf::InvalidId);
    Tile(gf::NoneType);

    int size;
    int spacing;

    Pixels pixels;
    Colors colors;

    std::array<gf::Id, 4> terrain;
    Fences fences;
    Borders borders;

    int id;

    void rotate(int quarters);
    void colorize(const std::map<gf::Id, Biome>& biomes, gf::Random& random);

  private:
    void checkPixels();
    void generateColors(const std::map<gf::Id, Biome>& biomes, gf::Random& random);
    void generateBorder();
    void fillColorsBorder();
  };

} // namespace tlgn

#endif // TLGN_TILE_H
