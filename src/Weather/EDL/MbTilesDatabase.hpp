// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Sqlite.hpp"
#include "Geo/GeoBounds.hpp"
#include "util/StaticString.hxx"

#include <cstddef>
#include <vector>

namespace EDL {

struct MbTilesMetadata {
  StaticString<32> format;
  GeoBounds bounds = GeoBounds::Invalid();
  unsigned min_zoom = 0;
  unsigned max_zoom = 0;
};

class MbTilesDatabase {
  SqliteDatabase db;
  MbTilesMetadata metadata;

public:
  explicit MbTilesDatabase(Path path);
  MbTilesDatabase(MbTilesDatabase &&other) noexcept = default;
  MbTilesDatabase &operator=(MbTilesDatabase &&other) noexcept = default;
  ~MbTilesDatabase() noexcept = default;

  MbTilesDatabase(const MbTilesDatabase &) = delete;
  MbTilesDatabase &operator=(const MbTilesDatabase &) = delete;

  const MbTilesMetadata &GetMetadata() const noexcept {
    return metadata;
  }

  [[gnu::pure]]
  bool
  HasTile(unsigned zoom, unsigned column, unsigned row) const;

  std::vector<std::byte>
  LoadTile(unsigned zoom, unsigned column, unsigned row) const;
};

} // namespace EDL
