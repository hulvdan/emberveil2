// cppcheck-suppress-file unusedFunction

#pragma once

#ifndef PI32
#  define PI32 (3.14159265358979323846f)
#endif
#ifndef PI64
#  define PI64 (3.14159265358979323846)
#endif

bool FloatEquals(f32 v1, f32 v2) {  ///
  return abs(v1 - v2) < 0.00001f;
}

f32 Unlerp(f32 value, f32 rangeMin, f32 rangeMax) {  ///
  ASSERT_IS_NUMBER(value);
  ASSERT_IS_NUMBER(rangeMin);
  ASSERT_IS_NUMBER(rangeMax);

  auto v1 = (value - rangeMin);
  auto v2 = (rangeMax - rangeMin);
  return v1 / v2;
}

TEST_CASE ("Unlerp") {  ///
  auto v = Unlerp(18.0f, 10.0f, 20.0f);
  ASSERT(FloatEquals(v, 0.8f));
}

#define REVOLUTIONS2RAD (3.14159265358979f)
#define REVOLUTIONS2DEG (3.14159265358979f * (RAD2DEG))

#define SIGN(X) ((X) == 0 ? 0 : ((X) < 0 ? -1 : 1))

int Round(f32 value) {  ///
  return lround(value);
}

f32 MoveTowardsF(f32 v, f32 target, f32 maxDistance) {  ///
  ASSERT(maxDistance >= 0);

  if (v > target) {
    v -= maxDistance;
    if (v < target)
      v = target;
  }
  else if (v < target) {
    v += maxDistance;
    if (v > target)
      v = target;
  }

  return v;
}

int MoveTowardsI(int v, int target, int maxDistance) {  ///
  ASSERT(maxDistance >= 0);

  if (v > target) {
    v -= maxDistance;
    if (v < target)
      v = target;
  }
  else if (v < target) {
    v += maxDistance;
    if (v > target)
      v = target;
  }

  return v;
}

TEST_CASE ("MoveTowards") {  ///
  ASSERT(MoveTowardsI(0, 10, 5) == 5);
  ASSERT(MoveTowardsI(-10, 10, 5) == -5);
  ASSERT(MoveTowardsI(10, -10, 5) == 5);
  ASSERT(MoveTowardsI(0, 10, 30) == 10);
  ASSERT(MoveTowardsI(-10, 10, 30) == 10);
  ASSERT(MoveTowardsI(10, -10, 30) == -10);

  ASSERT(MoveTowardsF(0, 10, 5) == 5);
  ASSERT(MoveTowardsF(-10, 10, 5) == -5);
  ASSERT(MoveTowardsF(10, -10, 5) == 5);
  ASSERT(MoveTowardsF(0, 10, 30) == 10);
  ASSERT(MoveTowardsF(-10, 10, 30) == 10);
  ASSERT(MoveTowardsF(10, -10, 30) == -10);
}

//  [0; 360)
f32 NormalizeAngleDeg0360(f32 a) {  ///
  ASSERT_IS_NUMBER(a);
  a = fmodf(a, 360);
  if (a < 0)
    a += 360;
  return a;
}

// [-180; 180)
f32 NormalizeAngleDeg180180(f32 a) {  ///
  ASSERT_IS_NUMBER(a);
  a = fmodf(a + 180, 360);
  if (a < 0)
    a += 360;
  return a - 180;
}

// [-180; 180)
f32 AngleDiffDeg(f32 a, f32 b) {  ///
  ASSERT_IS_NUMBER(a);
  ASSERT_IS_NUMBER(b);
  return NormalizeAngleDeg180180(b - a);
}

f32 BisectAngleDeg(f32 a, f32 b) {  ///
  ASSERT_IS_NUMBER(a);
  ASSERT_IS_NUMBER(b);
  return NormalizeAngleDeg0360(a + AngleDiffDeg(a, b) * 0.5f);
}

bool FloatAngleDegreesEquals(f32 a1, f32 a2) {  ///
  ASSERT_IS_NUMBER(a1);
  ASSERT_IS_NUMBER(a2);
  a1 = NormalizeAngleDeg0360(a1);
  a2 = NormalizeAngleDeg0360(a2);
  return a1 == a2;
}

