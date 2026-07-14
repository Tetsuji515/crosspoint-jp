#pragma once

#include <string>
#include <vector>

#include "../Activity.h"
#include "CrossPointSettings.h"
#include "util/ButtonNavigator.h"

// Full-screen photo-frame slideshow over the BMPs in /slideshow (SD root,
// same convention as the /sleep wallpaper folder). Up/Down step manually,
// Confirm opens an overlay menu (order / interval / exit), Back exits to the
// launcher. With a timed interval the device is kept awake (photo-frame use;
// X4 cannot timer-wake from its full-power-off sleep), manual mode follows
// the normal auto-sleep rules.
class SlideshowActivity final : public Activity {
  ButtonNavigator buttonNavigator;

  std::vector<std::string> files;  // valid BMPs in /slideshow, filename-sorted
  int currentIndex = 0;
  unsigned long lastAdvanceMs = 0;
  bool settingsDirty = false;  // save SETTINGS once on menu close/exit only

  // Overlay menu state
  bool menuVisible = false;
  int menuIndex = 0;
  static constexpr int MENU_ITEM_COUNT = 3;  // order, interval, exit

  void buildFileList();
  void advance(int step);
  void autoAdvance();
  unsigned long intervalMs() const;
  void closeMenuAndSave();
  void renderImage() const;
  void renderMenu() const;

 public:
  explicit SlideshowActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
      : Activity("Slideshow", renderer, mappedInput) {}
  void onEnter() override;
  void loop() override;
  void render(RenderLock&&) override;
  bool supportsLandscape() const override { return true; }
  bool preventAutoSleep() override {
    return SETTINGS.slideshowInterval != CrossPointSettings::SLIDESHOW_MANUAL && !files.empty();
  }
};
