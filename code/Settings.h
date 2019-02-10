#ifndef TILEGEN_SETTINGS_H
#define TILEGEN_SETTINGS_H

#include <string>

#include <gf/Vector.h>

namespace tlgn {

  struct TileSettings {
    int size;
    int spacing;

    gf::Vector2i getTileSize() const {
      return { size, size };
    }

    int getExtendedSize() const {
      return size + 2 * spacing;
    }

    gf::Vector2i getExtendedTileSize() const {
      return { getExtendedSize(), getExtendedSize() };
    }
  };

  struct Settings {
    std::string name;
    TileSettings tile;
    gf::Vector2i image;
  };

} // namespace tlgn

#endif // TILEGEN_SETTINGS_H
