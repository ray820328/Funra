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

#include "cpl_filter.h"

#include <assert.h>
#include <string.h>
#include <stdlib.h>


/** @addtogroup cpl_image

    Usage:  define the following preprocessor symbols as needed, 
            then include this file
*/
/**@{*/

#ifndef inline
#define inline /* inline */
#endif

#ifndef PIX_TYPE
#error PIX_TYPE not defined
#endif

#ifndef PIX_MAX
#error PIX_MAX not defined
#endif

#ifndef PIX_MIN
#error PIX_MIN not defined
#endif

#define assure(x) assert(x)

#ifdef bool
#define mybool bool
#else
#define mybool unsigned char
#endif

#ifdef true
#define mytrue true
#else
#define mytrue 1
#endif

#ifdef false
#define myfalse false
#else
#define myfalse 0
#endif

/** @cond */
typedef struct ADDTYPE(dheap) {
    unsigned *heaps;
    /* Double binary heap and ADDTYPE(median) element. Values are pixel buffer indices in the
       current filtering window.
       heaps[0] ... heaps[m-1]    : a maximum heap of pixels with values less than
                                    ADDTYPE(median)
       heaps[m]                   : ADDTYPE(median)
       heaps[m+1] ... heaps[2m]   : a minimum heap of pixels with values greater 
                                    than ADDTYPE(median)
    */
    unsigned *B; /* Size Nx x Ny map from image positions to double heap positions.
                    This is the inverse map of the heaps[] array, 
                       B[heaps[i]] = i.
                    It allows constant time look-up of any pixel in the double heap
                    array.

                    Notice that only the pixels in the current window need to be
                    simultansously stored in B, therefore B could be compressed to
                    size (2rx+1)(2ry+1)^2. However, in that case the overall execution time
                    would be dominated by mapping pixels to positions in B.
                 */
    unsigned *SCA; /* Array Nx*(2Ry+1) of sorted column arrays. Values are image pixel
                      buffer indices. Each column array is sorted according to image
                      buffer pixel values. This buffer is laid out in column major
                      order to allow fast iteration within each column array (i.e.
                      the x'th column array is at
                      {S[x*(2Ry+1)], ..., S[x*(2Ry+1) + 2Ry]}) 
                   */
    const PIX_TYPE *image;
    unsigned rx; //kernel radius
    unsigned ry; //kernel radius
    unsigned Nx; //image width
    unsigned Ny; //image height
    unsigned m;  //kernel window half size, must be positive
    unsigned Rx;  //2rx + 1
    unsigned Ry;  //2ry + 1
} ADDTYPE(dheap);
/** @endcond */


/**
  @internal
  @brief  Swap two elements in the Sorted Column Array
  @param  SCA    Sorted Column Array
  @param  i      index in SCA array
  @param  j      index in SCA array
*/
#define S_SWAP(SCA, i, j) \
do { \
    const unsigned SCAi = SCA[i]; \
    const unsigned SCAj = SCA[j]; \
    SCA[i] = SCAj; \
    SCA[j] = SCAi; \
} while(0)


#define BITS 64
/**
  @internal
  @brief quick sort
  @param  SCA    sorted column array to be sorted
  @param  n      length of SCA
  @param  image  input image
  
  SCA is sorted in increasing order by image[SCA[i]]
 */
static void
ADDTYPE(qsort_int)(unsigned *SCA, 
          unsigned n, const PIX_TYPE *image)
{
    int i, ir, j, k, l;
    int i_stack[BITS];
    int j_stack ;
    int a;

    PIX_TYPE ia;

    //assure( n <= 1 << BITS );  /* generates warnings */
    /* true because no-one ever wants to filter with a radius larger than 2^BITS */

    ir = n ;
    l = 1 ;
    j_stack = 0 ;
    while (1) {
        if (ir-l < 7) {
            for (j=l+1 ; j<=ir ; j++) {
                a = SCA[j-1];
                ia = image[a];
                for (i=j-1 ; i>=1 ; i--) {
                    if (image[SCA[i-1]] <= ia) break;
                    SCA[i] = SCA[i-1];
                }
                SCA[i] = a;
            }
            if (j_stack == 0) break;
            ir = i_stack[j_stack-- -1];
            l  = i_stack[j_stack-- -1];
        } else {
            /* The following is buggy and has O(n^2) behaviour if the
               input array is sorted in reverse order.
               For maximum performance we should select the pivot randomly,
               (however this step is used only in the initialization of 
               the sorted column arrays, so be it) */
            k = (l+ir) >> 1;
            S_SWAP(SCA, k-1, l);
            if (image[SCA[l]] > image[SCA[ir-1]]) {
                S_SWAP(SCA, l, ir-1);
            }
            if (image[SCA[l-1]] > image[SCA[ir-1]]) {
                S_SWAP(SCA, l-1, ir-1);
            }
            if (image[SCA[l]] > image[SCA[l-1]]) {
                S_SWAP(SCA, l, l-1);
            }

            i = l+1;
            j = ir;
            a = SCA[l-1];
            ia = image[a];
            for (;;) {
                do i++; while (image[SCA[i-1]] < ia);
                do j--; while (image[SCA[j-1]] > ia);
                if (j < i) break;
                S_SWAP(SCA, i-1, j-1);
            }
            SCA[l-1] = SCA[j-1];
            SCA[j-1] = a;
            j_stack += 2;

            assert( j_stack <= BITS );

            if (ir-i+1 >= j-l) {
                i_stack[j_stack-1] = ir;
                i_stack[j_stack-2] = i;
                ir = j-1;
            } else {
                i_stack[j_stack-1] = j-1;
                i_stack[j_stack-2] = l;
                l = i;
            }
        }
    }

    return;
}

/**
  @internal
  @brief initialize sorted column array
  @param  dh     double heap
  @param  x      current column
*/
static void
ADDTYPE(SCA_init)(ADDTYPE(dheap) *dh, unsigned x)
{
    unsigned y;
    unsigned *SCAx = dh->SCA + (dh->Ry)*x;

    for (y = 0; y < dh->Ry; y++) {
        SCAx[y] = x + y*dh->Nx;
    }

    ADDTYPE(qsort_int)(SCAx,
              dh->Ry,
              dh->image);
    
    return;
}

