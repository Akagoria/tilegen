add_executable(tilegen
  # main file
  tilegen.cc
  # other files
  Biomes.cc
  Database.cc
  Export.cc
  Settings.cc
  Tile.cc
  Tileset.cc
)

target_link_libraries(tilegen gf::gfcore0)

target_include_directories(tilegen
  PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/vendor/json/single_include"
)
