#ifndef SDL_endian_h_
#define SDL_endian_h_

#define SDL_BYTEORDER __BYTE_ORDER__
#define SDL_LIL_ENDIAN __ORDER_LITTLE_ENDIAN__
#define SDL_BIG_ENDIAN __ORDER_BIG_ENDIAN__

#if SDL_BYTEORDER == SDL_LIL_ENDIAN
#define SDL_SwapLE16(x) (x)
#define SDL_SwapLE32(x) (x)
#else
#error
#endif

#endif