/**
  @internal
  @brief  Update sorted column array
  @param  dh     double heap
  @param  x      current column
  @param  yold   image row of previous element
  @param  ynew   image row of new element
*/
static void
ADDTYPE(SCA_replace)(ADDTYPE(dheap) *dh, unsigned x, 
            unsigned yold, unsigned ynew)
{
    unsigned n = dh->Ry;
    unsigned *SCAx = dh->SCA + n*x; //start of current column array
    const PIX_TYPE *im = dh->image;

    unsigned i;
    unsigned aold = x + yold*dh->Nx;
    unsigned anew = x + ynew*dh->Nx;

    /* Do a linear search for the old pixel and replace it with
       the new.

       (Since SCAx is sorted according
       to pixel value, we could do a binary search here.
       This turns out to be no faster in practice,
       up to at least r = 64)
    */
    for (i = 0; i < n; i++) {
        if (SCAx[i] == aold) break;
    }
    SCAx[i] = anew;

    /* The new pixel is not necessarily in it the right place.
       Bubble until SCAx is again sorted.
    */
    if (i+1 < n && 
        im[SCAx[i  ]] >
        im[SCAx[i+1]]) {
        
        S_SWAP(SCAx, i  , i+1);
        i++;
        
        while (i+1 < n && 
               im[SCAx[i  ]] >
               im[SCAx[i+1]]) {
            S_SWAP(SCAx, i  , i+1);
            i++;
        }
    }
    else {
        while (i >= 1 && 
               im[SCAx[i  ]] <
               im[SCAx[i-1]]) {
            S_SWAP(SCAx, i  , i-1);
            i--;
        }
    }
    
    return;
}


/**
  @internal
  @brief  Swap elements in double heap
  @param  i      index in heaps array
  @param  j      index in heaps array
  @param  B      image to heap map
  @param  heaps  heaps array
*/
static void
ADDTYPE(HEAP_SWAP)(unsigned i, 
                   unsigned j,
                   unsigned int *B,
                   unsigned int *heaps)
{
    unsigned ai = heaps[i];
    unsigned aj = heaps[j];
    heaps[i] = aj;
    heaps[j] = ai;

    B[aj] = i;
    B[ai] = j;

  return;
}

/**
  @internal
  @brief  Partion double heaps array in ADDTYPE(median) and upper/lower values
  @param  dh     double heap with elements in no particular order
*/
static void
ADDTYPE(median)(ADDTYPE(dheap) *dh)
{
    unsigned       n = 2 * dh->m + 1;
    const unsigned k  = (n-1)/2;
    unsigned       lo = 0;
    unsigned       hi = n - 1;
    
    while (lo < hi) {
        /* Buggy. We should select the pivot randomly,
           or there is O(n^2) behaviour for certain inputs */
        const PIX_TYPE pivot = dh->image[dh->heaps[k]];
        unsigned i = lo;
        unsigned j = hi;

        do {
            while (dh->image[dh->heaps[i]] < pivot) i++;
            while (dh->image[dh->heaps[j]] > pivot) j--;
            if (i <= j) {
                if (i != j) ADDTYPE(HEAP_SWAP)(i, j, dh->B, dh->heaps);
                i++;
                j--;
            }
        } while (i <= j);
        
        if (j < k) lo = i;
        if (k < i) hi = j;
    }
    
    return;
}


/**
  @internal
  @brief bubble down (towards leaves) in upper heap
  @param  image   input image
  @param  B       inverse map
  @param  heaps   double heap array
  @param  m       heap size
  @param  m1      m + 1
  @param  root    double heap position
  @return final position
 */
static unsigned
ADDTYPE(bubble_down_gt)(const PIX_TYPE *image,
               unsigned *B,
               unsigned *heaps,
               unsigned m,
               unsigned m1,
               unsigned root)
{
    unsigned end = m-1;

    while (root * 2 + 1 <= end) {
        unsigned child = root*2 + 1;

        if (child < end && 
            image[heaps[m1 + child]] > 
            image[heaps[m1 + child + 1]]) {
            child = child + 1;
        }
        if (image[heaps[m1 + root]] > 
            image[heaps[m1 + child]]) {
            ADDTYPE(HEAP_SWAP)(m1 + root, m1 + child, B, heaps);
            root = child;
        }
        else {
            return root;
        }
    }
    return root;
}

/**
  @internal
  @brief  bubble up (towards root) in upper heap
  @param  image   input image
  @param  B       inverse map
  @param  heaps   double heap array
  @param  m1      heap size + 1
  @param  child   double heap position
  @return final position
 */
static unsigned
ADDTYPE(bubble_up_gt)(const PIX_TYPE *image,
             unsigned *B,
             unsigned *heaps, 
             unsigned m1,
             unsigned child)
{
    while (child > 0) {
        const unsigned parent = (child - 1) / 2;

        if (image[heaps[m1 + parent]] >
            image[heaps[m1 + child]]) {
            ADDTYPE(HEAP_SWAP)(m1 + parent, m1 + child, B, heaps);
            child = parent;
        }
        else return child;
    }
    return child;
}

/**
  @internal
  @brief  establish upper heap
  @param  dh     double heap
 */
static void
ADDTYPE(heapify_gt)(ADDTYPE(dheap) *dh)
{
    int start = dh->m/2-1;

    while (start >= 0) {
        ADDTYPE(bubble_down_gt)(dh->image,
                       dh->B,
                       dh->heaps,
                       dh->m,
                       dh->m+1, start);
        start = start - 1;
    }
    return;
}

/**
  @internal
  @brief  bubble down in lower heap
  @param  image   input image
  @param  B       inverse map
  @param  heaps   double heap array
  @param  end     double heap last position
  @param  root    double heap position
  @return final position
 */
static unsigned
ADDTYPE(bubble_down_lt)(const PIX_TYPE *image,
               unsigned *B,
               unsigned *heaps,
               unsigned end, unsigned root)
{
    while (root * 2 + 1 <= end) {
        unsigned child = root*2 + 1;
        
        if (child < end &&
            image[heaps[child]] <
            image[heaps[child + 1]]) {
            child = child + 1;
        }
        if (image[heaps[root]] <
            image[heaps[child]]) {
            ADDTYPE(HEAP_SWAP)(root, child, B, heaps);
            root = child;
        }
        else {
            return root;
        }
    }
    return root;
}

/**
  @internal
  @brief  bubble up in lower heap
  @param  image   input image
  @param  B       inverse map
  @param  heaps   double heap array
  @param  child   double heap position
  @return final position
 */
static unsigned
ADDTYPE(bubble_up_lt)(const PIX_TYPE *image,
             unsigned *B,
             unsigned *heaps, unsigned child)
{
    while (child > 0) {
        const unsigned parent = (child - 1) / 2;

        if (image[heaps[parent]] <
            image[heaps[child]]) {
            ADDTYPE(HEAP_SWAP)(parent, child, B, heaps);
            child = parent;
        }
        else return child;
    }
    return child;
}

