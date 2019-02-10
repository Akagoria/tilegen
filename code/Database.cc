#include "Database.h"

#include <nlohmann/json.hpp>

#include <gf/Color.h>

namespace tlgn {

  namespace {

    BorderEffect parseBorderEffect(const std::string& effect) {
      if (effect == "fade") {
        return BorderEffect::Fade;
      }

      if (effect == "outline") {
        return BorderEffect::Outline;
      }

      if (effect == "sharpen") {
        return BorderEffect::Sharpen;
      }

      if (effect == "blur") {
        return BorderEffect::Blur;
      }

      std::cerr << "Unknown border effect attribute: " << effect << '\n';
      return BorderEffect::None;
    }

    Frontier parseFrontier(nlohmann::json value) {
      Frontier frontier;

      frontier.offset = value["offset"].get<int>();

      if (value.count("border") == 1) {
        frontier.border.effect = parseBorderEffect(value["border"]["effect"].get<std::string>());
      }

      frontier.fence = value.count("fence") == 1 && value["fence"].get<bool>();

      return frontier;
    }

    PigmentStyle parsePigmentStyle(const std::string& style) {
      if (style == "plain") {
        return PigmentStyle::Plain;
      }

      if (style == "randomize") {
        return PigmentStyle::Randomize;
      }

      if (style == "striped") {
        return PigmentStyle::Striped;
      }

      std::cerr << "Unknown pigment style attribute: " << style << '\n';
      return PigmentStyle::Plain;
    }

  }

  Frontier Database::getFrontier(gf::Id b1, gf::Id b2) const {
    if (b1 == Void || b2 == Void) {
      for (auto& overlay : overlays) {
        if (overlay.b0 == b1) {
          return overlay.frontier;
        }

        if (overlay.b0 == b2) {
          return overlay.frontier.inverse();
        }
      }
    } else {
      for (auto& duo : duos) {
        if (duo.b1 == b1 && duo.b2 == b2) {
          return duo.frontier;
        }

        if (duo.b1 == b2 && duo.b2 == b1) {
          return duo.frontier.inverse();
        }
      }
    }

    return Frontier();
  }

  int Database::getIndex(gf::Id biome) const {
    if (biome == Void) {
      return -1;
    }

    auto it = biomes.find(biome);

    if (it == biomes.end()) {
      std::cerr << "Unknown biome id: " << biome << '\n';
      return -1;
    }

    return it->second.index;
  }

  Database Database::load(const gf::Path& filename) {
    std::ifstream ifs(filename.string());
    const auto j = nlohmann::json::parse(ifs);

    Database db;

    db.settings.name = j["settings"]["name"].get<std::string>();
    db.settings.tile.size = j["settings"]["tile"]["size"].get<int>();
    assert(db.settings.tile.size > 0);
    db.settings.tile.spacing = j["settings"]["tile"]["spacing"].get<int>();
    assert(db.settings.tile.spacing >= 0);

    auto image = j["settings"]["image"];
    db.settings.image = gf::Vector2i(image[0], image[1]);
    assert(db.settings.image.width > 0 && db.settings.image.height > 0);

    for (auto kv : j["biomes"].items()) {
      Biome biome;
      biome.index = std::stoi(kv.key());

      auto value = kv.value();
      biome.name = value["id"].get<std::string>();
      biome.id = gf::hash(biome.name);

      gf::Color4u color;
      color.r = value["color"][0].get<uint8_t>();
      color.g = value["color"][1].get<uint8_t>();
      color.b = value["color"][2].get<uint8_t>();
      color.a = value["color"][3].get<uint8_t>();

      biome.pigment.color = gf::Color::fromRgba32(color);
      biome.pigment.style = parsePigmentStyle(value["pigment"]["style"].get<std::string>());

      switch (biome.pigment.style) {
        case PigmentStyle::Randomize:
          biome.pigment.randomize.ratio = value["pigment"]["ratio"].get<double>();
          biome.pigment.randomize.deviation = value["pigment"]["deviation"].get<float>();
          break;
        default:
          break;
      }


      db.biomes.insert({ biome.id, biome });
    }

    auto check = [&db](const std::string& name) {
      gf::Id id = gf::hash(name);

      if (db.biomes.find(id) != db.biomes.end()) {
        return true;
      }

      std::cerr << "Unknown id: " << name << '\n';
      return false;
    };

    for (auto kv : j["duos"].items()) {
      BiomeDuo duo;

      auto value = kv.value();
      std::string b1 = value["biomes"][0].get<std::string>();
      std::string b2 = value["biomes"][1].get<std::string>();

      duo.b1 = gf::hash(b1); assert(check(b1));
      duo.b2 = gf::hash(b2); assert(check(b2));

      duo.frontier = parseFrontier(value);
      duo.frontier.border.b1 = duo.b1;
      duo.frontier.border.b2 = duo.b2;
      db.duos.push_back(duo);
    }

    for (auto kv : j["trios"].items()) {
      BiomeTrio trio;

      auto value = kv.value();
      std::string b1 = value[0].get<std::string>();
      std::string b2 = value[1].get<std::string>();
      std::string b3 = value[2].get<std::string>();

      trio.b1 = gf::hash(b1); assert(check(b1));
      trio.b2 = gf::hash(b2); assert(check(b2));
      trio.b3 = gf::hash(b3); assert(check(b3));

      db.trios.push_back(trio);
    }

    for (auto kv : j["overlays"].items()) {
      BiomeOverlay overlay;

      auto value = kv.value();
      std::string b0 = value["biome"].get<std::string>();

      overlay.b0 = gf::hash(b0); assert(check(b0));

      overlay.frontier = parseFrontier(value);
      overlay.frontier.border.b1 = overlay.b0;
      overlay.frontier.border.b2 = Void;

      db.overlays.push_back(overlay);
    }

    return db;
  }

}