TEST_CASE ("BisectAngleDeg") {  ///
  const struct {
    f32 a, b, c;
  } r[] = {
    {90.0f, 0.0f, 45.0f},
    {90.0f, 60.0f, 75.0f},
    {180.0f, 2.0f, 91.0f},
    {180.0f, -2.0f, -91.0f},
  };
  for (auto [a, b, c] : r) {
    auto res = BisectAngleDeg(a, b);
    CHECK(FloatAngleDegreesEquals(res, c));
    auto res2 = BisectAngleDeg(a, b);
    CHECK(FloatAngleDegreesEquals(res2, c));
  }
}

f32 MoveTowardsFAngleDeg(f32 a1, f32 a2, f32 maxAngle) {  ///
  ASSERT_IS_NUMBER(a1);
  ASSERT_IS_NUMBER(a2);
  ASSERT_IS_NUMBER(maxAngle);
  ASSERT(maxAngle >= 0);

  auto diff = AngleDiffDeg(a1, a2);
  auto d    = MoveTowardsF(0, diff, maxAngle);
  auto res  = a1 + d;
  return res;
}

TEST_CASE ("MoveTowardsFAngleDeg") {  ///
  const struct {
    int a, b, c, d;
  } r[] = {
    {1, 0, 5, 0},
    {-1, 0, 5, 0},
    {361, 0, 5, 0},
    {-361, 0, 5, 0},
    {90, 0, 5, 85},
    {-90, 0, 5, -85},
    {181, 0, 5, 186},
    {181, 90, 5, 176},
    {179, 0, 5, 174},
    {179, 180, 5, 180},
    {90, 180, 5, 95},
  };
  for (auto [a, b, c, d] : r) {
    auto res = MoveTowardsFAngleDeg((f32)a, (f32)b, (f32)c);
    CHECK(FloatAngleDegreesEquals(res, (f32)d));
  }
}

// ExponentialDecay - это ~Lerp, но без зависимости от framerate-а.
// NOTE: https://www.youtube.com/watch?v=LSNQuFEDOyQ
f32 ExponentialDecay(f32 a, f32 b, f32 decay, f32 dt) {  ///
  return b + (a - b) * expf(-decay * dt);
}

int Floor(int number, int factor) {  ///
  int result = (number / factor) * factor;
  if (number < 0 && number % factor != 0) {
    result -= factor;
  }
  return result;
}

TEST_CASE ("Floor") {  ///
  ASSERT(Floor(0, 20) == 0);
  ASSERT(Floor(1, 20) == 0);
  ASSERT(Floor(-1, 20) == -20);
  ASSERT(Floor(20, 20) == 20);
  ASSERT(Floor(21, 20) == 20);
}

int Ceil(f64 value) {  ///
  auto result = (int)std::ceil(value);
  return result;
}

int Ceilf(f32 value) {  ///
  auto result = (int)std::ceilf(value);
  return result;
}

int Floor(f64 value) {  ///
  auto result = (int)std::floor(value);
  return result;
}

int Floorf(f32 value) {  ///
  auto result = (int)std::floorf(value);
  return result;
}

u64 CeilDivisionU64(u64 value, u64 divisor) {  ///
  ASSERT(value >= 0);
  ASSERT(divisor > 0);
  return value / divisor + ((value % divisor) != 0);
}

int CeilDivision(int value, int divisor) {  ///
  ASSERT(value >= 0);
  ASSERT(divisor > 0);
  return value / divisor + ((value % divisor) != 0);
}

TEST_CASE ("CeilDivision") {  ///
  ASSERT(CeilDivision(10, 1) == 10);
  ASSERT(CeilDivision(10, 5) == 2);
  ASSERT(CeilDivision(10, 6) == 2);
  ASSERT(CeilDivision(10, 4) == 3);
}

f32 GetLesserAngle(f32 aa, f32 bb) {  ///
  auto a = aa;
  auto b = bb;
  while (a < -PI32)
    a += 2 * PI32;
  while (b < -PI32)
    b += 2 * PI32;
  while (a >= PI32)
    a -= 2 * PI32;
  while (b >= PI32)
    b -= 2 * PI32;

  if (abs(a) > abs(b))
    return bb;
  return aa;
}

