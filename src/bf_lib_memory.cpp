// cppcheck-suppress-file unusedFunction

#pragma once

// Unmapping allocator.
// ============================================================

#if BF_DEBUG && defined(SDL_PLATFORM_WINDOWS)

#  include <windows.h>
#  include <stdio.h>

struct UnmappedAllocation {
  void* result = {};
  void* base   = {};
};

static struct {
  std::vector<UnmappedAllocation> allocs   = {};
  size_t                          pageSize = {};
} g_unmappingAllocatorData;

void* unmapped_alloc(size_t size) {  ///
  auto& data = g_unmappingAllocatorData;

  static bool initialized = false;
  if (!initialized) {
    initialized = true;
    SYSTEM_INFO si{};
    GetSystemInfo(&si);
    data = {.pageSize = si.dwPageSize};
  }

  auto pages
    = CeilDivisionU64(size, data.pageSize) + 2 * UNMAPPING_ALLOCATOR_PAGES_MARGIN;
  auto base   = VirtualAlloc(nullptr, pages * data.pageSize, MEM_RESERVE, PAGE_NOACCESS);
  auto addr   = (void*)((u8*)base + UNMAPPING_ALLOCATOR_PAGES_MARGIN * data.pageSize);
  auto result = VirtualAlloc(addr, size, MEM_COMMIT, PAGE_READWRITE);

#  if UNMAPPING_ALLOCATOR_ERROR_ON_RIGHT
  auto baseOffset = (pages - UNMAPPING_ALLOCATOR_PAGES_MARGIN) * data.pageSize - size;
#  else
  auto baseOffset = data.pageSize * UNMAPPING_ALLOCATOR_PAGES_MARGIN;
#  endif

  auto r = (void*)((u8*)base + baseOffset);
  data.allocs.push_back({.result = r, .base = base});
  return r;
}

void unmapped_free(void* ptr) {  ///
  auto& data = g_unmappingAllocatorData;

  FOR_RANGE (int, i, data.allocs.size()) {
    auto& v = data.allocs[i];
    if (v.result == ptr) {
      VirtualFree((void*)v.base, 0, MEM_RELEASE);

      if (i != data.allocs.size() - 1)
        v = data.allocs[data.allocs.size() - 1];
      data.allocs.pop_back();

      return;
    }
  }
  INVALID_PATH;
}

TEST_CASE ("Unmapping allocator") {
  auto base = (u8*)unmapped_alloc(sizeof(u8));
  base[0]   = 255;
  unmapped_free(base);
}

#else

BF_FORCE_INLINE void* unmapped_alloc(size_t size) {  ///
  return malloc(size);
}

BF_FORCE_INLINE void unmapped_free(void* ptr) {  ///
  free(ptr);
}

#endif

// Arena.
// ============================================================

struct Arena {
  size_t used    = 0;
  size_t size    = 0;
  u8*    base    = nullptr;
  size_t maxUsed = 0;
};

Arena MakeArena(size_t size) {  ///
  return {.size = size, .base = (u8*)BF_ALLOC(size)};
}

void DeinitArena(Arena* arena) {  ///
  BF_FREE(arena->base);
  *arena = {};
}

#define ALLOCATE_FOR(arena, type) (type*)(Allocate_(arena, sizeof(type)))
#define ALLOCATE_ARRAY(arena, type, count) \
  (type*)(Allocate_(arena, sizeof(type) * (count)))

#define ALLOCATE_ZEROS_FOR(arena, type) (type*)(AllocateZeros_(arena, sizeof(type)))
#define ALLOCATE_ZEROS_ARRAY(arena, type, count) \
  (type*)(AllocateZeros_(arena, sizeof(type) * (count)))

// NOLINTBEGIN(bugprone-macro-parentheses)
#define ALLOCATE_FOR_AND_INITIALIZE(arena, type)        \
  (INLINE_LAMBDA {                                      \
    auto ptr = (type*)(Allocate_(arena, sizeof(type))); \
    std::construct_at(ptr);                             \
    return ptr;                                         \
  }())
// NOLINTEND(bugprone-macro-parentheses)

// NOLINTBEGIN(bugprone-macro-parentheses)
#define ALLOCATE_ARRAY_AND_INITIALIZE(arena, type, count)         \
  (INLINE_LAMBDA {                                                \
    auto ptr = (type*)(Allocate_(arena, sizeof(type) * (count))); \
    FOR_RANGE (int, i, (count)) {                                 \
      std::construct_at(ptr + i);                                 \
    }                                                             \
    return ptr;                                                   \
  }())
// NOLINTEND(bugprone-macro-parentheses)

#define DEALLOCATE_ARRAY(arena, type, count) Deallocate_(arena, sizeof(type) * (count))

//
// TODO: Introduce the notion of `alignment` here!
// NOTE: Refer to Casey's memory allocation functions
// https://youtu.be/MvDUe2evkHg?list=PLEMXAbCVnmY6Azbmzj3BiC3QRYHE9QoG7&t=2121
//
inline u8* Allocate_(Arena* arena, size_t size) {  ///
  ASSERT(size > 0);
  ASSERT(arena->size >= size);
  ASSERT(arena->used <= arena->size - size);

  u8* result = arena->base + arena->used;
  arena->used += size;
  arena->maxUsed = MAX(arena->used, arena->maxUsed);
  return result;
}

inline u8* AllocateZeros_(Arena* arena, size_t size) {  ///
  auto result = Allocate_(arena, size);
  memset(result, 0, size);
  return result;
}

inline void Deallocate_(Arena* arena, size_t size) {  ///
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

///