/**
  @internal
  @brief  establish lower heap
  @param  dh     double heap
 */
static void
ADDTYPE(heapify_lt)(ADDTYPE(dheap) *dh)
{
    int start = dh->m/2-1;

    while (start >= 0) {
        ADDTYPE(bubble_down_lt)(dh->image,
                       dh->B,
                       dh->heaps,
                       dh->m-1, start);
        start = start - 1;
    }
    return;
} 

/**
  @internal
  @brief  Initialize double heap
  @param  dh    double heap with elements in no particular order
 */
static void
ADDTYPE(dheap_establish)(ADDTYPE(dheap) *dh)
{
    ADDTYPE(median)(dh);
    
    ADDTYPE(heapify_lt)(dh);
    ADDTYPE(heapify_gt)(dh);
    
    return;
}

/**
  @internal
  @brief  double heap constructor
  @param  image       input image
  @param  Nx          image width
  @param  Ny          image height
  @param  rx          filtering half-size in x
  @param  ry          filtering half-size in y
  @return newly allocated double heap which may be destroyed using ADDTYPE(dheap_delete)() 

  The double heap, inverse map and SCA invariants are established
  using the first image window
*/
static ADDTYPE(dheap) *
ADDTYPE(dheap_new)(const PIX_TYPE *image, unsigned Nx, unsigned Ny, unsigned rx, unsigned ry)
{
    ADDTYPE(dheap) *dh = malloc(sizeof(*dh));
    assure( dh != NULL );
    assure( rx > 0 || ry > 0 );

    dh->heaps = malloc( (2*rx+1)*(2*ry+1) * sizeof(*dh->heaps));
    dh->B     = malloc( Nx*Ny             * sizeof(*dh->B));
    dh->SCA   = malloc( Nx*(2*ry+1)       * sizeof(*dh->SCA));
    assure( dh->heaps != NULL );
    assure( dh->B != NULL );
    assure( dh->SCA != NULL );
    
    dh->image = image;
    dh->m     = ((2*rx+1)*(2*ry+1))/2;
    dh->Nx    = Nx;
    dh->Ny    = Ny;
    dh->rx    = rx;
    dh->ry    = ry;
    dh->Rx    = 2*rx+1;
    dh->Ry    = 2*ry+1;

    {
        unsigned k = 0;
        unsigned x, y;
        
        for (y = 0; y < dh->Ry; y++) {
            for (x = 0; x < dh->Rx; x++) {
                unsigned ai = x + y * dh->Nx;
                dh->heaps[k]  = ai;
                dh->B[ai] = k;
                k++;
            }
        }
    }
    ADDTYPE(dheap_establish)(dh);

    {
        unsigned x;
        for (x = 0; x < 2*dh->rx; x++)
        {
            ADDTYPE(SCA_init)(dh, x);
        }
    }

    return dh;
}

/**
  @internal
  @brief double heap destructor
  @param dh     double heap
 */
static void
ADDTYPE(dheap_delete)(ADDTYPE(dheap) *dh)
{
    free(dh->SCA);
    free(dh->B);
    free(dh->heaps);
    free(dh);

    return;
}

/**
  @internal
  @brief  get current ADDTYPE(median)
  @param  dh    double heap
  @return the ADDTYPE(median) element value
 */
static PIX_TYPE
ADDTYPE(dheap_median)(const ADDTYPE(dheap) *dh)
{
    return dh->image[dh->heaps[dh->m]];
}

/**
  @internal
  @brief Insert and remove element in double heap
  @param  image   input image
  @param  B       inverse map
  @param  heaps   double heap array
  @param  pprev_was_larger  Previous element was larger
  @param  pprev_was_smaller Previous element was smaller
  @param  m       heap size
  @param  m1      m + 1
  @param  anew    image index of pixel to be inserted
  @param  aold    image index of pixel to be removed

  Algorithm: The inverse map is used to quickly locate the old pixel, which is
  replaced, and the new element bubbled to a (non-unique) correct position until
  the double heap structure is re-established. The inverse map is maintained while
  swapping heap elements.

  The two flags remember if the previously inserted element was larger/smaller
  than the one it replaced. Both flags may be false, which means nothing is
  known.

  If the previous element was larger (smaller), then the current element
  we are inserting now is likely to be also larger (smaller). This
  information is used to decrease the number of comparisons needed to bubble
  the current element into place.

 */
