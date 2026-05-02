// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Fri May  1 07:45:03 PM MDT 2026
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_ENDIAN_H_
#define YAE_ENDIAN_H_

// aeyae:
#include "yae/api/yae_api.h"
#include "yae/api/yae_assert.h"

// standard:
#include <stdint.h>
#include <string.h>


namespace yae
{

  //----------------------------------------------------------------
  // swap
  //
  inline void bswap(uint8_t & a, uint8_t & b)
  {
    a = a ^ b;
    b = b ^ a;
    a = a ^ b;
  }

  //----------------------------------------------------------------
  // bswap_16
  //
  inline uint16_t bswap_16(uint16_t x)
  {
    return ((x << 8) | (x >> 8));
  }

  //----------------------------------------------------------------
  // bswap_32
  //
  inline uint32_t bswap_32(uint32_t x)
  {
    return ((x << 24) |
            ((x & 0x0000FF00) << 8) |
            ((x & 0x00FF0000) >> 8) |
            (x >> 24));
  }

  //----------------------------------------------------------------
  // bswap_64
  //
  inline uint64_t bswap_64(uint64_t x)
  {
    return (uint64_t(yae::bswap_32(uint32_t(x))) << 32 |
            uint64_t(yae::bswap_32(uint32_t(x >> 32))));
  }

  //----------------------------------------------------------------
  // bswap_16
  //
  YAE_API void bswap_16(void * dst,
                        void * dst_end,
                        const void * src,
                        const void * src_end);

  //----------------------------------------------------------------
  // bswap_32
  //
  YAE_API void bswap_32(void * dst,
                        void * dst_end,
                        const void * src,
                        const void * src_end);

  //----------------------------------------------------------------
  // bswap_64
  //
  YAE_API void bswap_64(void * dst,
                        void * dst_end,
                        const void * src,
                        const void * src_end);

  //----------------------------------------------------------------
  // bswap_16_inplace
  //
  YAE_API void bswap_16_inplace(void * u16, void * u16_end);

  //----------------------------------------------------------------
  // bswap_32_inplace
  //
  YAE_API void bswap_32_inplace(void * u32, void * u32_end);

  //----------------------------------------------------------------
  // bswap_64_inplace
  //
  YAE_API void bswap_64_inplace(void * u64, void * u64_end);

  //----------------------------------------------------------------
  // copy
  //
  inline void copy(void * dst,
                   void * dst_end,
                   const void * src,
                   const void * src_end,
                   std::size_t alignment)
  {
    std::size_t dst_len = (uint8_t *)dst_end - (uint8_t *)dst;
    std::size_t src_len = (const uint8_t *)src_end - (const uint8_t *)src;
    std::size_t len = (dst_len < src_len) ? dst_len : src_len;
    YAE_ASSERT(len % alignment == 0);
    memcpy(dst, src, len);
  }
}

//----------------------------------------------------------------
// YAE_Nxx_TO_BE
// YAE_Nxx_TO_LE
//
// helper macros for converting from Native Endian to Big/Little Endian
//
#if defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
#define YAE_N16_TO_BE(x) x
#define YAE_N32_TO_BE(x) x
#define YAE_N64_TO_BE(x) x
#define YAE_N16_TO_LE(x) yae::bswap_16(x)
#define YAE_N32_TO_LE(x) yae::bswap_32(x)
#define YAE_N64_TO_LE(x) yae::bswap_64(x)

namespace yae
{
  inline void ntob_16(void * dst,
                      void * dst_end,
                      const void * src,
                      const void * src_end)
  { yae::copy(dst, dst_end, src, src_end, 2); }

  inline void ntob_32(void * dst,
                      void * dst_end,
                      const void * src,
                      const void * src_end)
  { yae::copy(dst, dst_end, src, src_end, 4); }

  inline void ntob_64(void * dst,
                      void * dst_end,
                      const void * src,
                      const void * src_end)
  { yae::copy(dst, dst_end, src, src_end, 8); }

  inline void ntob_16_inplace(void *, void *)
  { return; }

  inline void ntob_32_inplace(void *, void *)
  { return; }

  inline void ntob_64_inplace(void *, void *)
  { return; }

  inline void ntol_16(void * dst,
                      void * dst_end,
                      const void * src,
                      const void * src_end)
  { yae::bswap_16(dst, dst_end, src, src_end); }

  inline void ntol_32(void * dst,
                      void * dst_end,
                      const void * src,
                      const void * src_end)
  { yae::bswap_32(dst, dst_end, src, src_end); }

  inline void ntol_64(void * dst,
                      void * dst_end,
                      const void * src,
                      const void * src_end)
  { yae::bswap_64(dst, dst_end, src, src_end); }

  inline void ntol_16_inplace(void * dst, void * dst_end)
  { yae::bswap_16_inplace(dst, dst_end); }

  inline void ntol_32_inplace(void * dst, void * dst_end)
  { yae::bswap_32_inplace(dst, dst_end); }

  inline void ntol_64_inplace(void * dst, void * dst_end)
  { yae::bswap_64_inplace(dst, dst_end); }
}

#else

#define YAE_N16_TO_BE(x) yae::bswap_16(x)
#define YAE_N32_TO_BE(x) yae::bswap_32(x)
#define YAE_N64_TO_BE(x) yae::bswap_64(x)
#define YAE_N16_TO_LE(x) x
#define YAE_N32_TO_LE(x) x
#define YAE_N64_TO_LE(x) x

