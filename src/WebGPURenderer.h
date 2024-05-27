#pragma once

// Copyright 2021 The Emscripten Authors.  All rights reserved.
// Emscripten is available under two separate licenses, the MIT license and the
// University of Illinois/NCSA Open Source License.  Both these licenses can be
// found in the LICENSE file.

// Based on https://github.com/kainino0x/webgpu-cross-platform-demo


#include <emscripten.h>
#include <emscripten/html5.h>
#include <emscripten/html5_webgpu.h>
#include <webgpu/webgpu_cpp.h>

#undef NDEBUG

#include <array>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <functional>


using AdapterAndDeviceCallback = std::function<void(wgpu::Device)>;

void GetDevice(AdapterAndDeviceCallback callback);

void init();

// The depth stencil attachment isn't really needed to draw the triangle
// and doesn't really affect the render result.
// But having one should give us a slightly better test coverage for the compile of the depth stencil descriptor.
void render(wgpu::TextureView view, wgpu::TextureView depthStencilView);

void frame();

void run(const std::string& canvasId, uint32_t width, uint32_t height);

void* initWebGPURenderer(void *args);
