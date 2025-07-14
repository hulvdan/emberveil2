// cppcheck-suppress-file unusedFunction

#pragma once

GLOBAL_VAR u32 g_randState = 0;

// Based off splitMix32
u32 Rand() {
  u32 z = g_randState + 0x9e3779b9;
  z ^= z >> 15;
  z *= 0x85ebca6b;
  z ^= z >> 13;
  z *= 0xc2b2ae35;
  g_randState = z;  // NOTE: Added by me. Should i just increment g_randState?
  u32 result  = z ^ (z >> 16);
  return result;
}

// [0; 1)
f32 FRand() {
  return (f32)((f64)Rand() / (f64)((u64)u32_max + 1));
}

// [a; b]
int RandInt(int a, int b) {
  ASSERT(a <= b);
  auto r = (int)(FRand() * (f32)(b - a + 1));
  ASSERT(r >= 0);
  return a + r;
}

int RandInt(uint a, uint b) {
  ASSERT(a <= b);
  int r = (int)(FRand() * (f32)(b - a + 1));
  ASSERT(r >= 0);
  return (int)a + r;
}

int RandInt(int b) {
  return MIN((int)(FRand() * b), b);
}

int RandInt(uint b) {
  auto r = FRand() * (f32)b;
  ASSERT(r >= 0);
  return MIN((int)r, (int)b);
}

TEST_CASE ("RandInt") {
  ASSERT(RandInt(1) <= 1);
  ASSERT(RandInt(1) <= 1);
  ASSERT(RandInt(1) <= 1);
  ASSERT(RandInt(1) <= 1);
  ASSERT(RandInt(1) <= 1);
  ASSERT(RandInt(1) <= 1);
  ASSERT(RandInt(1) <= 1);
  ASSERT(RandInt(1) <= 1);
}

struct PerlinParams {
  int octaves;
  f32 smoothness;
};

/*
void CycledPerlin2D(
  View<u16>    output,
  Arena*       trashArena,
  PerlinParams params,
  Vector2Int   size
) {
  ASSERT(output.count == size.x * size.y);

  int sxPower = 0;
  int syPower = 0;

  ASSERT(size.x > 0);
  ASSERT(size.y > 0);
  {
    auto sxIs = IsPowerOf2(size.x, &sxPower);
    auto syIs = IsPowerOf2(size.y, &syPower);
    ASSERT(sxIs);
    ASSERT(syIs);
  }

  ASSERT(params.octaves > 0);
  ASSERT(params.smoothness > 0);

  TEMP_USAGE(trashArena);

  auto octaves = params.octaves;

  auto totalPixels = (u32)(size.x * size.y);
  auto cover       = ALLOCATE_ARRAY(trashArena, f32, totalPixels);
  auto accumulator = ALLOCATE_ARRAY(trashArena, f32, totalPixels);

  FOR_RANGE (u64, i, (u64)totalPixels) {
    *(cover + i)       = FRand();
    *(accumulator + i) = 0;
  }

  f32 sumOfDivision = 0;
  octaves           = MIN(sxPower, octaves);

  int offset = size.x;

  f32 octaveC = 1.0f;
  FOR_RANGE (int, _, octaves) {
    sumOfDivision += octaveC;

    int x0Index = 0;
    int x1Index = offset % size.x;
    int y0Index = 0;
    int y1Index = offset % size.y;

    int yit = 0;
    int xit = 0;
    FOR_RANGE (int, y, size.y) {
      int y0s = size.y * y0Index;
      int y1s = size.y * y1Index;

      FOR_RANGE (int, x, size.x) {
        if (xit == offset) {
          x0Index = x1Index;
          x1Index = (x1Index + offset) % size.x;
          xit     = 0;
        }

        auto a0 = *(cover + y0s + x0Index);
        auto a1 = *(cover + y0s + x1Index);
        auto a2 = *(cover + y1s + x0Index);
        auto a3 = *(cover + y1s + x1Index);

        auto xb      = (f32)xit / (f32)offset;
        auto yb      = (f32)yit / (f32)offset;
        auto blend01 = Lerp(a0, a1, xb);
        auto blend23 = Lerp(a2, a3, xb);
        auto value   = octaveC * Lerp(blend01, blend23, yb);

        *(accumulator + size.x * y + x) += value;
        xit++;
      }

      yit++;
      if (yit == offset) {
        y0Index = y1Index;
        y1Index = (y1Index + offset) % size.y;
        yit     = 0;
      }
    }

    offset >>= 1;
    octaveC /= params.smoothness;
  }

  FOR_RANGE (int, y, size.y) {
    FOR_RANGE (int, x, size.x) {
      f32 t = *(accumulator + y * size.y + x) / sumOfDivision;
      ASSERT(t >= 0);
      ASSERT(t <= 1.0f);

      u16 value = (u16)((f32)u16_max * t);

      output[y * size.x + x] = value;
    }
  }
}
*/
