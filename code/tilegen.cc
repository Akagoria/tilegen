#include <cstdlib>

#include <iostream>

#include <gf/Path.h>

#include "Database.h"
#include "Export.h"
#include "Tileset.h"

int main(int argc, char *argv[]) {
  if (argc != 2) {
    std::cout << "Usage: tilegen <file>\n";
    return EXIT_FAILURE;
  }

  // load config file

  gf::Path filename(argv[1]);
  auto db = tlgn::Database::load(filename);

//   for (auto& kv : db.biomes) {
//     auto& value = kv.second;
//     std::cout << std::hex;
//     std::cout << value.id << ": " << value.name << " [ " << value.pigment.color.r << ',' << value.pigment.color.g << ',' << value.pigment.color.b << ',' << value.pigment.color.a << " ]\n";
//   }

  // generate pixels

  gf::Random random;

  std::cout << "Computing tiles (1/4)...\n";

  std::vector<tlgn::Tileset> plain;

  {
    std::map<int, gf::Id> biomes;

    for (auto& pair : db.biomes) {
      biomes.insert({ pair.second.index, pair.first });
    }

    for (auto& kv : biomes) {
      plain.push_back(tlgn::generatePlainTileset(kv.second, db));
    }
  }

  std::cout << "Computing tiles (2/4)...\n";

  std::vector<tlgn::Tileset> wang2;

  for (auto& duo : db.duos) {
    wang2.push_back(tlgn::generateTwoCornersWangTileset(duo.b1, duo.b2, random, db));
  }

  std::cout << "Computing tiles (3/4)...\n";

  std::vector<tlgn::Tileset> wang3;

  for (auto& trio : db.trios) {
    wang3.push_back(tlgn::generateThreeCornersWangTileset(trio.b1, trio.b2, trio.b3, random, db));
  }

  std::cout << "Computing tiles (4/4)...\n";

  std::vector<tlgn::Tileset> overlays;

  for (auto& overlay : db.overlays) {
    overlays.push_back(tlgn::generateTwoCornersWangTileset(overlay.b0, tlgn::Void, random, db));
  }

  // generate colors

  std::cout << "Computing colors (1/4)...\n";

  for (auto& tileset : plain) {
    for (auto& tile : tileset) {
      tile.colorize(db.biomes, random);
    }
  }

  std::cout << "Computing colors (2/4)...\n";

  for (auto& tileset : wang2) {
    for (auto& tile : tileset) {
      tile.colorize(db.biomes, random);
    }
  }

  std::cout << "Computing colors (3/4)...\n";

  for (auto& tileset : wang3) {
    for (auto& tile : tileset) {
      tile.colorize(db.biomes, random);
    }
  }

  std::cout << "Computing colors (4/4)...\n";

  for (auto& tileset : overlays) {
    for (auto& tile : tileset) {
      tile.colorize(db.biomes, random);
    }
  }

  // generate files

  std::cout << "Computing biome image...\n";

  tlgn::Colors image(db.settings.image);

  tlgn::ImageContext ctx;
  tlgn::exportTilesetsToImage(plain, db.settings, image, ctx);
  tlgn::exportTilesetsToImage(wang2, db.settings, image, ctx);
  tlgn::exportTilesetsToImage(wang3, db.settings, image, ctx);
  tlgn::exportTilesetsToImage(overlays, db.settings, image, ctx);

  std::cout << "Generating biome image...\n";
  tlgn::exportImageToFile(image, "biomes.png");

  tlgn::Terrains terrains;
  tlgn::exportTilesetsToTerrains(plain, db, terrains);
  tlgn::exportTilesetsToTerrains(wang2, db, terrains);
  tlgn::exportTilesetsToTerrains(wang3, db, terrains);
  tlgn::exportTilesetsToTerrains(overlays, db, terrains);

  std::cout << "Generating biome tileset...\n";

  {
    std::ofstream tileset("biomes.tsx");
    tlgn::exportTerrainsToFile(terrains, db, tileset);
  }

  return EXIT_SUCCESS;
}
