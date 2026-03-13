// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Weather/EDL/Levels.hpp"
#include "Weather/EDL/Store.hpp"
#include "Atmosphere/Pressure.hpp"
#include "time/BrokenDateTime.hpp"

#include "tap.h"

#include <cstring>

static void
TestRequest()
{
  const BrokenDateTime forecast(2026, 3, 12, 6, 0, 0);
  const EDL::TileRequest request(forecast, 90000);

  const auto parameter = request.BuildForecastParameter();
  ok1(strcmp(parameter.c_str(), "2026-03-12+06:00:00") == 0);

  const auto url = request.BuildDownloadUrl();
  ok1(strcmp(url.c_str(),
             "https://www.edl-soaring.com/mbtiles/extract_mbtiles_from_date.php?fdate=2026-03-12+06:00:00&isobare=90000") == 0);

  const auto filename = EDL::TileRequest(forecast, 70000).BuildCacheFileName();
  ok1(strcmp(filename.c_str(), "20260312_060000_70000.mbtiles") == 0);

  ok1(EDL::IsSupportedIsobar(50000));
  ok1(!EDL::IsSupportedIsobar(85000));
}

static void
TestLevelResolver()
{
  const auto standard = EDL::ResolveLevel(AtmosphericPressure::Standard(),
                                          false, 1000);
  ok1(standard == 90000);

  const auto high_altitude = EDL::ResolveLevel(AtmosphericPressure::Standard(),
                                               false, 3000);
  ok1(high_altitude == 70000);

  const auto qnh_adjusted = EDL::ResolveLevel(AtmosphericPressure::HectoPascal(1030),
                                              true, 2500);
  ok1(qnh_adjusted == 80000);
}

int
main()
{
  plan_tests(8);

  TestRequest();
  TestLevelResolver();

  return exit_status();
}
