#ifndef VIDEO_RENDER_DEFINES_H
#define VIDEO_RENDER_DEFINES_H

// Enums
enum VideoRenderType {
  kRenderExternal = 0,  // External
  kRenderWindows = 1,   // Windows
  kRenderCocoa = 2,     // Mac
  kRenderCarbon = 3,
  kRenderiOS = 4,      // iPhone
  kRenderAndroid = 5,  // Android
  kRenderX11 = 6,      // Linux
  kRenderDefault
};

#endif
