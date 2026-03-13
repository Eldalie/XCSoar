// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Manager.hpp"

#include "Levels.hpp"
#include "Store.hpp"
#include "MbTilesOverlay.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"
#include "MapWindow/GlueMapWindow.hpp"
#include "Profile/Keys.hpp"
#include "Profile/Profile.hpp"
#include "UIGlobals.hpp"
#include "time/Convert.hxx"

#include <algorithm>
#include <cmath>
#include <memory>

namespace EDL {

static BrokenDateTime
ToLocalBrokenDateTime(BrokenDateTime utc) noexcept
{
  const auto tm = LocalTime(utc.ToTimePoint());
  return BrokenDateTime(tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
                        tm.tm_hour, tm.tm_min, tm.tm_sec);
}

static int
GetCurrentAircraftAltitude() noexcept
{
  const auto &basic = CommonInterface::Basic();

  /* The dedicated page should follow the aircraft as closely as the
     available sensors allow, preferring barometric altitude. */
  if (basic.baro_altitude_available)
    return std::max(0, int(std::lround(basic.baro_altitude)));

  if (basic.gps_altitude_available)
    return std::max(0, int(std::lround(basic.gps_altitude)));

  return 3000;
}

void
UpdateCurrentLevel() noexcept
{
  auto &state = CommonInterface::SetUIState().weather.edl;
  state.isobar = ResolveLevel(CommonInterface::GetComputerSettings().pressure,
                              CommonInterface::GetComputerSettings().pressure_available,
                              GetCurrentAircraftAltitude());
}

void
EnsureInitialised() noexcept
{
  auto &state = CommonInterface::SetUIState().weather.edl;

  /* Overlay mode may enter through the weather dialog without visiting
     the dedicated page first, so make the shared state self-healing. */
  if (!state.forecast_datetime.IsPlausible()) {
    const auto now = BrokenDateTime::NowUTC();
    state.forecast_datetime = BrokenDateTime(now.year, now.month, now.day,
                                             now.hour, 0, 0);
  }

  if (!IsSupportedIsobar(state.isobar))
    UpdateCurrentLevel();

  if (CommonInterface::GetComputerSettings().weather.edl.enabled &&
      state.status == EDLWeatherUIState::Status::DISABLED)
    state.status = EDLWeatherUIState::Status::IDLE;
}

void
ResetForDedicatedPage() noexcept
{
  auto &state = CommonInterface::SetUIState().weather.edl;
  const auto now = BrokenDateTime::NowUTC();
  /* Dedicated EDL pages always start from the current hour/current
     aircraft altitude instead of preserving a stale previous view. */
  state.forecast_datetime = BrokenDateTime(now.year, now.month, now.day,
                                           now.hour, 0, 0);
  UpdateCurrentLevel();

  if (CommonInterface::GetComputerSettings().weather.edl.enabled)
    state.status = EDLWeatherUIState::Status::IDLE;
  else
    state.status = EDLWeatherUIState::Status::DISABLED;
}

bool
ShouldShowOnMainMap() noexcept
{
  return CommonInterface::GetComputerSettings().weather.edl.show_on_main_map;
}

void
SetShowOnMainMap(bool enabled) noexcept
{
  CommonInterface::SetComputerSettings().weather.edl.show_on_main_map = enabled;
  Profile::Set(ProfileKeys::ShowEDLOnMainMap, enabled);
}

void
ClearOverlay() noexcept
{
  if (auto *map = UIGlobals::GetMap())
    map->SetOverlay(nullptr);
}

bool
OverlayEnabled() noexcept
{
  return CommonInterface::GetComputerSettings().weather.edl.enabled;
}

bool
OverlayVisible() noexcept
{
  const auto *map = UIGlobals::GetMap();
  return map != nullptr &&
    dynamic_cast<const MbTilesOverlay *>(map->GetOverlay()) != nullptr;
}

void
SetLoadingStatus() noexcept
{
  auto &state = CommonInterface::SetUIState().weather.edl;
  if (!OverlayEnabled()) {
    state.status = EDLWeatherUIState::Status::DISABLED;
    return;
  }

  state.status = EDLWeatherUIState::Status::LOADING;
}

void
SetIdleStatus() noexcept
{
  auto &state = CommonInterface::SetUIState().weather.edl;
  state.status = OverlayEnabled()
    ? EDLWeatherUIState::Status::IDLE
    : EDLWeatherUIState::Status::DISABLED;
}

void
SetErrorStatus() noexcept
{
  CommonInterface::SetUIState().weather.edl.status = EDLWeatherUIState::Status::ERROR;
}

void
ApplyOverlay(Path path) noexcept
{
  auto &state = CommonInterface::SetUIState().weather.edl;
  /* Reuse the generic overlay HUD by exposing the active EDL state as
     the overlay label instead of showing this mapping only in a widget. */
  const auto label = GetOverlayLabel();
  auto overlay = std::make_unique<MbTilesOverlay>(path, label.c_str());

  if (auto *map = UIGlobals::GetMap()) {
    map->SetOverlay(std::move(overlay));
    map->QuickRedraw();
  }

  state.enabled = true;
  state.status = EDLWeatherUIState::Status::READY;
}

void
RefreshOverlayVisibility() noexcept
{
  if (ShouldShowOnMainMap())
    return;

  ClearOverlay();
}

const char *
GetStatusLabel() noexcept
{
  switch (CommonInterface::GetUIState().weather.edl.status) {
  case EDLWeatherUIState::Status::DISABLED:
    return _("Disabled");
  case EDLWeatherUIState::Status::IDLE:
    return _("Idle");
  case EDLWeatherUIState::Status::LOADING:
    return _("Loading");
  case EDLWeatherUIState::Status::READY:
    return _("Ready");
  case EDLWeatherUIState::Status::ERROR:
    return _("Error");
  }

  return _("Error");
}

BrokenDateTime
GetForecastTime() noexcept
{
  return CommonInterface::GetUIState().weather.edl.forecast_datetime;
}

BrokenDateTime
GetForecastTimeLocal() noexcept
{
  EnsureInitialised();
  return ToLocalBrokenDateTime(GetForecastTime());
}

BrokenDateTime
ToLocalForecastTime(BrokenDateTime forecast) noexcept
{
  return ToLocalBrokenDateTime(forecast);
}

StaticString<64>
GetOverlayLabel() noexcept
{
  EnsureInitialised();

  const auto local = GetForecastTimeLocal();

  StaticString<64> label;
  label.Format("EDL %02u:00 %u hPa %d m",
               unsigned(local.hour),
               GetIsobar() / 100,
               GetAltitude());
  return label;
}

int
GetAltitude() noexcept
{
  return GetAltitudeForIsobar(GetIsobar());
}

unsigned
GetIsobar() noexcept
{
  return CommonInterface::GetUIState().weather.edl.isobar;
}

int
GetAltitudeForIsobar(unsigned isobar) noexcept
{
  const auto pressure = AtmosphericPressure::Pascal(isobar);
  const auto &qnh = CommonInterface::GetComputerSettings().pressure;
  const double altitude =
    CommonInterface::GetComputerSettings().pressure_available && qnh.IsPlausible()
    ? qnh.StaticPressureToQNHAltitude(pressure)
    : AtmosphericPressure::StaticPressureToPressureAltitude(pressure);
  return std::max(0, int(std::lround(altitude)));
}

} // namespace EDL
