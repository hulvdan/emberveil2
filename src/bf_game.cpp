#pragma once

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

struct Texture2D {
  int   w    = {};
  int   h    = {};
  void* data = {};
};

Texture2D LoadTexture(const char* path) {
  Texture2D result{};
  int       channels = 0;
  result.data        = stbi_load(path, &result.w, &result.h, &channels, 4);
  ASSERT(result.data);
  return result;
}

void UnloadTexture(Texture2D* texture) {
  stbi_image_free(texture->data);
  texture->data = nullptr;
}

struct GameData {
  Texture2D tex = {};
} g = {};

void Draw() {
  const auto texturePath = "c:/Users/user/dev/home/isaac/assets/tex.png";

  LOCAL_PERSIST bool loaded = false;

  if (!loaded) {
    loaded = true;

    g.tex = LoadTexture(texturePath);
  }
}
