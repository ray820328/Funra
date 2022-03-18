/*
 * This file is part of the ESO Common Pipeline Library
 * Copyright (C) 2001-2017 European Southern Observatory
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Perform the bitwise operation
  @param  self    Pre-allocated buffer to hold the result
  @param  first   First operand, or NULL for an in-place operation
  @param  second  Second operand
  @param  nxy     The number of elements
  @return void
  @note No error checking in this internal function!

 */
/*----------------------------------------------------------------------------*/
void APPENDOPER(CPL_MASK_BINARY_WOPER)(cpl_binary * self,
                                       const cpl_binary * first,
                                       const cpl_binary * second,
                                       size_t nxy)

{

    CPL_MASK_REGISTER_TYPE      * word0 = (CPL_MASK_REGISTER_TYPE*)self;
    const CPL_MASK_REGISTER_TYPE* word1 = first
        ? (const CPL_MASK_REGISTER_TYPE*)first
        : (const CPL_MASK_REGISTER_TYPE*)self;
    const CPL_MASK_REGISTER_TYPE* word2 = (const CPL_MASK_REGISTER_TYPE*)second;
    const size_t                  nword = nxy / CPL_MASK_REGISTER_SIZE;
    size_t                        i;


    for (i = 0; i < nword; i++) {
#if CPL_MASK_REGISTER_SIZE == 16
#ifndef __SSE2__
#error "For 16-byte processing __SSE2__ must be defined"
#endif

        /* Assume unaligned data. With load of aligned data gcc can perform the
           operation directly on an address operand and thus avoid one load */
        const __m128i v1 = _mm_loadu_si128(word1 + i);
        const __m128i v2 = _mm_loadu_si128(word2 + i);
        const __m128i v0 = OPER2MM(CPL_MASK_BINARY_WOPER)(v1, v2);
        _mm_storeu_si128(word0 + i, v0);
#else
        word0[i] = word1[i] CPL_MASK_BINARY_OPER word2[i];
#endif
    }
    i *= CPL_MASK_REGISTER_SIZE;

    if (first == NULL) {
        for (; i < nxy; i++) {
            self[i] = self[i] CPL_MASK_BINARY_OPER second[i];
        }
    } else {
        for (; i < nxy; i++) {
            self[i] = first[i] CPL_MASK_BINARY_OPER second[i];
        }
    }
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Perform the bitwise operation
  @param  self    Pre-allocated buffer to hold the result
  @param  first   First operand, or NULL for an in-place operation
  @param  second  Second operand (scalar)
  @param  nxy     The number of elements
  @return void
  @note No error checking in this internal function!

 */
/*----------------------------------------------------------------------------*/
void APPENDOPERS(CPL_MASK_BINARY_WOPER)(cpl_binary * self,
                                        const cpl_binary * first,
                                        cpl_bitmask second,
                                        size_t nxy)

{

    CPL_MASK_REGISTER_TYPE      * word0 = (CPL_MASK_REGISTER_TYPE*)self;
    const CPL_MASK_REGISTER_TYPE* word1 = first
        ? (const CPL_MASK_REGISTER_TYPE*)first
        : (const CPL_MASK_REGISTER_TYPE*)self;
    const size_t                  nword = nxy / CPL_MASK_REGISTER_SIZE;
    size_t                        i;


    for (i = 0; i < nword; i++) {
#if CPL_MASK_REGISTER_SIZE == 16
#ifndef __SSE2__
#error "For 16-byte processing __SSE2__ must be defined"
#endif
        /* Process two cpl_bitmasks in each iteration */
        /* Assume unaligned data. With load of aligned data gcc can perform the
           operation directly on an address operand and thus avoid one load */
        const __m128i v1 = _mm_loadu_si128(word1 + i);
        const __m128i v2 = _mm_set1_epi64(cpl_m_from_int64(second));
        const __m128i v0 = OPER2MM(CPL_MASK_BINARY_WOPER)(v1, v2);
        _mm_storeu_si128(word0 + i, v0);
#else
        word0[i] = word1[i] CPL_MASK_BINARY_OPER (CPL_MASK_REGISTER_TYPE)second;
#endif
    }

    i *= CPL_MASK_REGISTER_SIZE;

#if CPL_MASK_REGISTER_SIZE > 4
#if CPL_MASK_REGISTER_SIZE > 8
    if (i + 8 <= nxy) {
        /* Handle the remaining uint64_t */
        uint64_t       * self8  = (uint64_t *)self;
        const uint64_t * first8 = (const uint64_t *)(first ? first : self);

        self8[i/8] = first8[i/8] CPL_MASK_BINARY_OPER (uint64_t)second;
        i += 8;
    }
#endif
    if (i + 4 <= nxy) {
        /* Handle the remaining uint32_t */
        uint32_t       * self4  = (uint32_t *)self;
        const uint32_t * first4 = (const uint32_t *)(first ? first : self);

        self4[i/4] = first4[i/4] CPL_MASK_BINARY_OPER (uint32_t)second;
        i += 4;
    }
#endif

    if (first == NULL) {
        for (; i < nxy; i++) {
            self[i] = self[i] CPL_MASK_BINARY_OPER (cpl_binary)second;
        }
    } else {
        for (; i < nxy; i++) {
            self[i] = first[i] CPL_MASK_BINARY_OPER (cpl_binary)second;
        }
    }
}