static void
ADDTYPE(dheap_replace)(const PIX_TYPE *image,
              unsigned *B,
              unsigned *heaps,
              mybool   *pprev_was_larger,
              mybool   *pprev_was_smaller,
              unsigned m,
              unsigned m1,
              unsigned anew,
              unsigned aold)
{
    unsigned pos;

    /* Lookup position of old pixel in heaps structure, overwrite with
       new pixel */
    unsigned ci = B[aold]; 
    heaps[ci] = anew;
    B[anew] = ci;

    /* Move new element to a correct place. 
       Swap heaps priority element with ADDTYPE(median) as needed */
    if (ci < m) {
        /* Lower heap */
        if (*pprev_was_smaller) {
            pos = ADDTYPE(bubble_down_lt)(image, B, heaps, m-1,
                                 ci);
            if (pos == ci) pos = ADDTYPE(bubble_up_lt)(image, B, heaps, ci);
        }
        else 
            /* previous was larger or nothing is known */
        {
            /* Optimistic guess optimization here: bubble_up requires only one comparison
               and is faster than bubble_down. Therefore, in the case where nothing is known
               about the previous element, we try to bubble_up before bubble_down.
            */
            pos = ADDTYPE(bubble_up_lt)(image, B, heaps, ci);
            if (pos == ci) pos = ADDTYPE(bubble_down_lt)(image, B, heaps, m-1, ci);
        }
        
        if (pos == 0 &&
            image[heaps[0]] > image[heaps[m]]) {
            ADDTYPE(HEAP_SWAP)(0, m, B, heaps);
            
            if (image[heaps[m1]] < 
                image[heaps[m]]) {
                ADDTYPE(HEAP_SWAP)(m1, m, B, heaps);
                ADDTYPE(bubble_down_gt)(image, B, heaps, m, m1, 0);
            }
        }

        /* Finally prepare flags for the next call of this function.
           If final position and initial postions are the same,
           we cannot tell if this element was larger/smaller than the
           one it replaced (at least not without the cost of a comparison with
           the previous element, which may or may not pay off).
        */
        *pprev_was_larger  = (pos < ci);
        *pprev_was_smaller = (pos > ci);
        return;
    }
    else if (ci >= m1) {
        /* Upper heap */
        if (*pprev_was_larger) {
            pos = ADDTYPE(bubble_down_gt)(image, B, heaps, m, m1, ci-(m1));
            if (pos == ci-(m1)) 
                pos = ADDTYPE(bubble_up_gt)(image, B, heaps, m1, ci-(m1));
        }
        else {
            pos = ADDTYPE(bubble_up_gt)(image, B, heaps, m1, ci-(m1));
            if (pos == ci-(m1)) 
                pos = ADDTYPE(bubble_down_gt)(image, B, heaps, m, m1, ci-(m1));
        }

        if (pos == 0 &&
            image[heaps[m1]] < image[heaps[m]]) {
            ADDTYPE(HEAP_SWAP)(m1, m, B, heaps);
            
            if (image[heaps[0]] > image[heaps[m]]) {
                ADDTYPE(HEAP_SWAP)(0, m, B, heaps);
                ADDTYPE(bubble_down_lt)(image, B, heaps, m-1, 0);
            }
        }

        *pprev_was_smaller = (pos < ci-(m1));
        *pprev_was_larger  = (pos > ci-(m1));
        return;
    }
    else {
        /* The ADDTYPE(median) element was replaced */
        if (*pprev_was_smaller) {
            if (image[heaps[0]] > image[heaps[m]]) {
                ADDTYPE(HEAP_SWAP)(0, m, B, heaps);
                ADDTYPE(bubble_down_lt)(image, B, heaps, m-1, 0);
                *pprev_was_smaller = mytrue;
                *pprev_was_larger = myfalse;
                return;
            } 
            else if (image[heaps[m1]] < 
                     image[heaps[m]]) {
                ADDTYPE(HEAP_SWAP)(m1, m, B, heaps);
                ADDTYPE(bubble_down_gt)(image, B, heaps, m, m1, 0);
                *pprev_was_smaller = myfalse;
                *pprev_was_larger = mytrue;
                return;
            } else {
                *pprev_was_smaller = myfalse;
                *pprev_was_larger  = myfalse;
                return;
            }
        } else {
            if (image[heaps[m1]] < 
                image[heaps[m]]) {
                ADDTYPE(HEAP_SWAP)(m1, m, B, heaps);
                ADDTYPE(bubble_down_gt)(image, B, heaps, m, m1, 0);
                *pprev_was_smaller = myfalse;
                *pprev_was_larger = mytrue;
                return;
            } else if (image[heaps[0]] > image[heaps[m]]) {
                ADDTYPE(HEAP_SWAP)(0, m, B, heaps);
                ADDTYPE(bubble_down_lt)(image, B, heaps, m-1, 0);
                *pprev_was_smaller = mytrue;
                *pprev_was_larger = myfalse;
                return;
            } else {
                *pprev_was_smaller = myfalse;
                *pprev_was_larger  = myfalse;
                return;
            }
        }
    }
}

/**
  @internal
  @brief max of 3 elements
  @param  p0  element
  @param  p1  element
  @param  p2  element
  @return maximum element

  Cost: 2 comparisons
 */
static inline PIX_TYPE
ADDTYPE(max3)(PIX_TYPE p0,
     PIX_TYPE p1,
     PIX_TYPE p2)
{
    return (p0 > p1) 
        ?
        ((p0 > p2) ? p0 : p2)
        :
        ((p1 > p2) ? p1 : p2);
}

/**
  @internal
  @brief min of 3 elements
  @param  p0  element
  @param  p1  element
  @param  p2  element
  @return minimum element

  Cost: 2 comparisons
*/
static inline PIX_TYPE
ADDTYPE(min3)(PIX_TYPE p0,
     PIX_TYPE p1,
     PIX_TYPE p2)
{
    return (p0 < p1)
        ?
        ((p0 < p2) ? p0 : p2)
        :
        ((p1 < p2) ? p1 : p2);
}

/**
  @internal
  @brief ADDTYPE(median) of 5
  @param  p0  element
  @param  p1  element
  @param  p2  element
  @param  p3  element
  @param  p4  element
  @return ADDTYPE(median)

  Precondition: p0 <= p1 <= p2

  Cost:
  2 + 0 (probability 4/7) or 
  2 + 2 (probability 3/7)
  = 20/7 = 2.86 comparisons
 */
static inline PIX_TYPE
ADDTYPE(median5)(PIX_TYPE p0,
        PIX_TYPE p1,
        PIX_TYPE p2,
        PIX_TYPE p3,
        PIX_TYPE p4)
{
    /*
      This would be a one-off ADDTYPE(median) filter (i.e. rank 4th, 5th
      or 6th of 9). Saves some 35% execution time.

      return p1;
    */
    
    return (p3 < p1)
        ?
        ((p4 >= p1) ? p1 : ADDTYPE(max3)(p3, p4, p0))
        :
        ((p4 <= p1) ? p1 : ADDTYPE(min3)(p3, p4, p2));
}

/**
  @internal
  @brief ADDTYPE(median) of 9
  @param  p0  element
  @param  p1  element
  @param  p2  element
  @param  p3  element
  @param  p4  element
  @param  p5  element
  @param  p6  element
  @param  p7  element
  @param  p8  element
  @return ADDTYPE(median)

  Precondition: 
  p0 <= p1 <= p2
  p3 <= p4 <= p5
  p6 <= p7 <= p8
  p1 <= p4

  Cost:
  1 + 2.86 (probability 1/3) or
  2 + 2.86 (probability 2/3) 
  = 95/21 = 4.52 comparisons
*/
static inline PIX_TYPE
ADDTYPE(median9_2)(PIX_TYPE p0,
          PIX_TYPE p1,
          PIX_TYPE p2,
          PIX_TYPE p3,
          PIX_TYPE p4,
          PIX_TYPE p5,
          PIX_TYPE p6,
          PIX_TYPE p7,
          PIX_TYPE p8)
{
    return (p4 <= p7) 
        ? 
        ADDTYPE(median5)(p3, p4, p5, p2, p6)
        :
        ((p1 < p7) ? ADDTYPE(median5)(p6, p7, p8, p2, p3)
         : ADDTYPE(median5)(p0, p1, p2, p8, p3));
}
/**
  @internal
  @brief ADDTYPE(median) of 9
  @param  p0  element
  @param  p1  element
  @param  p2  element
  @param  p3  element
  @param  p4  element
  @param  p5  element
  @param  p6  element
  @param  p7  element
  @param  p8  element
  @return ADDTYPE(median)

  Precondition: 
  p0 <= p1 <= p2
  p3 <= p4 <= p5
  p6 <= p7 <= p8

  Cost: 1 + 4.52 = 116/21 = 5.52 comparisons
*/
static inline PIX_TYPE
ADDTYPE(median9_1)(PIX_TYPE p0,
          PIX_TYPE p1,
          PIX_TYPE p2,
          PIX_TYPE p3,
          PIX_TYPE p4,
          PIX_TYPE p5,
          PIX_TYPE p6,
          PIX_TYPE p7,
          PIX_TYPE p8)
{
    return (p1 < p4) ? ADDTYPE(median9_2)(p0, p1, p2, p3, p4, p5, p6, p7, p8)
             : ADDTYPE(median9_2)(p3, p4, p5, p0, p1, p2, p6, p7, p8);
}