TEST_CASE ("GetLesserAngle") {  ///
  ASSERT(FloatEquals(GetLesserAngle(PI32 * 1 / 2, PI32), PI32 * 1 / 2));
  ASSERT(FloatEquals(GetLesserAngle(-PI32 * 1 / 2, PI32), -PI32 * 1 / 2));
  ASSERT(FloatEquals(GetLesserAngle(PI32 / 2, PI32 * 15 / 8), PI32 * 15 / 8));
}

f32 Clamp(f32 value, f32 min, f32 max) {  ///
  ASSERT_IS_NUMBER(min);
  ASSERT_IS_NUMBER(max);
  ASSERT(min <= max);
  if (value < min)
    return min;
  if (max < value)
    return max;
  return value;
}

f32 Clamp01(f32 value) {  ///
  return Clamp(value, 0, 1);
}

f32 Clamp11(f32 value) {  ///
  return Clamp(value, -1, 1);
}

int ClampInt(int value, int min, int max) {  ///
  ASSERT(min <= max);
  if (value < min)
    return min;
  if (value > max)
    return max;
  return value;
}

TEST_CASE ("ClampInt") {  ///
  ASSERT(ClampInt(2, 1, 3) == 2);
  ASSERT(ClampInt(0, 1, 3) == 1);
  ASSERT(ClampInt(4, 1, 3) == 3);
  ASSERT(ClampInt(1, 1, 3) == 1);
  ASSERT(ClampInt(3, 1, 3) == 3);
}

#define QUERY_BIT(bytesPtr, bitIndex) \
  ((*((u8*)(bytesPtr) + ((bitIndex) / 8))) & (1 << ((bitIndex) % 8)))

#define MARK_BIT(bytesPtr, bitIndex)                  \
  STATEMENT({                                         \
    u8& byte = *((u8*)(bytesPtr) + ((bitIndex) / 8)); \
    byte     = byte | (1 << ((bitIndex) % 8));        \
  })

#define UNMARK_BIT(bytesPtr, bitIndex)                \
  STATEMENT({                                         \
    u8& byte = *((u8*)(bytesPtr) + ((bitIndex) / 8)); \
    byte &= 0xFF - (1 << ((bitIndex) % 8));           \
  })

TEST_CASE ("Bit operations") {  ///
  {
    const struct {
      u8 a, b, c;
    } marks[] = {
      {0b00000000, 0, 0b00000001},
      {0b00000000, 1, 0b00000010},
      {0b00000000, 2, 0b00000100},
      {0b00000000, 7, 0b10000000},
    };
    for (auto& [initialValue, index, afterValue] : marks) {
      u8  byte  = initialValue;
      u8* bytes = &byte;
      MARK_BIT(bytes, index);
      CHECK(byte == afterValue);
      UNMARK_BIT(bytes, index);
      CHECK(byte == initialValue);
    }
  }

  {
    const struct {
      u8 a, b, c;
    } unmarks[] = {
      {0b11111111, 0, 0b11111110},
      {0b11111111, 1, 0b11111101},
      {0b11111111, 2, 0b11111011},
      {0b11111111, 7, 0b01111111},
    };
    for (auto& [initialValue, index, afterValue] : unmarks) {
      u8  byte  = initialValue;
      u8* bytes = &byte;
      UNMARK_BIT(bytes, index);
      CHECK(byte == afterValue);
      MARK_BIT(bytes, index);
      CHECK(byte == initialValue);
    }
  }

  {
    u8 ptr[] = {
      0b00000000,
      0b01000001,
    };
    CHECK((bool)QUERY_BIT(ptr, 0) == false);
    CHECK((bool)QUERY_BIT(ptr, 8) == true);
    CHECK((bool)QUERY_BIT(ptr, 14) == true);
  }
}

// Usage Examples:
//     32 -> 32
//     26 -> 32
//     13 -> 16
//     8 -> 8
//     0 -> ASSERT
//     2147483648 and above -> ASSERT
u32 CeilToPowerOf2(u32 value) {  ///
  ASSERT(value <= 2147483648);
  ASSERT(value != 0);

  u32 ceiledValue = 1;
  while (ceiledValue < value)
    ceiledValue *= 2;

  return ceiledValue;
}

