#ifndef TILEGEN_TILESET_H
#define TILEGEN_TILESET_H

#include <gf/Array2D.h>
#include <gf/Id.h>
#include <gf/Random.h>

#include "Database.h"
#include "Tile.h"

namespace tlgn {

  using Tileset = gf::Array2D<Tile, int>;

  Tileset generatePlainTileset(gf::Id b0, const Database& db);
  Tileset generateTwoCornersWangTileset(gf::Id b1, gf::Id b2, gf::Random& random, const Database& db);
  Tileset generateThreeCornersWangTileset(gf::Id b1, gf::Id b2, gf::Id b3, gf::Random& random, const Database& db);

}

#endif // TILEGEN_TILESET_H
