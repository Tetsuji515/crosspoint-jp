#include "SlideshowActivity.h"

#include <Bitmap.h>
#include <FsHelpers.h>
#include <GfxRenderer.h>
#include <HalStorage.h>
#include <I18n.h>

#include <algorithm>
#include <cmath>

#include "MappedInputManager.h"
#include "activities/ActivityManager.h"
#include "components/UITheme.h"
#include "fontIds.h"
#include "util/MemLog.h"
#include "util/WallpaperImage.h"

namespace {
constexpr const char* SLIDESHOW_DIR = "/slideshow";

// Matches CrossPointSettings::SLIDESHOW_INTERVAL order.
constexpr unsigned long INTERVAL_MS[] = {0 /*manual*/, 30000, 60000, 300000, 900000};

StrId intervalLabel(uint8_t interval) {
  switch (interval) {
    case CrossPointSettings::SLIDESHOW_30_SEC: return StrId::STR_SEC_30;
    case CrossPointSettings::SLIDESHOW_1_MIN:  return StrId::STR_MIN_1;
    case CrossPointSettings::SLIDESHOW_5_MIN:  return StrId::STR_MIN_5;
    case CrossPointSettings::SLIDESHOW_15_MIN: return StrId::STR_MIN_15;
    default:                                   return StrId::STR_MANUAL;
  }
}
}  // namespace

void SlideshowActivity::buildFileList() {
  files.clear();
  auto dir = Storage.open(SLIDESHOW_DIR);
  if (!dir || !dir.isDirectory()) {
    return;
  }
  char name[500];
  for (auto file = dir.openNextFile(); file; file = dir.openNextFile()) {
    if (file.isDirectory()) {
      continue;
    }
    file.getName(name, sizeof(name));
    std::string filename(name);
    if (filename[0] == '.' || !WallpaperImage::hasSupportedExtension(filename)) {
      continue;
    }
    // Only BMP headers are cheap enough to validate up front; PNG/JPEG are
    // validated by their decoders at render time.
    if (FsHelpers::hasBmpExtension(filename)) {
      Bitmap bitmap(file);
      if (bitmap.parseHeaders() != BmpReaderError::Ok) {
        LOG_DBG("SLIDE", "Skipping invalid BMP: %s", name);
        continue;
      }
    }
    files.emplace_back(std::move(filename));
  }
  std::sort(files.begin(), files.end());
  LOG_DBG("SLIDE", "%d slideshow images in %s", static_cast<int>(files.size()), SLIDESHOW_DIR);
}

unsigned long SlideshowActivity::intervalMs() const {
  const uint8_t i = SETTINGS.slideshowInterval;
  return i < CrossPointSettings::SLIDESHOW_INTERVAL_COUNT ? INTERVAL_MS[i] : 0;
}

void SlideshowActivity::advance(const int step) {
  if (files.empty()) {
    return;
  }
  currentIndex = (currentIndex + step % static_cast<int>(files.size()) + static_cast<int>(files.size())) %
                 static_cast<int>(files.size());
  lastAdvanceMs = millis();
  requestUpdate();
}

void SlideshowActivity::autoAdvance() {
  if (files.size() > 1 && SETTINGS.slideshowOrder == CrossPointSettings::SLIDESHOW_ORDER_RANDOM) {
    // Random order: jump to any other image.
    int next = currentIndex;
    while (next == currentIndex) {
      next = static_cast<int>(random(static_cast<long>(files.size())));
    }
    currentIndex = next;
    lastAdvanceMs = millis();
    requestUpdate();
  } else {
    advance(1);
  }
}

void SlideshowActivity::onEnter() {
  Activity::onEnter();
  MemLog::log("slideshow_onEnter");
  buildFileList();
  currentIndex = 0;
  lastAdvanceMs = millis();
  requestUpdate();
}

void SlideshowActivity::closeMenuAndSave() {
  menuVisible = false;
  if (settingsDirty) {
    settingsDirty = false;
    SETTINGS.saveToFile();
  }
  requestUpdate();
}

void SlideshowActivity::loop() {
  if (menuVisible) {
    buttonNavigator.onNext([this] {
      menuIndex = ButtonNavigator::nextIndex(menuIndex, MENU_ITEM_COUNT);
      requestUpdate();
    });
    buttonNavigator.onPrevious([this] {
      menuIndex = ButtonNavigator::previousIndex(menuIndex, MENU_ITEM_COUNT);
      requestUpdate();
    });
    if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
      switch (menuIndex) {
        case 0:
          SETTINGS.slideshowOrder =
              (SETTINGS.slideshowOrder + 1) % CrossPointSettings::SLIDESHOW_ORDER_COUNT;
          settingsDirty = true;
          requestUpdate();
          break;
        case 1:
          SETTINGS.slideshowInterval =
              (SETTINGS.slideshowInterval + 1) % CrossPointSettings::SLIDESHOW_INTERVAL_COUNT;
          lastAdvanceMs = millis();  // restart the timer from the change
          settingsDirty = true;
          requestUpdate();
          break;
        default:
          if (settingsDirty) {
            SETTINGS.saveToFile();
          }
          activityManager.goHome();
          break;
      }
      return;
    }
    if (mappedInput.wasReleased(MappedInputManager::Button::Back)) {
      closeMenuAndSave();
    }
    return;
  }

  if (mappedInput.wasReleased(MappedInputManager::Button::Back)) {
    if (settingsDirty) {
      SETTINGS.saveToFile();
    }
    activityManager.goHome();
    return;
  }

  if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
    menuVisible = true;
    menuIndex = 0;
    requestUpdate();
    return;
  }

  buttonNavigator.onNext([this] { advance(1); });
  buttonNavigator.onPrevious([this] { advance(-1); });

  const unsigned long interval = intervalMs();
  if (interval > 0 && !files.empty() && millis() - lastAdvanceMs >= interval) {
    autoAdvance();
  }
}