/**
  @internal
  @brief sort 3 elements
  @param  p0  (out) rank 1
  @param  p1  (out) rank 2
  @param  p2  (out) rank 3
  @param  i0  (in) element
  @param  i1  (in) element
  @param  i2  (in) element

  Cost:
  2 (probability 1/3) or
  3 (probability 2/3) = 8/3 = 2.66 comparisons
*/
#define sort3(p0, p1, p2, i0, i1, i2)                         \
do {                                                          \
    if (i0 <= i1) {                                           \
        if (i1 <= i2) {                                       \
             p0 = i0; p1 = i1; p2 = i2;                       \
        } else if (i0 <= i2) {                                \
            /* i0, i2, i1 */                                  \
            p0 = i0; p1 = i2; p2 = i1;                        \
        } else {                                              \
            /*  i2, i0, i1 */                                 \
            p0 = i2; p1 = i0; p2 = i1;                        \
        }                                                     \
    } else {                                                  \
        if (i0 <= i2) {                                       \
            /* i1, i0, i2 */                                  \
            p0 = i1; p1 = i0; p2 = i2;                        \
        } else if (i1 < i2) {                                 \
            /* i1, i2, i0 */                                  \
            p0 = i1; p1 = i2; p2 = i0;                        \
        } else {                                              \
            /* i2, i1, i0 */                                  \
            p0 = i2; p1=i1; p2 = i0;                          \
        }                                                     \
    }                                                         \
} while(0)

/**
  @internal
  @brief fast 3x3 image ADDTYPE(median) filter
  @param in           image to filter
  @param out          pre-allocated output image,
                      must not overlap with the input image
  @param Nx           image width
  @param Ny           image height
  @param border_mode

  periodic boundaries
  
  WARNING: If this function is modified (only sections regarding 
  CPL_BORDER_FILTER), also the fill_chess function has to be modified, for
  consistency. Here, an alternate lower and upper median is taken, which has
  the same effect as the +- inf chess-like pattern.
*/
static void
ADDTYPE(filter_median_1)(const PIX_TYPE *in, PIX_TYPE *out,
                unsigned Nx, unsigned Ny, unsigned border_mode)
{
    register PIX_TYPE p0, p1, p2, p3, p4, p5;
    register PIX_TYPE p6 = (PIX_TYPE)0; /* Fix (false) uninit warning */
    register PIX_TYPE p7 = (PIX_TYPE)0; /* Fix (false) uninit warning */
    register PIX_TYPE p8 = (PIX_TYPE)0; /* Fix (false) uninit warning */
    const PIX_TYPE * i0 = in;
    const PIX_TYPE * i1 = i0 + Nx;
    const PIX_TYPE * i2 = i1 + Nx;
    const PIX_TYPE * istop = in + Nx*Ny; /* First pixel not to be accessed */
    unsigned m = 2;  /* lower ADDTYPE(median) of 6 elements */

    assure( Nx >= 3 );
    assure( Ny >= 3 );

    switch (border_mode) {
        case CPL_BORDER_FILTER: {
            PIX_TYPE buf6[6];
            unsigned j = 0;

            buf6[0] = i0[0];
            buf6[1] = i0[1];
            buf6[2] = i1[0];
            buf6[3] = i1[1];
            /* upper median */
            out[j++] = ADDTYPE(cpl_tools_get_kth)(buf6, 4, 2);

            for (; j < Nx-1; j++) {
                memcpy(buf6,   i0+j-1, 3*sizeof(*buf6));
                memcpy(buf6+3, i1+j-1, 3*sizeof(*buf6));
                out[j] = ADDTYPE(cpl_tools_get_kth)(buf6, 6, m);
                m = 5-m; /* alternate between lower/upper median */
            }
            buf6[0] = i0[Nx-2];
            buf6[1] = i0[Nx-1];
            buf6[2] = i1[Nx-2];
            buf6[3] = i1[Nx-1];
            out[j] = ADDTYPE(cpl_tools_get_kth)(buf6, 4, m-1);
            out += Nx;

            break;
        }
        case CPL_BORDER_COPY:
            /* From the first row copy all but the last pixel */
            (void)memcpy(out, in, (Nx-1)*sizeof(*out));
            out += Nx-1; /* Could also fall through since dimensions are same */
            break;
        case CPL_BORDER_NOP:
            out += Nx-1; /* Same as above, except no action taken */
            break;
        default:
        case CPL_BORDER_CROP:
            break;
    }

    if (border_mode == CPL_BORDER_FILTER) m = 2; /* lower ADDTYPE(median) */

    for (;i2 < istop; i0 += Nx, i1 += Nx, i2 += Nx, out += Nx) {

        unsigned char k = 2;
        unsigned j = 0;

        /* Start a new row */

        switch (border_mode) {
            case CPL_BORDER_FILTER: {
                PIX_TYPE buf6[6];

                buf6[0] = i0[0];
                buf6[1] = i1[0];
                buf6[2] = i2[0];
                buf6[3] = i0[1];
                buf6[4] = i1[1];
                buf6[5] = i2[1];
                out[0] = ADDTYPE(cpl_tools_get_kth)(buf6, 6, m);

                /* j must be incremented twice, but out[j] should
                   only be incremented once */
                out--;

                break;
            }
            case CPL_BORDER_CROP:
                out -= 2; /* Nx - 2 medians in each row */
                break;
            case CPL_BORDER_COPY: {
                /* Copy last pixel from previous row and
                   first pixel from current row */
                out[0] = (PIX_TYPE)i0[Nx-1];
                out[1] = (PIX_TYPE)i0[Nx];
                break;
            }
            default:
                break;
        }

        sort3(p0, p1, p2, i0[j], i1[j], i2[j]);
        j++;
        
        sort3(p3, p4, p5, i0[j], i1[j], i2[j]);
        j++;

        for (; j < Nx; j++) {

            if (k == 0) {
                k = 1;
                sort3(p0, p1, p2, i0[j], i1[j], i2[j]);
            }
            else if (k == 1) {
                k = 2;
                sort3(p3, p4, p5, i0[j], i1[j], i2[j]);
            }
            else {
                k = 0;
                sort3(p6, p7, p8, i0[j], i1[j], i2[j]);
            }
        
            out[j]
                = (PIX_TYPE)ADDTYPE(median9_1)(p0, p1, p2, p3, p4, p5, p6, p7, p8);

            /* The cost per pixel was determined by considering the 9! possible
               permutations of 9 different numbers and subsequently confirmed by
               experiment:

               2.66 + 5.52 = 172/21 = 8.19 comparisons per pixel
               3 + 3 + 4 = 10  (worst case)
               
               In the case of two or more equal elements the cost is always the
               same or less (down to 6 for all equal elements) due to the way the
               comparisons are made.

               If sorted column arrays were used the cost would be one less
               1.66 + 5.52 = 151/21 = 7.19
               (worst case 9)
               
               A rank 4th-6th filter would cost 2.86 comparisons less
               
               2.66 + 2.66 = 16/3 = 5.33
               
               or only 4.33 comparisons per pixel if SCAs are used.
            */
        }

        if (border_mode == CPL_BORDER_FILTER) {
            PIX_TYPE buf6[6];

            buf6[0] = i0[j-2];
            buf6[1] = i1[j-2];
            buf6[2] = i2[j-2];
            buf6[3] = i0[j-1];
            buf6[4] = i1[j-1];
            buf6[5] = i2[j-1];

            out[j] = ADDTYPE(cpl_tools_get_kth)(buf6, 6,
                                                ((Nx & 1) == 1) ? m : 5-m);
            m = 5-m;

            /* out was decremented, increment it back */
            out++;
        }
    }

    switch (border_mode) {
        case CPL_BORDER_FILTER: {
            PIX_TYPE buf6[6];
            unsigned j = 0;

            buf6[0] = i0[0];
            buf6[1] = i0[1];
            buf6[2] = i1[0];
            buf6[3] = i1[1];
            out[j++] = ADDTYPE(cpl_tools_get_kth)(buf6, 4, m-1);
            m = 5-m;

            for (; j < Nx-1; j++) {
                memcpy(buf6,   i0+j-1, 3*sizeof(*buf6));
                memcpy(buf6+3, i1+j-1, 3*sizeof(*buf6));
                out[j] = ADDTYPE(cpl_tools_get_kth)(buf6, 6, m);
                m = 5-m;
            }
            buf6[0] = i0[Nx-2];
            buf6[1] = i0[Nx-1];
            buf6[2] = i1[Nx-2];
            buf6[3] = i1[Nx-1];
            out[j] = ADDTYPE(cpl_tools_get_kth)(buf6, 4, m-1);

            break;
        }
        default:
        case CPL_BORDER_CROP:
            break;
        case CPL_BORDER_COPY: {
            /* From the 2nd last row copy last pixel */
            /* From the last row copy all pixels */
            (void)memcpy(out, istop-(Nx+1), (Nx+1)*sizeof(*out));
            break;
        }
    }

    return;
}