namespace yae
{
  inline void ntob_16(void * dst,
                      void * dst_end,
                      const void * src,
                      const void * src_end)
  { yae::bswap_16(dst, dst_end, src, src_end); }

  inline void ntob_32(void * dst,
                      void * dst_end,
                      const void * src,
                      const void * src_end)
  { yae::bswap_32(dst, dst_end, src, src_end); }

  inline void ntob_64(void * dst,
                      void * dst_end,
                      const void * src,
                      const void * src_end)
  { yae::bswap_64(dst, dst_end, src, src_end); }

  inline void ntob_16_inplace(void * dst, void * dst_end)
  { yae::bswap_16_inplace(dst, dst_end); }

  inline void ntob_32_inplace(void * dst, void * dst_end)
  { yae::bswap_32_inplace(dst, dst_end); }

  inline void ntob_64_inplace(void * dst, void * dst_end)
  { yae::bswap_64_inplace(dst, dst_end); }

  inline void ntol_16(void * dst,
                      void * dst_end,
                      const void * src,
                      const void * src_end)
  { yae::copy(dst, dst_end, src, src_end, 2); }

  inline void ntol_32(void * dst,
                      void * dst_end,
                      const void * src,
                      const void * src_end)
  { yae::copy(dst, dst_end, src, src_end, 4); }

  inline void ntol_64(void * dst,
                      void * dst_end,
                      const void * src,
                      const void * src_end)
  { yae::copy(dst, dst_end, src, src_end, 8); }

  inline void ntol_16_inplace(void *, void *)
  { return; }

  inline void ntol_32_inplace(void *, void *)
  { return; }

  inline void ntol_64_inplace(void *, void *)
  { return; }
}

#endif

namespace yae
{

  //----------------------------------------------------------------
  // ntob_16
  //
  template <typename TData>
  inline static TData
  ntob_16(TData data)
  {
    YAE_ASSERT(sizeof(TData) == 2);

    union U
    {
      TData data;
      uint16_t i;
    } dst;

    dst.data = data;
    dst.i = YAE_N16_TO_BE(dst.i);
    return dst.data;
  }

  //----------------------------------------------------------------
  // ntob_32
  //
  template <typename TData>
  inline static TData
  ntob_32(TData data)
  {
    YAE_ASSERT(sizeof(TData) == 4);

    union U
    {
      TData data;
      uint32_t i;
    } dst;

    dst.data = data;
    dst.i = YAE_N32_TO_BE(dst.i);
    return dst.data;
  }

  //----------------------------------------------------------------
  // ntob_64
  //
  template <typename TData>
  inline static TData
  ntob_64(TData data)
  {
    YAE_ASSERT(sizeof(TData) == 8);

    union U
    {
      TData data;
      uint64_t i;
    } dst;

    dst.data = data;
    dst.i = YAE_N64_TO_BE(dst.i);
    return dst.data;
  }

  //----------------------------------------------------------------
  // bton_16
  //
  template <typename TData>
  inline TData bton_16(TData data)
  { return yae::ntob_16<TData>(data); }

  //----------------------------------------------------------------
  // bton_32
  //
  template <typename TData>
  inline TData bton_32(TData data)
  { return yae::ntob_32<TData>(data); }

  //----------------------------------------------------------------
  // bton_64
  //
  template <typename TData>
  inline TData bton_64(TData data)
  { return yae::ntob_64<TData>(data); }


  //----------------------------------------------------------------
  // ntol_16
  //
  template <typename TData>
  inline static TData
  ntol_16(TData data)
  {
    YAE_ASSERT(sizeof(TData) == 2);

    union U
    {
      TData data;
      uint16_t i;
    } dst;

    dst.data = data;
    dst.i = YAE_N16_TO_LE(dst.i);
    return dst.data;
  }

  //----------------------------------------------------------------
  // ntol_32
  //
  template <typename TData>
  inline static TData
  ntol_32(TData data)
  {
    YAE_ASSERT(sizeof(TData) == 4);

    union U
    {
      TData data;
      uint32_t i;
    } dst;

    dst.data = data;
    dst.i = YAE_N32_TO_LE(dst.i);
    return dst.data;
  }

  //----------------------------------------------------------------
  // ntol_64
  //
  template <typename TData>
  inline static TData
  ntol_64(TData data)
  {
    YAE_ASSERT(sizeof(TData) == 8);

    union U
    {
      TData data;
      uint64_t i;
    } dst;

    dst.data = data;
    dst.i = YAE_N64_TO_LE(dst.i);
    return dst.data;
  }

  //----------------------------------------------------------------
  // lton_16
  //
  template <typename TData>
  inline TData lton_16(TData data)
  { return yae::ntol_16<TData>(data); }

  //----------------------------------------------------------------
  // lton_32
  //
  template <typename TData>
  inline TData lton_32(TData data)
  { return yae::ntol_32<TData>(data); }

  //----------------------------------------------------------------
  // lton_64
  //
  template <typename TData>
  inline TData lton_64(TData data)
  { return yae::ntol_64<TData>(data); }

}


#endif // YAE_ENDIAN_H_
