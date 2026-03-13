// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Levels.hpp"
#include "co/Task.hxx"
#include "time/BrokenDateTime.hpp"
#include "system/Path.hpp"
#include "util/StaticString.hxx"

#include <vector>

class AllocatedPath;
class ProgressListener;
class CurlGlobal;

namespace EDL {

struct CachedDay {
  BrokenDateTime day;
  unsigned file_count;

  [[gnu::pure]]
  bool IsComplete() const noexcept;
};

class TileRequest final {
public:
  const BrokenDateTime forecast;
  const unsigned isobar;

  constexpr TileRequest(const BrokenDateTime &_forecast,
                        unsigned _isobar) noexcept
    :forecast(_forecast), isobar(_isobar) {}

  [[gnu::const]]
  StaticString<32>
  BuildForecastParameter() const noexcept;

  [[gnu::const]]
  StaticString<256>
  BuildDownloadUrl() const noexcept;

  [[gnu::const]]
  StaticString<64>
  BuildCacheFileName() const noexcept;

  AllocatedPath
  BuildCachePath() const noexcept;

  Co::Task<AllocatedPath>
  EnsureDownloaded(CurlGlobal &curl, ProgressListener &progress) const;
};

AllocatedPath
BuildCacheDirectory() noexcept;

Co::Task<unsigned>
EnsureDayDownloaded(BrokenDateTime day, CurlGlobal &curl,
                    ProgressListener &progress);

std::vector<CachedDay>
ListDownloadedDays() noexcept;

unsigned
DeleteOtherDownloadedDays(BrokenDateTime keep_day) noexcept;

} // namespace EDL
