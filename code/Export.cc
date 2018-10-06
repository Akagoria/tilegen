#include "Export.h"

#include <gf/Color.h>
#include <gf/VectorOps.h>

#include "Settings.h"

namespace tlgn {

  namespace {

    template<typename T>
    void blit(const gf::Array2D<T, int>& source, gf::Array2D<T, int>& destination, gf::Vector2i offset) {
      auto sourceSize = source.getSize();
      auto destinationSize = destination.getSize();

      assert(offset.x >= 0);
      assert(offset.y >= 0);
      assert(offset.x + sourceSize.width <= destinationSize.width);
      assert(offset.y + sourceSize.height <= destinationSize.height);

      for (auto j : source.getRowRange()) {
        for (auto i : source.getColRange()) {
          destination({ offset.x + i, offset.y + j }) = source({ i, j });
        }
      }
    }

  }

  void exportTilesetsToImage(std::vector<Tileset>& tilesets, Colors& image, ImageContext& ctx) {
    auto imageSize = image.getSize();
    auto tilesetSize = tilesets.front().getSize();

//     std::cout << "Image size: " << std::dec << imageSize.width << ',' << imageSize.height << '\n';
//     std::cout << "Tileset size: " << tilesetSize.width << ',' << tilesetSize.height << '\n';

    gf::Vector2i offset(0, ctx.startingPixelRow);

    int tilesetsPerRow = imageSize.width / (TileSizeExt * tilesetSize.width);

    int tilesPerRow = imageSize.width / TileSizeExt;
    int idOffset = (ctx.startingPixelRow / TileSizeExt) * tilesPerRow;

//     std::cout << std::dec << "startingRow: " << startingRow << '\n';
//     std::cout << std::dec << "tilesPerRow: " << tilesPerRow << '\n';
//     std::cout << "idOffset: " << idOffset << '\n';

    int indexTileset = 0;

    for (auto& tileset : tilesets) {
      assert(tileset.getSize() == tilesetSize);

      gf::Vector2i offsetTileset;
      offsetTileset.x = indexTileset % tilesetsPerRow;
      offsetTileset.y = indexTileset / tilesetsPerRow;

      int idTileset = offsetTileset.y * tilesetSize.height * tilesPerRow + offsetTileset.x * tilesetSize.width;

//       std::cout << "idTileset: " << idTileset << '\n';

      int indexTile = 0;

      for (auto& tile : tileset) {
        gf::Vector2i offsetTile;
        offsetTile.x = indexTile % tilesetSize.width;
        offsetTile.y = indexTile / tilesetSize.width;

        gf::Vector2i totalOffset = offset + (offsetTileset * tilesetSize + offsetTile) * TileSizeExt;

//         std::cout << "offset: " << std::dec << offset.x << ',' << offset.y << '\n';
//         std::cout << "offsetTileset: " << std::dec << offsetTileset.x << ',' << offsetTileset.y << '\n';
//         std::cout << "offsetTile: " << std::dec << offsetTile.x << ',' << offsetTile.y << '\n';
//         std::cout << "totalOffset: " << std::dec << totalOffset.x << ',' << totalOffset.y << '\n';

        blit(tile.colors, image, totalOffset);

        tile.id = idOffset + idTileset + offsetTile.y * tilesPerRow + offsetTile.x;

        indexTile++;
      }

      indexTileset++;
    }

    int numberOfRows = (tilesets.size() - 1) / tilesetsPerRow + 1;
    ctx.startingPixelRow += numberOfRows * tilesetSize.height * TileSizeExt;
  }

  void exportImageToFile(const Colors& image, std::ostream& os) {
    auto size = image.getSize();

    os << "P7\n";
    os << "WIDTH " << size.width << '\n';
    os << "HEIGHT " << size.height << '\n';
    os << "DEPTH " << 4 << '\n';
    os << "MAXVAL " << 255 << '\n';
    os << "TUPLTYPE RGB_ALPHA\n";
    os << "ENDHDR\n";

    for (auto j : image.getRowRange()) {
      for (auto i : image.getColRange()) {
        gf::Color4f raw = image({ i, j });
        gf::Color4u color = gf::Color::toRgba32(raw);

        for (char channel : color) {
          os.write(&channel, 1);
        }
      }
    }
  }

