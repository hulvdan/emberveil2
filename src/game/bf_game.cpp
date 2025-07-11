#pragma once

const char* GetWindowTitle() {
  return "The Game"
#if BF_DEBUG
         " [DEBUG]"
#endif
    ;
}

UpdateFunctionResult GameUpdate() {
  auto texId = glib->background_texture_id();
  auto tex   = glib->atlas_textures()->Get(texId);

  Vector2 size{(f32)tex->size_x(), (f32)tex->size_y()};
  auto    ratio = size * ScaleToFit(size, ge.meta.screenSize);
  ratio /= ge.meta.screenSize;

  DrawTexture({
    .texId = texId,
    // .pos   = {0, 200},
    .scale = ratio,
  });
  // .rotation   =0,
  // .pos        ={},
  // .anchor     =Vector2Half(),
  // .scale      =,
  // .sourceSize =,
  // .color      =,
  return UpdateFunctionResult_CONTINUE;
}