/**
  @internal
  @brief   Write values to image row
  @param in        image to fill
  @param nx        image width
  @param y         row (?) to fill
  @param xmin      first column to fill (inclusive)
  @param xmax      last column to fill (exclusive)
  @param val1      value to write first position (xmin, y)
  @param val2      other value

  Every second pixel is filled with val1, other pixels with val2
  
  WARNING: This function is duplicated from the code found in 
  tests/cpl_filter_body.h. Do not forget to keep both in sync.
  
*/
static void
ADDTYPE(fill_row)(PIX_TYPE *in, unsigned nx, unsigned y,
         unsigned xmin, unsigned xmax,
         PIX_TYPE val1, PIX_TYPE val2) 
{
    unsigned x = xmin;
    
    while (x+1 < xmax) {
        in[(x++) + y*nx] = val1;
        in[(x++) + y*nx] = val2;
    }
    if (x < xmax) {
        in[(x++) + y*nx] = val1;
    }

    return;
}

/**
  @internal
  @brief   Fill image border with +- infinity
  @param in_larger     image to fill
  @param in            input image
  @param Nx_larger     in_larger width
  @param Ny_larger     in_larger height
  @param Nx            in width
  @param Ny            in height
  @param rx            filter x-radius
  @param ry            filter y-radius
  
  WARNING: This function is duplicated from the code found in 
  tests/cpl_filter_body.h. Do not forget to keep both in sync.
  
  WARNING: If this function is modified , also the filter_median_1 function 
  has to be modified (only sections regarding CPL_BORDER_FILTER), for
  consistency. Here, an +- inf chess-like pattern is used, which has
  the same effect as the alternate lower and upper median used in 
  filter_median_1.
  
*/
void
ADDTYPE(cpl_image_filter_fill_chess)(PIX_TYPE *in_larger, const PIX_TYPE *in,
                                     unsigned Nx_larger, unsigned Ny_larger, 
                                     unsigned Nx, unsigned Ny, 
                                     unsigned rx, unsigned ry)
{
    unsigned y;
    
    /* Fill border with PIX_MAX if x+y is even and PIX_MIN if x+y is odd. 
       Loop over images in cache-friendly order.  */
    for (y = 0; y < ry; y++) {

        ADDTYPE(fill_row)(in_larger, Nx_larger, y,
                 0, Nx_larger,
                 (y & 1) == 0 ? PIX_MAX : PIX_MIN,
                 (y & 1) == 0 ? PIX_MIN : PIX_MAX);
    }

    for (y = ry; y < ry+Ny; y++) {

        ADDTYPE(fill_row)(in_larger, Nx_larger, y,
                 0, rx,
                 (y & 1) == 0 ? PIX_MAX : PIX_MIN,
                 (y & 1) == 0 ? PIX_MIN : PIX_MAX);
            
        (void)memcpy(in_larger + rx + y*Nx_larger,
                     in + (y-ry)*Nx, Nx*sizeof(*in));
            
        ADDTYPE(fill_row)(in_larger, Nx_larger, y,
                 rx + Nx, Nx_larger,
                 ((y + rx + Nx) & 1) == 0 ? PIX_MAX : PIX_MIN,
                 ((y + rx + Nx) & 1) == 0 ? PIX_MIN : PIX_MAX);
    }

    for (y = ry + Ny; y < Ny_larger; y++) {

        ADDTYPE(fill_row)(in_larger, Nx_larger, y,
                 0, Nx_larger,
                 (y & 1) == 0 ? PIX_MAX : PIX_MIN,
                 (y & 1) == 0 ? PIX_MIN : PIX_MAX);
    }
}

