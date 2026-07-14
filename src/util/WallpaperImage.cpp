#include "WallpaperImage.h"

#include <FsHelpers.h>
#include <GfxRenderer.h>
#include <HalStorage.h>
#include <JpegToBmpConverter.h>
#include <Logging.h>

#include <Epub/converters/ImageDecoderFactory.h>

#include <algorithm>
#include <cstdio>

namespace WallpaperImage {

bool hasSupportedExtension(const std::string& fileName) {
  return FsHelpers::hasBmpExtension(fileName) || FsHelpers::hasPngExtension(fileName) ||
         FsHelpers::hasJpgExtension(fileName);
}

bool ensureJpegBmpCache(const std::string& jpegPath, const int maxWidth, const int maxHeight,
                        std::string& bmpPathOut) {
  FsFile jpegFile;
  if (!Storage.openFileForRead("WIMG", jpegPath, jpegFile)) {
    return false;
  }
  const uint32_t srcSize = static_cast<uint32_t>(jpegFile.size());

  const auto slash = jpegPath.find_last_of('/');
  const std::string base = slash == std::string::npos ? jpegPath : jpegPath.substr(slash + 1);
  char suffix[48];
  snprintf(suffix, sizeof(suffix), "_%u_%dx%d.bmp", static_cast<unsigned>(srcSize), maxWidth, maxHeight);
  bmpPathOut = std::string("/.crosspoint/wallpaper/") + base + suffix;

  if (Storage.exists(bmpPathOut.c_str())) {
    jpegFile.close();
    return true;
  }

  Storage.mkdir("/.crosspoint");
  Storage.mkdir("/.crosspoint/wallpaper");
  FsFile bmpFile;
  if (!Storage.openFileForWrite("WIMG", bmpPathOut, bmpFile)) {
    jpegFile.close();
    return false;
  }
  const bool ok = JpegToBmpConverter::jpegFileToBmpStreamWithSize(jpegFile, bmpFile, maxWidth, maxHeight);
  jpegFile.close();
  bmpFile.close();
  if (!ok) {
    Storage.remove(bmpPathOut.c_str());
    LOG_ERR("WIMG", "JPEG->BMP conversion failed: %s", jpegPath.c_str());
    return false;
  }
  LOG_DBG("WIMG", "Cached JPEG as BMP: %s", bmpPathOut.c_str());
  return true;
}

bool renderPngFitted(GfxRenderer& renderer, const std::string& pngPath, const int screenW, const int screenH) {
  ImageToFramebufferDecoder* decoder = ImageDecoderFactory::getDecoder(pngPath);
  if (!decoder) {
    return false;
  }
  ImageDimensions dims{};
  if (!decoder->getDimensions(pngPath, dims) || dims.width <= 0 || dims.height <= 0) {
    LOG_ERR("WIMG", "Failed to read PNG dimensions: %s", pngPath.c_str());
    return false;
  }

  int outW = dims.width;
  int outH = dims.height;
  if (outW > screenW || outH > screenH) {
    const float scale =
        std::min(static_cast<float>(screenW) / static_cast<float>(outW), static_cast<float>(screenH) / static_cast<float>(outH));
    outW = std::max(1, static_cast<int>(outW * scale));
    outH = std::max(1, static_cast<int>(outH * scale));
  }

  RenderConfig config;
  config.x = (screenW - outW) / 2;
  config.y = (screenH - outH) / 2;
  config.maxWidth = outW;
  config.maxHeight = outH;
  config.useGrayscale = false;  // 1-bit dithered; caller does a plain BW refresh
  config.useDithering = true;
  config.useExactDimensions = true;
  return decoder->decodeToFramebuffer(pngPath, renderer, config);
}

}  // namespace WallpaperImage