TEST_CASE ("CeilToPowerOf2") {  ///
  CHECK(CeilToPowerOf2(1) == 1);
  CHECK(CeilToPowerOf2(2) == 2);
  CHECK(CeilToPowerOf2(3) == 4);
  CHECK(CeilToPowerOf2(4) == 4);
  CHECK(CeilToPowerOf2(31) == 32);
  CHECK(CeilToPowerOf2(32) == 32);
  CHECK(CeilToPowerOf2(65535) == 65536);
  CHECK(CeilToPowerOf2(65536) == 65536);
  CHECK(CeilToPowerOf2(2147483647) == 2147483648);
  CHECK(CeilToPowerOf2(2147483648) == 2147483648);
}

// Проверка на то, является ли число степенью двойки.
// При передаче указателя на power, там окажется значение этой степени.
bool IsPowerOf2(u32 number, int* power = nullptr) {  ///
  if (number == 0)
    return false;

  if (power)
    *power = 0;

  while (number > 1) {
    if (number & 1)
      return false;

    number >>= 1;
    if (power)
      *power += 1;
  }

  return true;
}

TEST_CASE ("IsPowerOf2") {  ///
  int power = 0;

  ASSERT(IsPowerOf2(2, &power));
  ASSERT(power == 1);
  ASSERT(IsPowerOf2(4, &power));
  ASSERT(power == 2);
  ASSERT(IsPowerOf2(1, &power));
  ASSERT(power == 0);

  ASSERT_FALSE(IsPowerOf2(-1, &power));
  ASSERT_FALSE(IsPowerOf2(0, &power));
  ASSERT_FALSE(IsPowerOf2(3, &power));
}

// NOTE: bounds are EXCLUSIVE.
#define POS_IS_IN_BOUNDS(pos, bounds) \
  (!((pos).x < 0 || (pos).x >= (bounds).x || (pos).y < 0 || (pos).y >= (bounds).y))

BF_FORCE_INLINE Vector2 Vector2Lerp(Vector2 v1, Vector2 v2, f32 amount) {  ///
  return v1 + (v2 - v1) * amount;
}

BF_FORCE_INLINE Vector2
Bezier(Vector2 v1, Vector2 v2, Vector2 v3, Vector2 v4, f32 t) {  ///
  ASSERT(t >= 0);
  ASSERT(t <= 1);

  auto a1 = Vector2Lerp(v1, v2, t);
  auto a2 = Vector2Lerp(v2, v3, t);
  auto a3 = Vector2Lerp(v3, v4, t);

  auto b1 = Vector2Lerp(a1, a2, t);
  auto b2 = Vector2Lerp(a2, a3, t);

  auto c = Vector2Lerp(b1, b2, t);
  return c;
}

using Easing_function_t = f32 (*)(f32);

f32 EaseLinear(f32 p) {  ///
  ASSERT_IS_NUMBER(p);
  return p;
}

f32 EaseInQuad(f32 p) {  ///
  ASSERT_IS_NUMBER(p);
  return p * p;
}

f32 EaseOutQuad(f32 p) {  ///
  ASSERT_IS_NUMBER(p);
  return -(p * (p - 2));
}

f32 EaseInOutQuad(f32 p) {  ///
  ASSERT_IS_NUMBER(p);
  if (p < 0.5)
    return 2 * p * p;
  else
    return (-2 * p * p) + (4 * p) - 1;
}

f32 EaseInCubic(f32 p) {  ///
  ASSERT_IS_NUMBER(p);
  return p * p * p;
}

f32 EaseOutCubic(f32 p) {  ///
  ASSERT_IS_NUMBER(p);
  f32 f = (p - 1);
  return f * f * f + 1;
}

f32 EaseInOutCubic(f32 p) {  ///
  ASSERT_IS_NUMBER(p);
  if (p < 0.5f)
    return 4 * p * p * p;
  else {
    f32 f = ((2 * p) - 2);
    return 0.5f * f * f * f + 1;
  }
}

f32 EaseInQuart(f32 p) {  ///
  ASSERT_IS_NUMBER(p);
  return p * p * p * p;
}

f32 EaseOutQuart(f32 p) {  ///
  ASSERT_IS_NUMBER(p);
  f32 f = (p - 1);
  return f * f * f * (1 - p) + 1;
}