/**
  @internal
  @brief Fast, any bit-depth, image ADDTYPE(median) filter

  @param in          image to filter
  @param out         pre-allocated output image, may not overlap with the input image,
                     same size as input image
  @param Nx          image width
  @param Ny          image height
  @param rx          filtering half-size in x, non-negative
  @param ry          filtering half-size in y, non-negative
  @param border_mode Handling of the pixels near the border (the region
                    where part of the kernel would be outside the image)

  Supported border modes:
      CPL_BORDER_FILTER: Compute the ADDTYPE(median) of available values.
      CPL_BORDER_NOP:    The border region in the output image is not
                                   touched.
      CPL_BORDER_CROP:   The border region is removed. Consequently the
                                   size of the output image must be
                                   (Nx - 2rx) x (Ny - 2ry)
      CPL_BORDER_COPY:   The border is filled by copying the input pixels.
                                   

  Unsupported filter modes:
    In-place: Possible using a pixel buffer of dimension (Nx + 2Rx) * (1 + 2Ry)
  Unsupported border modes:
    Input extrapolation: The input image is extended to (Nx+2Rx) * (Ny+2Ry) and the halo
                         is filled using extrapolation of the input border.
    Mean: As any, except the arithmetic mean of the two central values is used.

            
  For each pixel in the image the ADDTYPE(median) of a (2*Rx + 1)(2*Ry + 1) window is 
  computed.

  The straightforward approach is to loop over the image and build and sort the 
  kernel array for every pixel. This leads to an O(RxRy log RxRy) algorithm, or 
  O(RxRy) if the ADDTYPE(median) is computed by repeated partitions.

  The basic idea in this implementation is to maintain a double heap structure of
  the running window pixels:

  _____
  \   / <-- minimum heap of elements greater than the ADDTYPE(median)
   \ /
    .   <-- ADDTYPE(median)

   / \  <-- maximum heap of elements less than the ADDTYPE(median)
  /   \
  -----

  With every 1 pixel shift of the running window (2Ry + 1) pixels have to be removed
  from the double heap, and another (2Ry + 1) pixels must be inserted. The worst
  case time for inserting/removing one element is two heap heights, O(log RxRy), so
  the overall time is O(Ry log RxRy).

  Various tricks improve the performance

      - The image is traversed row by row, alternating between left->right and
      right->left iteration. In this way the running window always moves by one
      pixel and the double heap structure is always reused.

      - A pixel is inserted into the double heap by overwriting a pixel (which
      is thereby removed) at an existing position, then the new pixel is bubbled 
      up/down until the heap structure is re-established. This is fastest if the
      new pixel happens to be initially inserted at a position close to its final
      position. To increase the chance of that happening, the (2Ry + 1) pixels to be
      removed are sorted, as are the (2Ry + 1) pixels to be inserted. Then the
      minimum "new" pixel replaces the minimum "old" pixel, the 2nd smallest new
      pixel replaces the 2nd smallest old pixel etc. This heuristical optimization 
      greatly reduces the number of bubble operations required.

      Rather than sorting every time the (2Ry + 1) new and old values (which would
      cost O(Ry log Ry), a sorted array of the current pixels is maintained for
      every column in the image. To update the sorted column arrays (SCA) only one
      pixel needs to be inserted/removed per iteration. When using simple linear
      search and insertion, the maintenance cost for the SCAs is O(Ry). A possible
      optimization not pursued here is to implement the SCAs as balanced binary
      search trees, which would reduce their maintenance cost to O(log Ry) (but
      the heap maintenance cost would still be O(Ry).

      - When inserting elements into the double heap, a memory is kept of whether
      the inserted element is larger/smaller. Due to the correlation between
      consecutive insertions this information can be used to guess that the next
      element will also be larger/smaller.

  Performance: While the theoretical time complexity is O(Ry log RxRy), the 
  empirical behaviour (for realistic size images with random, uncorrelated pixels
  from the same probability distribution) is O(Ry) with small constant coefficients.
  
  Rx = Ry = 1 is handled as a separate case.
 */
