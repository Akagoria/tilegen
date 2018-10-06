#ifndef TILEGEN_BIOMES_H
#define TILEGEN_BIOMES_H

#include <iostream>
#include <map>
#include <string>
#include <vector>

#include <gf/Id.h>
#include <gf/Random.h>
#include <gf/Vector.h>

using namespace gf::literals;

namespace tlgn {

  constexpr gf::Id Void = "Void"_id;

  enum class PigmentStyle {
    Plain,
    Randomize,
    Striped,
  };

  struct Pigment {
    gf::Color4f color;
    PigmentStyle style;

    union {
      struct {
        double ratio;
        float deviation;
      } randomize;
    };

    gf::Color4f getColor(gf::Random& random, gf::Vector2i pos) const;
  };

  struct Biome {
    gf::Id id;
    std::string name;
    Pigment pigment;
    int index;
  };

  enum class BorderEffect {
    None,
    Fade,
    Outline,
    Sharpen,
    Blur,
  };

  struct Border {
    BorderEffect effect;
    gf::Id b1;
    gf::Id b2;
  };

  struct Frontier {
    int offset = 0;
    Border border = { BorderEffect::None, gf::InvalidId, gf::InvalidId };
    bool fence = false;

    Frontier inverse() const {
      return { -offset, border, fence };
    }
  };

  struct BiomeDuo {
    gf::Id b1;
    gf::Id b2;
    Frontier frontier;
  };

  struct BiomeTrio {
    gf::Id b1;
    gf::Id b2;
    gf::Id b3;
  };

  struct BiomeOverlay {
    gf::Id b0;
    Frontier frontier;
  };


} // namespace tlgn

#endif // TILEGEN_BIOMES_H
