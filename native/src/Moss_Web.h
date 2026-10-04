#pragma once

#include <Moss/Moss_stdinc.h>

#if defined(MOSS_PLATFORM_WASM)

/* WebAssembly/browser build marker used by Moss Web platform sources. */
#ifndef MOSS_PLATFORM_WASM
#  define MOSS_PLATFORM_WASM 1
#endif
#ifndef MOSS_PLATFORM_WEB
#  define MOSS_PLATFORM_WEB 1
#endif

/* Pull in the real Moss standard header. The package's CMake include order
 * puts the Moss source tree after this package, so this resolves upstream. */
#include <Moss/Moss_stdinc.h>

/* The current upstream header contains several malformed allocation macros.
 * Undefine and replace them with valid macro bodies for Web/WASM builds. */
#ifdef MOSS_CALLOC
#  undef MOSS_CALLOC
#endif
#define MOSS_CALLOC(nmemb, size) calloc((nmemb), (size))

#ifdef MOSS_REALLOC
#  undef MOSS_REALLOC
#endif
#define MOSS_REALLOC(ptr, size) realloc((ptr), (size))

#ifdef MOSS_ALIGNED_ALLOC
#  undef MOSS_ALIGNED_ALLOC
#endif
#define MOSS_ALIGNED_ALLOC(ptr, alignment, size) aligned_alloc((alignment), (size))

#ifdef MOSS_ALIGNED_FREE
#  undef MOSS_ALIGNED_FREE
#endif
#define MOSS_ALIGNED_FREE(mem) Moss_Free((mem))

namespace MossWeb {

void SetCanvas(const char* selector);
void SetCanvasSize(int width, int height);
void ResizeCanvasToDisplaySize();

int GetCanvasWidth();
int GetCanvasHeight();
float GetDevicePixelRatio();

void RequestFullscreen();
void ExitFullscreen();

void RequestPointerLock();
void ExitPointerLock();

bool IsPointerLocked();
bool IsFullscreen();

void SetCanvasCursor(const char* cursor);
void SetCanvasFocus();
bool IsCanvasFocused();

bool CreateWebGLContext();
void DestroyWebGLContext();
bool MakeWebGLContextCurrent();

void BeginFrame();
void EndFrame();

}

#endif
