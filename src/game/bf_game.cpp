#pragma once

const char* GetWindowTitle() {
  return "The Game"
#if BF_DEBUG
         " [DEBUG]"
#endif
    ;
}

UpdateFunctionResult GameUpdate() {
  DrawTexture({
    .texId = 0,
    // .rotation   =0,
    // .pos        ={},
    // .anchor     =Vector2Half(),
    // .scale      =,
    // .sourceSize =,
    // .color      =,
  });
  return UpdateFunctionResult_CONTINUE;
}
