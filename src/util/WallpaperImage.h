#pragma once

#include <string>

class GfxRenderer;

// Shared image handling for the sleep-screen wallpaper (/sleep) and the
// slideshow app (/slideshow): BMP renders natively, PNG streams through the
// EPUB pipeline's PNG decoder, and JPEG is converted once to a BMP cached
// under /.crosspoint/wallpaper (picojpeg-based converter -- the JPEGDEC
// direct path has known scaling artifacts, see ImageBlock.cpp).
namespace WallpaperImage {

// .bmp / .png / .jpg / .jpeg
bool hasSupportedExtension(const std::string& fileName);

// For JPEG sources: ensure a fitted BMP cache exists and return its path.
// The cache name encodes source size and target box, so replacing the file
// or rotating the screen invalidates it naturally. Returns false when the
// conversion fails (the broken cache file is removed).
bool ensureJpegBmpCache(const std::string& jpegPath, int maxWidth, int maxHeight, std::string& bmpPathOut);

// Decode a PNG scaled to fit (and centered in) screenW x screenH, 1-bit
// dithered, directly into the framebuffer. The caller displays the buffer.
bool renderPngFitted(GfxRenderer& renderer, const std::string& pngPath, int screenW, int screenH);

}  // namespace WallpaperImage