f32 EaseInOutQuart(f32 p) {  ///
  ASSERT_IS_NUMBER(p);
  if (p < 0.5)
    return 8 * p * p * p * p;
  else {
    f32 f = (p - 1);
    return -8 * f * f * f * f + 1;
  }
}

f32 EaseInQuint(f32 p) {  ///
  ASSERT_IS_NUMBER(p);
  return p * p * p * p * p;
}

f32 EaseOutQuint(f32 p) {  ///
  ASSERT_IS_NUMBER(p);
  f32 f = (p - 1);
  return f * f * f * f * f + 1;
}

f32 EaseInOutQuint(f32 p) {  ///
  ASSERT_IS_NUMBER(p);
  if (p < 0.5f)
    return 16 * p * p * p * p * p;
  else {
    f32 f = ((2 * p) - 2);
    return 0.5f * f * f * f * f * f + 1;
  }
}

f32 EaseInSin(f32 p) {  ///
  ASSERT_IS_NUMBER(p);
  return sinf((p - 1) * 2 * PI32) + 1;
}

f32 EaseOutSin(f32 p) {  ///
  ASSERT_IS_NUMBER(p);
  return sinf(p * 2 * PI32);
}

f32 EaseInOutSin(f32 p) {  ///
  ASSERT_IS_NUMBER(p);
  return 0.5f * (1 - cosf(p * PI32));
}

f32 EaseInCirc(f32 p) {  ///
  ASSERT_IS_NUMBER(p);
  return 1 - sqrtf(1 - (p * p));
}

f32 EaseOutCirc(f32 p) {  ///
  ASSERT_IS_NUMBER(p);
  return sqrtf((2 - p) * p);
}

f32 EaseInOutCirc(f32 p) {  ///
  ASSERT_IS_NUMBER(p);
  if (p < 0.5f)
    return 0.5f * (1 - sqrtf(1 - 4 * (p * p)));
  else
    return 0.5f * (sqrtf(-((2 * p) - 3) * ((2 * p) - 1)) + 1);
}

f32 EaseInExpo(f32 p) {  ///
  ASSERT_IS_NUMBER(p);
  return (p == 0.0f) ? p : powf(2, 10 * (p - 1));
}

f32 EaseOutExpo(f32 p) {  ///
  ASSERT_IS_NUMBER(p);
  return (p == 1.0f) ? p : 1 - powf(2, -10 * p);
}

f32 EaseInOutExpo(f32 p) {  ///
  ASSERT_IS_NUMBER(p);
  if (p == 0.0f || p == 1.0f)
    return p;
  if (p < 0.5f)
    return 0.5f * powf(2, (20 * p) - 10);
  else
    return -0.5f * powf(2, (-20 * p) + 10) + 1;
}

f32 EaseInElastic(f32 p) {  ///
  ASSERT_IS_NUMBER(p);
  return sinf(13 * 2 * PI32 * p) * powf(2, 10 * (p - 1));
}

f32 EaseOutElastic(f32 p) {  ///
  ASSERT_IS_NUMBER(p);
  return sinf(-13 * 2 * PI32 * (p + 1)) * powf(2, -10 * p) + 1;
}

f32 EaseInOutElastic(f32 p) {  ///
  ASSERT_IS_NUMBER(p);
  if (p < 0.5f)
    return 0.5f * sinf(13 * 2 * PI32 * (2 * p)) * powf(2, 10 * ((2 * p) - 1));
  else
    return 0.5f
           * (sinf(-13 * 2 * PI32 * ((2 * p - 1) + 1)) * powf(2, -10 * (2 * p - 1)) + 2);
}

f32 EaseInBack(f32 p) {  ///
  ASSERT_IS_NUMBER(p);
  return p * p * p - p * sinf(p * PI32);
}

f32 EaseOutBack(f32 p) {  ///
  ASSERT_IS_NUMBER(p);
  f32 f = (1 - p);
  return 1 - (f * f * f - f * sinf(f * PI32));
}

f32 EaseInOutBack(f32 p) {  ///
  ASSERT_IS_NUMBER(p);
  if (p < 0.5f) {
    f32 f = 2 * p;
    return 0.5f * (f * f * f - f * sinf(f * PI32));
  }
  else {
    f32 f = (1 - (2 * p - 1));
    return 0.5f * (1 - (f * f * f - f * sinf(f * PI32))) + 0.5f;
  }
}

