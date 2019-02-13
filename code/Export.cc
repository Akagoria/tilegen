#include "Export.h"

#include <gf/Color.h>
#include <gf/Image.h>
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

  void exportTilesetsToImage(std::vector<Tileset>& tilesets, const Settings& settings, Colors& image, ImageContext& ctx) {
    auto imageSize = image.getSize();
    auto tilesetSize = tilesets.front().getSize();

//     std::cout << "Image size: " << std::dec << imageSize.width << ',' << imageSize.height << '\n';
//     std::cout << "Tileset size: " << tilesetSize.width << ',' << tilesetSize.height << '\n';

    gf::Vector2i offset(0, ctx.startingPixelRow);

    int tilesetsPerRow = imageSize.width / (settings.tile.getExtendedSize() * tilesetSize.width);

    int tilesPerRow = imageSize.width / settings.tile.getExtendedSize();
    int idOffset = (ctx.startingPixelRow / settings.tile.getExtendedSize()) * tilesPerRow;

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

        gf::Vector2i totalOffset = offset + (offsetTileset * tilesetSize + offsetTile) * settings.tile.getExtendedSize();

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
    ctx.startingPixelRow += numberOfRows * tilesetSize.height * settings.tile.getExtendedSize();
  }

  void exportImageToFile(const Colors& image, const gf::Path& filename) {
    auto size = image.getSize();

    gf::Image out;
    out.create(size, gf::Color::toRgba32(gf::Color::Transparent));

    for (auto j : image.getRowRange()) {
      for (auto i : image.getColRange()) {
        gf::Color4f raw = image({ i, j });
        gf::Color4u color = gf::Color::toRgba32(raw);

        out.setPixel({ i, j }, color);
      }
    }

    out.saveToFile(filename);

#if 0
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
#endif
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

    template<typename T>
    struct KV {
      const char *key;
      T value;
    };

    template<typename T>
    KV<T> kv(const char * key, T value) {
      return { key, value };
    }

    template<typename T>
    std::ostream& operator<<(std::ostream& os, const KV<T>& kv) {
      return os << kv.key << '=' << '"' << kv.value << '"';
    }

  }

  void exportTerrainsToFile(const Terrains& terrains, const Database& db, std::ostream& os) {
    gf::Vector2i tileCount = db.settings.image / db.settings.tile.getExtendedTileSize();

    os << "<?xml " << kv("version", "1.0") << ' ' << kv("encoding", "UTF-8") << "?>\n";
    os << "<tileset " << kv("name", db.settings.name) << ' '
        << kv("tilewidth", db.settings.tile.size) << ' ' << kv("tileheight", db.settings.tile.size) << ' '
        << kv("tilecount", tileCount.width * tileCount.height) << ' ' << kv("columns", tileCount.width) << ' '
        << kv("spacing", db.settings.tile.spacing * 2) << ' ' << kv("margin", db.settings.tile.spacing)
        << ">\n";
    os << "<image " << kv("source", "biomes.png") << ' ' // TODO: pass the source name in the parameter
        << kv("width", db.settings.image.width) << ' ' << kv("height", db.settings.image.height)
        << "/>\n";

    os << "<terraintypes>\n";

    std::map<int, std::reference_wrapper<const Biome>> biomes;

    for (auto& pair : db.biomes) {
      biomes.insert({ pair.second.index, pair.second });
    }

    int index = 0;

    for (auto& pair : biomes) {
      assert(pair.first == index);
      const Biome& biome = pair.second;
      os << "\t<terrain " << kv("name", biome.name) << ' ' << kv("tile", findTerrainBiome(terrains, biome.index)) << "/>\n";
      index++;
    }

    os << "</terraintypes>\n";

    for (auto& pair : terrains) {
      const Terrain& terrain = pair.second;

      os << "<tile id=\"" << pair.first << "\" terrain=\""
        << dumpTerrainIndex(terrain.indices[0]) << ','
        << dumpTerrainIndex(terrain.indices[1]) << ','
        << dumpTerrainIndex(terrain.indices[2]) << ','
        << dumpTerrainIndex(terrain.indices[3]) << "\"";

      if (terrain.fences.count > 0) {
        os << ">\n";
        os << "\t<properties>\n";
        os << "\t\t<property " << kv("name", "fence_count") << ' ' << kv("type", "int") << ' ' << kv("value", terrain.fences.count) << " />\n";

        for (int i = 0; i < terrain.fences.count; ++i) {
          os << "\t\t<property name=\"fence" << i << "\" value=\"" << dumpTerrainFence(terrain.fences.fence[i].d1) << dumpTerrainFence(terrain.fences.fence[i].d2)  << "\"/>\n";
        }

        os << "\t</properties>\n";

        os << "</tile>\n";
      } else {
        os << "/>\n";
      }
    }

    os << "</tileset>\n";
  }

}
