// Do this:
//    #define BASE91_IMPLEMENTATION
// before you include this file in *one* C or C++ file to create the implementation.
//
// You can #define BASE91_ASSERT(x) before the #include to avoid using assert.h.

#ifndef BASE91_INCLUDE_H
#define BASE91_INCLUDE_H

#include <stddef.h>

// !banner: INTERFACE
// ██╗███╗   ██╗████████╗███████╗██████╗ ███████╗ █████╗  ██████╗███████╗
// ██║████╗  ██║╚══██╔══╝██╔════╝██╔══██╗██╔════╝██╔══██╗██╔════╝██╔════╝
// ██║██╔██╗ ██║   ██║   █████╗  ██████╔╝█████╗  ███████║██║     █████╗
// ██║██║╚██╗██║   ██║   ██╔══╝  ██╔══██╗██╔══╝  ██╔══██║██║     ██╔══╝
// ██║██║ ╚████║   ██║   ███████╗██║  ██║██║     ██║  ██║╚██████╗███████╗
// ╚═╝╚═╝  ╚═══╝   ╚═╝   ╚══════╝╚═╝  ╚═╝╚═╝     ╚═╝  ╚═╝ ╚═════╝╚══════╝

#ifdef __cplusplus
extern "C" {
#endif

// n binary bytes will be encoded as `ceil((n)*8/6.5)` bytes
// Returned size (similarly to `strlen`) DOESN'T include 0-byte.
// String includes 0-byte at the end.
size_t base91_encode(const void *inBytesToEncode, size_t inBytesToEncodeSize, char *outEncodedString);

size_t base91_decode(const char *inEncodedString, size_t inEncodedStringSize, void *outDecodedBytes);

#ifdef __cplusplus
}
#endif

#endif	// BASE91_INCLUDE_H

// !banner: IMPL
// ██╗███╗   ███╗██████╗ ██╗
// ██║████╗ ████║██╔══██╗██║
// ██║██╔████╔██║██████╔╝██║
// ██║██║╚██╔╝██║██╔═══╝ ██║
// ██║██║ ╚═╝ ██║██║     ███████╗
// ╚═╝╚═╝     ╚═╝╚═╝     ╚══════╝

#ifdef BASE91_IMPLEMENTATION

#ifndef BASE91_ASSERT
#include <assert.h>
#define BASE91_ASSERT(x) assert(x)
#endif

#ifdef __cplusplus
extern "C" {
#endif

size_t base91_encode(const void *i, size_t len, char *o) {  ///
  const uint8_t *ib = (const uint8_t*)i;
  uint8_t *ob = (uint8_t*)o;
  size_t n = 0;
  uint8_t nbits = 0;
  uint32_t bqueue = 0;

  while (len--) {
    nbits += 8;
    bqueue |= (uint32_t) * ib++ << (32 - nbits);
    if (nbits > 12) {  /* enough bits in queue */
      const uint16_t val = bqueue >> (32 - 13);
      bqueue <<= 13;
      nbits -= 13;
      ob[n++] = val / 91 + 33; // APRS alphabet
      ob[n++] = val % 91 + 33;
    }
  }

  // finish the queue
  if (nbits > 6) { // put remaining in to 2 more bytes
    const uint16_t val = bqueue >> (32 - 13);
    ob[n++] = val / 91 + 33;
    ob[n++] = val % 91 + 33;
  }
  else if (nbits > 0) { //need 1 more byte
    const uint16_t val = bqueue >> (32 - 6);
    ob[n++] = val % 91 + 33;
  }

  ob[n] = 0;
  return n;
}

size_t base91_decode(const char *i, size_t len, void *o) {  ///
  const uint8_t *ib = (const uint8_t *)i;
  uint8_t *ob = (uint8_t*)o;
  size_t n = 0;
  uint16_t d;
  //int32_t val;
  int32_t val = -1;
  uint8_t nbits = 0;
  uint32_t bqueue = 0;

  while (len--) {
    // d = dectab[*ib++];
    d = *ib++ - 33; // APRS alphabet
    if (val == -1)
      val = d; /* start next value */
    else {
      val = val * 91 + d;
      nbits += 13;
      bqueue |= (uint32_t) val << (32 - nbits);
      do {
        ob[n++] = bqueue >> (32 - 8);
        bqueue <<= 8;
        nbits -= 8;
      } while (nbits > 7);
      val = -1;  /* mark value complete */
    }
  }

  if (val != -1) {
    nbits += 6;
    bqueue |= val << (32 - nbits);
    ob[n++] = bqueue >> (32 - 8);
    val = -1;  /* mark value complete */
  }

  return n;
}

#ifdef __cplusplus
}
#endif

#endif // BASE91_IMPLEMENTATION

///