f32 EaseOutBounce(f32 p) {  ///
  ASSERT_IS_NUMBER(p);
  if (p < 4 / 11.0f)
    return (121 * p * p) / 16.0f;
  else if (p < 8 / 11.0f)
    return (363 / 40.0f * p * p) - (99 / 10.0f * p) + 17 / 5.0f;
  else if (p < 9 / 10.0)
    return (4356 / 361.0f * p * p) - (35442 / 1805.0f * p) + 16061 / 1805.0f;
  else
    return (54 / 5.0f * p * p) - (513 / 25.0f * p) + 268 / 25.0f;
}

f32 EaseInBounce(f32 p) {  ///
  ASSERT_IS_NUMBER(p);
  return 1 - EaseOutBounce(1 - p);
}

f32 EaseInOutBounce(f32 p) {  ///
  ASSERT_IS_NUMBER(p);
  if (p < 0.5f)
    return 0.5f * EaseInBounce(p * 2);
  else
    return 0.5f * EaseOutBounce(p * 2 - 1) + 0.5f;
}

f32 EaseABitUpThenDown(f32 p) {  ///
  ASSERT_IS_NUMBER(p);
  /*
  [[[cog
  import numpy as np
  k = np.polyfit([0, 0.3, 1], [0, 1, -2], 2)
  cog.outl("return {:.6f}f * p * p + {:.6f}f * p + {:.6f}f;".format(*k))
  ]]] */
  return -7.619048f * p * p + 5.619048f * p + -0.000000f;
  /* [[[end]]] */
}

f32 EaseBounceThenZero(f32 p) {  ///
  ASSERT_IS_NUMBER(p);
  /*
  [[[cog
  import numpy as np
  k = np.polyfit([0, 0.5, 1], [0, 1, 0], 2)
  cog.outl("return {:.6f}f * p * p + {:.6f}f * p + {:.6f}f;".format(*k))
  ]]] */
  return -4.000000f * p * p + 4.000000f * p + 0.000000f;
  /* [[[end]]] */
}

f32 EaseBounceSmall(f32 p) {  ///
  ASSERT_IS_NUMBER(p);
  /*
  [[[cog
  import numpy as np
  k = np.polyfit([0, 0.5, 1], [0, 1.025, 1], 2)
  cog.outl("return {:.6f}f * p * p + {:.6f}f * p + {:.6f}f;".format(*k))
  ]]] */
  return -2.100000f * p * p + 3.100000f * p + 0.000000f;
  /* [[[end]]] */
}

BF_FORCE_INLINE f32 EaseBounceSmallSmooth(f32 p) {  ///
  ASSERT_IS_NUMBER(p);
  return Bezier({0, 0}, {0.8f, 1.8f}, {0.8f, 0.9f}, {1, 1}, p).y;
}

#define OFFSET_IN_DIRECTION_OF_ANGLE_RAD(offset, angleRad) \
  (Vector2Rotate(Vector2((offset), 0), (angleRad)))
#define OFFSET_IN_DIRECTION_OF_ANGLE_DEG(offset, angleDeg) \
  OFFSET_IN_DIRECTION_OF_ANGLE_RAD((offset), (angleDeg) * DEG2RAD)

f32 Lerp(f32 start, f32 end, f32 amount) {  ///
  ASSERT_IS_NUMBER(start);
  ASSERT_IS_NUMBER(end);
  ASSERT_IS_NUMBER(amount);

  return start + amount * (end - start);
}

f32 Remap(f32 v, f32 range1min, f32 range1max, f32 range2min, f32 range2max) {  ///
  ASSERT_IS_NUMBER(v);
  ASSERT_IS_NUMBER(range1min);
  ASSERT_IS_NUMBER(range1max);
  ASSERT_IS_NUMBER(range2min);
  ASSERT_IS_NUMBER(range2max);

  const auto t = Unlerp(v, range1min, range1max);
  return Lerp(range2min, range2max, t);
}

TEST_CASE ("Remap") {  ///
  ASSERT(FloatEquals(Remap(18.0f, 10.0f, 20.0f, 5.0f, 10.0f), 9.0f));
}