  void exportTilesetsToTerrains(const std::vector<Tileset>& tilesets, const Database& db, Terrains& terrains) {
    for (auto& tileset : tilesets) {
      for (auto& tile : tileset) {
        Terrain terrain;

        terrain.indices[0] = db.getIndex(tile.terrain[0]);
        terrain.indices[1] = db.getIndex(tile.terrain[1]);
        terrain.indices[2] = db.getIndex(tile.terrain[2]);
        terrain.indices[3] = db.getIndex(tile.terrain[3]);

        if (terrains.find(tile.id) != terrains.end()) {
          std::cerr << "Duplicate index: " << tile.id << '\n';
        }

        terrain.fences = tile.fences;

        terrains.insert({ tile.id, terrain });
      }
    }
  }


  namespace {

    int findTerrainBiome(const Terrains& terrains, int index) {
      std::array<int, 4> model = { index, index, index, index };

      for (auto& terrain : terrains) {
        if (terrain.second.indices == model) {
          return terrain.first;
        }
      }

      return -1;
    }

    std::string dumpTerrainIndex(int index) {
      return index >= 0 ? std::to_string(index) : "";
    }

    std::string dumpTerrainFence(gf::Direction direction) {
      switch (direction) {
        case gf::Direction::Up:
          return "U";
        case gf::Direction::Right:
          return "R";
        case gf::Direction::Down:
          return "D";
        case gf::Direction::Left:
          return "L";
        default:
          assert(false);
      }

      return "?";
    }

  }

  void exportTerrainsToFile(const Terrains& terrains, const Database& db, std::ostream& os) {
    static constexpr int TileCount = ImageSize / TileSizeExt;

    os << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    os << "<tileset name=\"Biomes\" tilewidth=\"" << TileSize << "\" tileheight=\"" << TileSize << "\" tilecount=\"" << (TileCount * TileCount) << "\" columns=\"" << TileCount << "\" spacing=\"2\" margin=\"1\">\n";
    os << "<image source=\"biomes.png\" width=\"" << ImageSize << "\" height=\"" << ImageSize << "\"/>\n";

    os << "<terraintypes>\n";

    std::map<int, std::reference_wrapper<const Biome>> biomes;

    for (auto& kv : db.biomes) {
      biomes.insert({ kv.second.index, kv.second });
    }

    int index = 0;

    for (auto& kv : biomes) {
      assert(kv.first == index);
      const Biome& biome = kv.second;
      os << "  <terrain name=\"" << biome.name << "\" tile=\"" << findTerrainBiome(terrains, biome.index) << "\"/>\n";
      index++;
    }

    os << "</terraintypes>\n";

    for (auto& kv : terrains) {
      const Terrain& terrain = kv.second;

      os << "<tile id=\"" << kv.first << "\" terrain=\""
        << dumpTerrainIndex(terrain.indices[0]) << ','
        << dumpTerrainIndex(terrain.indices[1]) << ','
        << dumpTerrainIndex(terrain.indices[2]) << ','
        << dumpTerrainIndex(terrain.indices[3]) << "\"";

      if (terrain.fences.count > 0) {
        os << ">\n";

        for (int i = 0; i < terrain.fences.count; ++i) {
          os << "  <properties><property name=\"limit" << i << "\" value=\"" << dumpTerrainFence(terrain.fences.fence[i].d1) << dumpTerrainFence(terrain.fences.fence[i].d2)  << "\"/></properties>\n";
        }

        os << "</tile>\n";
      } else {
        os << "/>\n";
      }
    }

    os << "</tileset>\n";
  }

}
