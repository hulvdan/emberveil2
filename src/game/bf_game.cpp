#pragma once

const char* GetWindowTitle() {
  return "The Game"
#if BF_DEBUG
         " [DEBUG]"
#endif
    ;
}

SDL_AppResult GameUpdate() {
  auto backgroundTexId = glib->background_texture_id();
  auto backgroundTex   = glib->atlas_textures()->Get(backgroundTexId);

  Vector2 size{(f32)backgroundTex->size_x(), (f32)backgroundTex->size_y()};
  auto    scale = ScaleToFit(size, LOGICAL_RESOLUTION);

  DrawTexture({
    .texId = backgroundTexId,
    .pos   = LOGICAL_RESOLUTION / 2,
    .scale = Vector2One() * scale,
  });

  DrawTexture({
    .texId = glib->player_texture_id(),
    .pos   = Vector2((f32)LOGICAL_RESOLUTION.x / 2, 0),
  });

  // .rotation   =0,
  // .pos        ={},
  // .anchor     =Vector2Half(),
  // .scale      =,
  // .sourceSize =,
  // .color      =,
  return SDL_APP_CONTINUE;
}
