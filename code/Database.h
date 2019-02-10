#ifndef TILEGEN_DATABASE_H
#define TILEGEN_DATABASE_H

#include <gf/Id.h>
#include <gf/Path.h>

#include "Biomes.h"
#include "Settings.h"

namespace tlgn {

  struct Database {
    Settings settings;

    std::map<gf::Id, Biome> biomes;
    std::vector<BiomeDuo> duos;
    std::vector<BiomeTrio> trios;
    std::vector<BiomeOverlay> overlays;

    Frontier getFrontier(gf::Id b1, gf::Id b2) const;
    int getIndex(gf::Id biome) const;

    static Database load(const gf::Path& filename);
  };

}

#endif // TILEGEN_DATABASE_H