void SlideshowActivity::renderImage() const {
  const auto pageWidth = renderer.getScreenWidth();
  const auto pageHeight = renderer.getScreenHeight();

  std::string path = std::string(SLIDESHOW_DIR) + "/" + files[currentIndex];

  // PNG: streaming decode straight into the framebuffer, fitted + centered.
  if (FsHelpers::hasPngExtension(path)) {
    if (!WallpaperImage::renderPngFitted(renderer, path, pageWidth, pageHeight)) {
      renderer.drawCenteredText(UI_12_FONT_ID, pageHeight / 2, files[currentIndex].c_str());
    }
    return;
  }

  // JPEG: convert once to a cached BMP, then fall through to the BMP path.
  if (FsHelpers::hasJpgExtension(path)) {
    std::string bmpPath;
    if (!WallpaperImage::ensureJpegBmpCache(path, pageWidth, pageHeight, bmpPath)) {
      renderer.drawCenteredText(UI_12_FONT_ID, pageHeight / 2, files[currentIndex].c_str());
      return;
    }
    path = bmpPath;
  }

  FsFile file;
  if (!Storage.openFileForRead("SLIDE", path, file)) {
    renderer.drawCenteredText(UI_12_FONT_ID, pageHeight / 2, files[currentIndex].c_str());
    return;
  }
  Bitmap bitmap(file, true);
  if (bitmap.parseHeaders() != BmpReaderError::Ok) {
    renderer.drawCenteredText(UI_12_FONT_ID, pageHeight / 2, tr(STR_NO_SLIDESHOW_IMAGES));
    file.close();
    return;
  }

  // Fit and center (same approach as BmpViewerActivity).
  int x, y;
  if (bitmap.getWidth() > pageWidth || bitmap.getHeight() > pageHeight) {
    const float ratio = static_cast<float>(bitmap.getWidth()) / static_cast<float>(bitmap.getHeight());
    const float screenRatio = static_cast<float>(pageWidth) / static_cast<float>(pageHeight);
    if (ratio > screenRatio) {
      x = 0;
      y = std::round((static_cast<float>(pageHeight) - static_cast<float>(pageWidth) / ratio) / 2);
    } else {
      x = std::round((static_cast<float>(pageWidth) - static_cast<float>(pageHeight) * ratio) / 2);
      y = 0;
    }
  } else {
    x = (pageWidth - bitmap.getWidth()) / 2;
    y = (pageHeight - bitmap.getHeight()) / 2;
  }
  renderer.drawBitmap(bitmap, x, y, pageWidth, pageHeight, 0, 0);
  file.close();
}

void SlideshowActivity::renderMenu() const {
  const auto pageWidth = renderer.getScreenWidth();
  const auto pageHeight = renderer.getScreenHeight();
  const int lineHeight = renderer.getLineHeight(UI_12_FONT_ID);
  constexpr int rowPad = 10;
  const int rowH = lineHeight + rowPad;
  constexpr int panelW = 320;
  const int panelH = MENU_ITEM_COUNT * rowH + 24;
  const int panelX = (pageWidth - panelW) / 2;
  const int panelY = (pageHeight - panelH) / 2;

  renderer.fillRect(panelX, panelY, panelW, panelH, false);
  renderer.drawRect(panelX, panelY, panelW, panelH, 2, true);

  char valueBuf[64];
  for (int i = 0; i < MENU_ITEM_COUNT; i++) {
    const int rowY = panelY + 12 + i * rowH;
    const bool selected = i == menuIndex;
    if (selected) {
      renderer.fillRect(panelX + 6, rowY - rowPad / 2, panelW - 12, rowH, true);
    }
    const bool black = !selected;
    switch (i) {
      case 0:
        snprintf(valueBuf, sizeof(valueBuf), "%s: %s", tr(STR_SLIDESHOW_ORDER),
                 I18N.get(SETTINGS.slideshowOrder == CrossPointSettings::SLIDESHOW_ORDER_RANDOM
                              ? StrId::STR_RANDOM
                              : StrId::STR_FILENAME));
        break;
      case 1:
        snprintf(valueBuf, sizeof(valueBuf), "%s: %s", tr(STR_SLIDESHOW_INTERVAL),
                 I18N.get(intervalLabel(SETTINGS.slideshowInterval)));
        break;
      default:
        snprintf(valueBuf, sizeof(valueBuf), "%s", tr(STR_EXIT));
        break;
    }
    renderer.drawText(UI_12_FONT_ID, panelX + 20, rowY, valueBuf, black);
  }
}

void SlideshowActivity::render(RenderLock&&) {
  renderer.clearScreen();
  if (files.empty()) {
    const auto pageHeight = renderer.getScreenHeight();
    renderer.drawCenteredText(UI_12_FONT_ID, pageHeight / 2, tr(STR_NO_SLIDESHOW_IMAGES));
    const auto labels = mappedInput.mapLabels(tr(STR_BACK), "", "", "");
    GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);
    renderer.displayBuffer(HalDisplay::HALF_REFRESH);
    return;
  }
  renderImage();
  if (menuVisible) {
    renderMenu();
  }
  renderer.displayBuffer(HalDisplay::HALF_REFRESH);
}
