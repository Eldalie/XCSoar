// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "MbTilesDatabase.hpp"

#include "Math/Angle.hpp"
#include "util/NumberParser.hpp"

#include <cstdlib>
#include <stdexcept>
#include <string_view>

namespace EDL {

static GeoBounds
ParseBounds(std::string_view value) noexcept
{
  if (value.empty())
    return GeoBounds::Invalid();

  auto *cursor = value.data();
  char *endptr = nullptr;

  const double west = ParseDouble(cursor, &endptr);
  if (endptr == cursor || *endptr != ',')
    return GeoBounds::Invalid();
  cursor = endptr + 1;

  const double south = ParseDouble(cursor, &endptr);
  if (endptr == cursor || *endptr != ',')
    return GeoBounds::Invalid();
  cursor = endptr + 1;

  const double east = ParseDouble(cursor, &endptr);
  if (endptr == cursor || *endptr != ',')
    return GeoBounds::Invalid();
  cursor = endptr + 1;

  const double north = ParseDouble(cursor, &endptr);
  if (endptr == cursor || *endptr != '\0')
    return GeoBounds::Invalid();

  return GeoBounds({Angle::Degrees(west), Angle::Degrees(north)},
                   {Angle::Degrees(east), Angle::Degrees(south)});
}

MbTilesDatabase::MbTilesDatabase(Path path)
  :db(path)
{
  {
    auto stmt = db.CreateStatement("SELECT name, value FROM metadata");
    while (stmt.StepRow()) {
      const auto name = stmt.GetTextColumn(0);
      const auto value = stmt.GetTextColumn(1);
      if (name.empty() || value.empty())
        continue;

      if (name == "format")
        metadata.format = value.data();
      else if (name == "bounds")
        metadata.bounds = ParseBounds(value);
      else if (name == "minzoom")
        metadata.min_zoom = strtoul(value.data(), nullptr, 10);
      else if (name == "maxzoom")
        metadata.max_zoom = strtoul(value.data(), nullptr, 10);
    }
  }

  if (metadata.max_zoom == 0 && metadata.min_zoom == 0) {
    auto stmt = db.CreateStatement("SELECT MIN(zoom_level), MAX(zoom_level) FROM tiles");
    if (stmt.StepRow()) {
      metadata.min_zoom = stmt.GetIntColumn(0);
      metadata.max_zoom = stmt.GetIntColumn(1);
    }
  }
}

bool
MbTilesDatabase::HasTile(unsigned zoom, unsigned column, unsigned row) const
{
  auto stmt = db.CreateStatement("SELECT 1 FROM tiles "
                                 "WHERE zoom_level=? AND tile_column=? AND tile_row=?");
  stmt.BindInt(1, zoom);
  stmt.BindInt(2, column);
  stmt.BindInt(3, row);
  return stmt.StepRow();
}

std::vector<std::byte>
MbTilesDatabase::LoadTile(unsigned zoom, unsigned column, unsigned row) const
{
  auto stmt = db.CreateStatement("SELECT tile_data FROM tiles "
                                 "WHERE zoom_level=? AND tile_column=? AND tile_row=?");
  stmt.BindInt(1, zoom);
  stmt.BindInt(2, column);
  stmt.BindInt(3, row);

  if (!stmt.StepRow())
    throw std::runtime_error("MBTiles tile not found");

  const auto *data = stmt.GetBlobColumn(0);
  const int size = stmt.GetBytesColumn(0);
  return std::vector<std::byte>(data, data + size);
}

} // namespace EDL