static void
ADDTYPE(filter_median)(const PIX_TYPE *in, PIX_TYPE *out,
              unsigned Nx, unsigned Ny,
              unsigned rx, unsigned ry,
              unsigned border_mode)
{
    unsigned out_Nx;
    ADDTYPE(dheap) *dh;
    unsigned y;
    unsigned x;
    int dx;

    mybool prev_was_larger  = myfalse;
    mybool prev_was_smaller = myfalse;


    assure( 2*rx + 1 <= Nx );
    assure( 2*ry + 1 <= Ny );

    if (rx == 1 && ry == 1) {
        ADDTYPE(filter_median_1)(in, out, Nx, Ny, border_mode);
        return;
    }

    if (rx == 0 && ry == 0) {
        /* The double heap structure must have non-empty heaps,
           so handle this (trivial) case explicitly.
           
           Works for all border modes (because there are no borders!).
        */

        (void)memcpy(out, in, (Nx*Ny)*sizeof(*out));

        return;
    }

    assure( border_mode == CPL_BORDER_NOP ||
            border_mode == CPL_BORDER_CROP ||
            border_mode == CPL_BORDER_COPY ||
            border_mode == CPL_BORDER_FILTER );

    if (border_mode == CPL_BORDER_FILTER) {
        /* Add a border of (width, height) = (rx, ry) to the input image.
           Fill with +-infinity in a chessboard pattern.

           Then filter this larger image using CPL_BORDER_CROP,
           and the result will come out right (because the added border region
           always contains the same number of +infinity and -infinity).
        */
        const unsigned Nx_larger = Nx + 2*rx;
        const unsigned Ny_larger = Ny + 2*ry;

        PIX_TYPE *in_larger = malloc(Nx_larger*Ny_larger*sizeof(*in_larger));
        assure( in_larger != NULL );

        ADDTYPE(cpl_image_filter_fill_chess)(in_larger, in, Nx_larger,
                                             Ny_larger, Nx, Ny, rx, ry);
        
        ADDTYPE(filter_median)(in_larger, out,
                      Nx_larger, Ny_larger,
                      rx, ry,
                      CPL_BORDER_CROP);

        free(in_larger);
        return;
    }

    assert( border_mode == CPL_BORDER_NOP ||
            border_mode == CPL_BORDER_CROP ||
            border_mode == CPL_BORDER_COPY);

    if (border_mode == CPL_BORDER_COPY) {
        (void)memcpy(out, in, (ry*Nx+rx)*sizeof(*out));
    }
    
    if (border_mode == CPL_BORDER_CROP) {
        /* Subtract a bit from the out pointer, so that
           indexing out[x + y*out_Nx] works */
        out_Nx = Nx - 2*rx;
        out -= rx + ry * out_Nx;
    }
    else out_Nx = Nx;

    dh = ADDTYPE(dheap_new)(in, Nx, Ny, rx, ry);
    
    /* Loop over image like this
       -->->->-
       -<-<-<--
       -->->->-  
       etc.
       so that the current window always moves by 1 pixel per iteration
    */
    for (y = 0 + dh->ry, dx = 1;
         y < dh->Ny - dh->ry; 
         y++, dx = -dx) {

        const unsigned xfirst = (dx ==  1) ? dh->rx : dh->Nx-1 - dh->rx;
        const unsigned xlast  = (dx == -1) ? dh->rx : dh->Nx-1 - dh->rx;

        if (border_mode == CPL_BORDER_COPY && dx == -1 &&
            y != dh->Ny - dh->ry - 1) {

                 PIX_TYPE *o = out + xfirst + 1 + y*dh->Nx;
            const PIX_TYPE *i = in  + xfirst + 1 + y*dh->Nx;

            /* in- and out-types have identical sizes */
            (void)memcpy(o, i, 2*rx*sizeof(*o));
        }

        for (x = xfirst; x != xlast + dx; x += dx) {

            if (y == dh->ry) {
                ADDTYPE(SCA_init)(dh, x + dx * dh->rx);
            }
            else {
                /* Most frequent case. Update just 1 pixel in 1 sorted column array */
                ADDTYPE(SCA_replace)(dh, x + dx * dh->rx,
                            y - dh->ry - 1,
                            y + dh->ry);
            }
            
            if (x == xfirst) {
                
                if (y != dh->ry) {
                    /* Window just moved to next row,
                       update pixels at window bottom row */
                    unsigned i;
                    for (i  = x - dx*dh->rx; 
                         i != x + dx*dh->rx; i += dx) {
                        ADDTYPE(SCA_replace)(dh, i,
                                    y - dh->ry - 1,
                                    y + dh->ry);
                        
                        ADDTYPE(dheap_replace)(dh->image,
                                      dh->B,
                                      dh->heaps,
                                      &prev_was_larger,
                                      &prev_was_smaller,
                                      dh->m,
                                      dh->m+1,
                                      i + ((y + dh->ry    ) * dh->Nx),
                                      i + ((y - dh->ry - 1) * dh->Nx));
                    }
                    
                    ADDTYPE(dheap_replace)(dh->image,
                                  dh->B,
                                  dh->heaps,
                                  &prev_was_larger,
                                  &prev_was_smaller,
                                  dh->m,
                                  dh->m+1,
                                  i + ((y + dh->ry    ) * dh->Nx),
                                  i + ((y - dh->ry - 1) * dh->Nx));
                }
            }
            else {
                /* Most frequent case. Replace pixels in xold column
                   with pixels from xnew column */
                const unsigned xnew = x + dx * dh->rx;
                const unsigned xold = x - dx * (dh->rx+1);
                const unsigned *SCAxnew = dh->SCA + dh->Ry*xnew;
                const unsigned *SCAxold = dh->SCA + dh->Ry*xold;
                
                const PIX_TYPE *image = dh->image;
                unsigned *B = dh->B;
                unsigned *heaps = dh->heaps;
                const unsigned m = dh->m;
                const unsigned m1 = m+1;
                unsigned i;
                
                /* Loop through old/new sorted column arrays and replace
                   old pixels by new pixels of the same rank (i.e. position in
                   sorted column array). 
                   
                   This heuristic increases the chance that the new pixel is close
                   to its final position when inserted into the double heap structure.
                */
                for (i = 0; i < dh->Ry; i++) {
                    const unsigned anew = SCAxnew[i];
                    const unsigned aold = SCAxold[i];
                    
                    ADDTYPE(dheap_replace)(image, B, heaps,
                                  &prev_was_larger,
                                  &prev_was_smaller,
                                  m, m1, anew, aold);
                }
            }

            out[x + y*out_Nx] = (PIX_TYPE)ADDTYPE(dheap_median)(dh);

        } /* for x */
        
        if (border_mode == CPL_BORDER_COPY && dx == 1 &&
            y != dh->Ny - dh->ry - 1) {

                 PIX_TYPE *o = out + xlast + 1 + y*dh->Nx;
            const PIX_TYPE *i = in  + xlast + 1 + y*dh->Nx;

            /* in- and out- types have identical sizes */
            (void)memcpy(o, i, 2*rx*sizeof(*o));
        }

    } /* for y */

    if (border_mode == CPL_BORDER_COPY) {
        out += (Ny-ry)*dh->Nx - rx;
        in  += (Ny-ry)*dh->Nx - rx;
        /* in- and out- types have identical sizes */
        (void)memcpy(out, in, (ry*Nx + rx)*sizeof(*out));
    }

    ADDTYPE(dheap_delete)(dh);
    return;
}

static
void ADDTYPE(cpl_filter_median_fast)(void * out, const void * in,
                                     cpl_size Nx, cpl_size Ny, cpl_size hsizex,
                                     cpl_size hsizey, cpl_border_mode mode)
{

    const unsigned Nxu     = (const unsigned)Nx;
    const unsigned Nyu     = (const unsigned)Ny;
    const unsigned hsizexu = (const unsigned)hsizex;
    const unsigned hsizeyu = (const unsigned)hsizey;

    if ((cpl_size)Nxu == Nx && (cpl_size)Nyu == Ny &&
        (cpl_size)hsizexu == hsizex && (cpl_size)hsizeyu == hsizey) {
        ADDTYPE(filter_median)(in, out, (unsigned)Nx, (unsigned)Ny,
                               (unsigned)hsizex, (unsigned)hsizey,
                               (unsigned)mode);
    } else {
        cpl_error_set_(CPL_ERROR_UNSUPPORTED_MODE);
    }

    return;
}


/**@}*/

#undef IN_TYPE
#undef PIX_TYPE
#undef PIX_MIN
#undef PIX_MAX
