#pragma once

#include <RNS/types.h>

#if USE_IMGUI
#define GLEW_STATIC
#include <GL/glew.h>
#include <imgui_impl_sdl.h>
#include <imgui_impl_opengl3.h>
#include <SDL.h>
#endif

void imgui_init();