Vector2 Vector2ExponentialDecay(Vector2 a, Vector2 b, f32 decay, f32 dt) {  ///
  ASSERT_IS_NUMBER(decay);
  ASSERT_IS_NUMBER(dt);
  return b + (a - b) * expf(-decay * dt);
}

Vector3 Vector3ExponentialDecay(Vector3 a, Vector3 b, f32 decay, f32 dt) {  ///
  ASSERT_IS_NUMBER(decay);
  ASSERT_IS_NUMBER(dt);
  return b + (a - b) * expf(-decay * dt);
}

f32 InOutLerp(f32 from, f32 to, f32 elapsed, f32 totalDuration, f32 easeDuration) {  ///
  ASSERT_IS_NUMBER(from);
  ASSERT_IS_NUMBER(to);
  ASSERT_IS_NUMBER(elapsed);
  ASSERT_IS_NUMBER(totalDuration);
  ASSERT_IS_NUMBER(easeDuration);
  ASSERT(easeDuration * 2 <= totalDuration);

  if (elapsed < easeDuration) {
    auto p = elapsed / easeDuration;
    return Lerp(from, to, p);
  }
  else if (elapsed > totalDuration - easeDuration) {
    auto p = 1 - (elapsed - (totalDuration - easeDuration)) / easeDuration;
    return Lerp(from, to, p);
  }
  ASSERT(elapsed <= totalDuration);
  return to;
}

constexpr u32 EMPTY_HASH32 = 0x811c9dc5;

u32 Hash32(const u8* key, const int len) {  ///
  ASSERT(len >= 0);

  // Wiki: https://en.wikipedia.org/wiki/Fowler%E2%80%93Noll%E2%80%93Vo_hash_function
  auto hash = EMPTY_HASH32;
  FOR_RANGE (int, i, len) {
    // hash = (hash * 16777619) ^ (*key);  // FNV-1
    hash = (hash ^ (*key)) * 0x01000193;  // FNV-1a
    key++;
  }

  return hash;
}

u32 Hash32_ZeroTerminatedString(const char* key) {  ///
  auto hash = EMPTY_HASH32;
  while (*key) {
    // hash = (hash * 16777619) ^ (*key);  // FNV-1
    hash = (hash ^ (*key)) * 0x01000193;  // FNV-1a
    key++;
  }

  return hash;
}

void IncrementSetZeroOn(int* value, int mod) {  ///
  *value = *value + 1;

  if (*value >= mod)
    *value = 0;
}

// NOTE: `start` and `end` values must be accessible via `step`!
int ArithmeticSum(int start, int end, int step = 1) {  ///
  ASSERT_FALSE((end - start) % step);
  if (end >= start) {
    ASSERT(step > 0);
    return (start + end) * (end - start + 1) / 2;
  }
  else if (end < start) {
    ASSERT(step < 0);
    return (start + end) * (start - end + 1) / 2;
  }
  INVALID_PATH;
  return 0;
}

TEST_CASE ("ArithmeticSum") {  ///
  ASSERT(ArithmeticSum(1, 1) == 1);
  ASSERT(ArithmeticSum(2, 2) == 2);
  ASSERT(ArithmeticSum(1, 4) == 10);
  ASSERT(ArithmeticSum(4, 1, -1) == 10);
  ASSERT(ArithmeticSum(1, 5) == 15);
  ASSERT(ArithmeticSum(2, 5) == 14);
}

// NOTE: `start` and `end` values must be accessible via `step`!
f32 ArithmeticSumAverage(int start, int end, int step = 1) {  ///
  int sum = ArithmeticSum(start, end, step);
  if (start > end) {
    auto t = start;
    start  = end;
    end    = t;
  }
  return (f32)sum / (f32)(end - start + 1);
}

TEST_CASE ("AverageOfArithmeticSum") {  ///
  ASSERT(FloatEquals(ArithmeticSumAverage(1, 4), 2.5f));
  ASSERT(FloatEquals(ArithmeticSumAverage(4, 1, -1), 2.5f));
  ASSERT(FloatEquals(ArithmeticSumAverage(1, 5), 3.0f));
  ASSERT(FloatEquals(ArithmeticSumAverage(2, 5), 3.5f));
}

///
