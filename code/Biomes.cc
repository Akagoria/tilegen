#include "Biomes.h"

#include <gf/Color.h>
#include <gf/Math.h>
#include <gf/VectorOps.h>

namespace tlgn {

  gf::Color4f Pigment::getColor(gf::Random& random, gf::Vector2i pos) const {
    switch (style) {
      case PigmentStyle::Plain:
        return color;

      case PigmentStyle::Randomize:
        if (random.computeBernoulli(randomize.ratio)) {
          float change = random.computeNormalFloat(0.0f, randomize.deviation);
          change = gf::clamp(change, -0.5f, 0.5f);

          if (change > 0) {
            return gf::Color::darker(color, change);
          }

          return gf::Color::lighter(color, -change);
        }

        return color;

      case PigmentStyle::Striped:
        if ((pos.x / 2 + pos.y / 2) % 8 == 3) {
          return color;
        }

        return color * gf::Color::Opaque(0.0f);
    }

    return color;
  }


}

