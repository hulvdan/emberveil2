// cppcheck-suppress-file unusedFunction

#pragma once

struct Arena {
  size_t used = 0;
  size_t size = 0;
  u8*    base = nullptr;
};

Arena MakeArena(size_t size) {
  return {.size = size, .base = (u8*)malloc(size)};
}

void DeinitArena(Arena* arena) {
  free(arena->base);
  *arena = {};
}

#define ALLOCATE_FOR(arena, type) RCAST<type*>(Allocate_(arena, sizeof(type)))
#define ALLOCATE_ARRAY(arena, type, count) \
  RCAST<type*>(Allocate_(arena, sizeof(type) * (count)))

#define ALLOCATE_ZEROS_FOR(arena, type) RCAST<type*>(AllocateZeros_(arena, sizeof(type)))
#define ALLOCATE_ZEROS_ARRAY(arena, type, count) \
  RCAST<type*>(AllocateZeros_(arena, sizeof(type) * (count)))

// NOLINTBEGIN(bugprone-macro-parentheses)
#define ALLOCATE_FOR_AND_INITIALIZE(arena, type)             \
  (INLINE_LAMBDA {                                           \
    auto ptr = RCAST<type*>(Allocate_(arena, sizeof(type))); \
    std::construct_at(ptr);                                  \
    return ptr;                                              \
  }())
// NOLINTEND(bugprone-macro-parentheses)

// NOLINTBEGIN(bugprone-macro-parentheses)
#define ALLOCATE_ARRAY_AND_INITIALIZE(arena, type, count)              \
  (INLINE_LAMBDA {                                                     \
    auto ptr = RCAST<type*>(Allocate_(arena, sizeof(type) * (count))); \
    FOR_RANGE (int, i, (count)) {                                      \
      std::construct_at(ptr + i);                                      \
    }                                                                  \
    return ptr;                                                        \
  }())
// NOLINTEND(bugprone-macro-parentheses)

#define DEALLOCATE_ARRAY(arena, type, count) Deallocate_(arena, sizeof(type) * (count))

//
// TODO: Introduce the notion of `alignment` here!
// NOTE: Refer to Casey's memory allocation functions
// https://youtu.be/MvDUe2evkHg?list=PLEMXAbCVnmY6Azbmzj3BiC3QRYHE9QoG7&t=2121
//
inline u8* Allocate_(Arena* arena, size_t size) {
  ASSERT(size > 0);
  ASSERT(arena->size >= size);
  ASSERT(arena->used <= arena->size - size);

  u8* result = arena->base + arena->used;
  arena->used += size;
  return result;
}

inline u8* AllocateZeros_(Arena* arena, size_t size) {
  auto result = Allocate_(arena, size);
  memset(result, 0, size);
  return result;
}

inline void Deallocate_(Arena* arena, size_t size) {
  ASSERT(size > 0);
  ASSERT(arena->used >= size);
  arena->used -= size;
}

// TEMP_USAGE используется для временного использования арены.
// При вызове TEMP_USAGE запоминается текущее количество занятого
// пространства арены, которое обратно устанавливается при выходе из scope.
//
// Пример использования:
//
//     size_t X = trash_arena->used;
//     {
//         TEMP_USAGE(trash_arena);
//         int* aboba = ALLOCATE_FOR(trash_arena, u32);
//         ASSERT(trash_arena->used == X + 4);
//     }
//     ASSERT(trash_arena->used == X);
//

#define TEMP_USAGE_WITH_COUNTER_(arena, counter)     \
  auto _arena##counter##Used_ = (arena)->used;       \
  DEFER {                                            \
    ASSERT((arena)->used >= _arena##counter##Used_); \
    (arena)->used = _arena##counter##Used_;          \
  };
#define TEMP_USAGE_(arena, counter) TEMP_USAGE_WITH_COUNTER_(arena, counter)
#define TEMP_USAGE(arena) TEMP_USAGE_(arena, __COUNTER__)
