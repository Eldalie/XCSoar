// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Atmosphere/Pressure.hpp"

namespace EDL {

static constexpr unsigned NUM_ISOBARS = 5;
static constexpr unsigned ISOBARS[NUM_ISOBARS] = {
  50000,
  60000,
  70000,
  80000,
  90000,
};

[[gnu::pure]]
bool
IsSupportedIsobar(unsigned isobar) noexcept;

[[gnu::pure]]
unsigned
ResolveLevel(AtmosphericPressure qnh, bool qnh_available,
             double altitude) noexcept;

} // namespace EDL
