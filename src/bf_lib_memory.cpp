// cppcheck-suppress-file unusedFunction

#pragma once

// Arena.
// ============================================================

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

// Unmapping allocator.
// ============================================================

#if BF_DEBUG && defined(SDL_PLATFORM_WINDOWS)
#  define UNMAPPED_PAGES_MARGIN 4
#  define UNMAPPING_ALLOCATOR_ERROR_ON_RIGHT 1

#  include <windows.h>
#  include <stdio.h>

struct UnmappedAllocation {
  void* result = {};
  void* base   = {};
};

Vector<UnmappedAllocation> _g_unmappedAllocations = {};
size_t                     _g_pageSize            = {};

void* unmapped_alloc(size_t size) {
  static bool initialized = false;
  if (!initialized) {
    initialized = true;
    SYSTEM_INFO si{};
    GetSystemInfo(&si);
    _g_pageSize = si.dwPageSize;
  }

  auto pages  = CeilDivisionU64(size, _g_pageSize) + 2 * UNMAPPED_PAGES_MARGIN;
  auto base   = VirtualAlloc(0, pages * _g_pageSize, MEM_RESERVE, PAGE_NOACCESS);
  auto addr   = (void*)((u8*)base + UNMAPPED_PAGES_MARGIN * _g_pageSize);
  auto result = VirtualAlloc(addr, size, MEM_COMMIT, PAGE_READWRITE);

  auto baseOffset = _g_pageSize * UNMAPPED_PAGES_MARGIN;
#  if UNMAPPING_ALLOCATOR_ERROR_ON_RIGHT
  baseOffset += pages * _g_pageSize - size;
#  endif

  *_g_unmappedAllocations.Add() = {
    .result = (void*)((u8*)base + baseOffset),
    .base   = base,
  };
  return result;
}

void unmapped_free(void* ptr) {
  FOR_RANGE (int, i, _g_unmappedAllocations.count) {
    auto& v = _g_unmappedAllocations[i];
    if (v.result == ptr) {
      VirtualFree((void*)v.base, 0, MEM_RELEASE);
      _g_unmappedAllocations.UnstableRemoveAt(i);
      return;
    }
  }
  INVALID_PATH;
}

#else

BF_FORCE_INLINE void* unmapped_alloc(size_t size) {
  return malloc(size);
}

BF_FORCE_INLINE void unmapped_free(void* ptr) {
  return free(ptr);
}

#endif
