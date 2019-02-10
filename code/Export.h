#ifndef TILEGEN_EXPORT_H
#define TILEGEN_EXPORT_H

#include <array>
#include <map>
#include <iosfwd>

#include "Database.h"
#include "Settings.h"
#include "Tile.h"
#include "Tileset.h"

namespace tlgn {

  struct ImageContext {
    int startingPixelRow = 0;
  };

  void exportTilesetsToImage(std::vector<Tileset>& tilesets, const Settings& settings, Colors& image, ImageContext& ctx);
  void exportImageToFile(const Colors& image, std::ostream& os);

  struct Terrain {
    std::array<int, 4> indices;
    Fences fences;
  };

  using Terrains = std::map<int, Terrain>;

  void exportTilesetsToTerrains(const std::vector<Tileset>& tilesets, const Database& db, Terrains& terrains);
  void exportTerrainsToFile(const Terrains& terrains, const Database& db, std::ostream& os);

}

#endif // TILEGEN_EXPORT_H
