#pragma once

#include <cstddef>
#include <cstdint>

#include "activities/ActivityManager.h"

// Function pointer (not std::function) so AppEntry stays a trivial, flash-resident
// POD: no vtable, no heap-allocated closure. Appropriate for a 380KB-RAM device.
using LaunchFn = void (*)();

struct AppEntry {
  const char* label;     // English display label shown in the launcher (uppercase, ASCII)
  const uint8_t* icon;   // 1-bit bitmap for GfxRenderer::drawIcon(); nullptr => text-only row
  bool enabled;           // false => selecting the entry shows a "coming soon" toast instead of launching
  LaunchFn create;        // nullptr when enabled == false
};

inline void launchBookshelf() { activityManager.goToBookshelf(); }
inline void launchFileTransfer() { activityManager.goToFileTransfer(); }
inline void launchSettings() { activityManager.goToSettings(); }
inline void launchSlideshow() { activityManager.goToSlideshow(); }

// Registration order == display order in the launcher list. Labels are
// deliberately ASCII (the launcher renders with the scalable built-in fonts;
// see LauncherActivity.cpp).
inline constexpr AppEntry APP_REGISTRY[] = {
    {"LIBRARY", nullptr, true, &launchBookshelf},
    {"NOTES", nullptr, false, nullptr},
    {"WEATHER", nullptr, false, nullptr},
    {"HABITS", nullptr, false, nullptr},
    {"SLIDESHOW", nullptr, true, &launchSlideshow},
    {"SUDOKU", nullptr, false, nullptr},
    {"VAULT SYNC", nullptr, false, nullptr},
    {"FILE TRANSFER", nullptr, true, &launchFileTransfer},
    {"SETTINGS", nullptr, true, &launchSettings},
};

inline constexpr int APP_REGISTRY_COUNT = sizeof(APP_REGISTRY) / sizeof(APP_REGISTRY[0]);
