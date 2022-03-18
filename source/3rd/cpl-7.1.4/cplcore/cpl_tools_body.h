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

#undef ADDTYPE
#define ADDTYPE(a) CONCAT2X(a, CPL_TYPE_NAME)

#ifdef CPL_TYPE_IS_NUM

static int ADDTYPE(compar_ascn)(const void *, const void *) CPL_ATTR_NONNULL;

static int ADDTYPE(compar_desc)(const void *, const void *) CPL_ATTR_NONNULL;

static void ADDTYPE(cpl_tools_get_median_3)(CPL_TYPE *) CPL_ATTR_NONNULL;

static void ADDTYPE(cpl_tools_get_median_5)(CPL_TYPE *) CPL_ATTR_NONNULL;

static void ADDTYPE(cpl_tools_get_median_6)(CPL_TYPE *) CPL_ATTR_NONNULL;

static void ADDTYPE(cpl_tools_get_median_7)(CPL_TYPE *) CPL_ATTR_NONNULL;

static void ADDTYPE(cpl_tools_get_median_9)(CPL_TYPE *) CPL_ATTR_NONNULL;

static void ADDTYPE(cpl_tools_get_median_25)(CPL_TYPE *) CPL_ATTR_NONNULL;

static void ADDTYPE(cpl_tools_get_0th)(CPL_TYPE *, size_t) CPL_ATTR_NONNULL;


/* Swap macro */
#define CPL_TYPE_SWAP(a, b) { register const CPL_TYPE t=(a); (a)=(b); (b)=t; }


/* Guarded swap macro */
#define CPL_TYPE_SORT(a, b) { if ((a) > (b)) CPL_TYPE_SWAP((a), (b)); }


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Get the smallest value in a numerical array
  @param  self The array to permute and request from
  @param  n    The number of elements in the array, must be at least 1
  @return void
  @see cpl_tools_get_kth
  @note Since the function is static its error checking is disabled
  @note cpl_tools_get_0th(self, n) is the same as
  (void)cpl_tools_get_kth(self, n, 0)

 */
/*----------------------------------------------------------------------------*/
static void ADDTYPE(cpl_tools_get_0th)(CPL_TYPE * self, size_t n)
{
    CPL_TYPE min = self[0];
    size_t amin = 0;
    size_t i;

    for (i = 1; i < n; i++) {
        if (self[i] < min) {
            amin = i;
            min = self[i];
        }
    }

    CPL_TYPE_SWAP(self[0], self[amin]);
}

#ifdef CPL_TYPE_IS_TYPE_PIXEL

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Flip (reverse the order of) the elements of the array
  @param  self The array to flip in place
  @param  n    The array size, nop unless greater than 1
  @return void
  @note Since the function is not exported its error checking is disabled

 */
/*----------------------------------------------------------------------------*/
void ADDTYPE(cpl_tools_flip)(CPL_TYPE * self, size_t n)
{
    for (size_t i = 0; i < n/2; i++) {
        CPL_TYPE_SWAP(self[i], self[n-1-i]);
    }
}
#endif
/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Get the median of a numerical array
  @param  self The array to permute and request from
  @param  n   the number of elements
  @return The median
  @note Since the function is not exported its error checking is disabled

  For a finite population or sample, the median is the middle value of an odd
  number of values (arranged in ascending order) or any value between the two
  middle values of an even number of values.
  For an even number of elements in the array, the mean of the two central
  values is returned. Note that in this case, the median might not be a value
  of the input array. Also, note that in the case of integer data types, 
  the division by 2 is performed with integer arithmetic.
  Consider casting your integer array to float if that is not the desired 
  behavior.  

  The function is optimized for 3, 5, 6, 7, 9 and 25 elements.

  After a successful call, self is permuted so elements less than the median
  have lower indices, while elements greater than the median have higher
  indices.

  Example: the median of (1 2 3) is 2 and the median of (1. 2. 3. 4.) is 2.5.
  The median of the integers (1 2 3 4) is 2 (due to the integer arithmetic).
 */
/*----------------------------------------------------------------------------*/
CPL_TYPE ADDTYPE(cpl_tools_get_median)(CPL_TYPE * self, cpl_size n)
{

    switch (n) {
    case 3:
        ADDTYPE(cpl_tools_get_median_3)(self);
        break;

    case 5:
        ADDTYPE(cpl_tools_get_median_5)(self);
        break;

    case 6:
        ADDTYPE(cpl_tools_get_median_6)(self);
        break;

    case 7:
        ADDTYPE(cpl_tools_get_median_7)(self);
        break;

    case 9:
        ADDTYPE(cpl_tools_get_median_9)(self);
        break;

    case 25:
        ADDTYPE(cpl_tools_get_median_25)(self);
        break;

    default:
        if (n & 1) {
            /* Odd number */
            (void)ADDTYPE(cpl_tools_quickselection)(self, n, (n-1)/2);
        } else {
            /* Even number */
            (void)ADDTYPE(cpl_tools_quickselection)(self, n, n/2-1);
            ADDTYPE(cpl_tools_get_0th)(self + n / 2, n / 2);
        }
    }

    return (n & 1)
        ? self[(n-1)/2] /* Odd number */
        : self[n/2-1] + (self[n/2]-self[n/2-1])/((CPL_TYPE)2); /* Even number */
}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Optimized median computation for 3 elements (via 3 guarded swaps)
  @param  self  Array to sort for median
  @return void
  @note   Since the function is not exported its error checking is disabled
  @see    cpl_tools_get_median_double()


  http://ndevilla.free.fr/median/median/

 */
/*----------------------------------------------------------------------------*/
static void ADDTYPE(cpl_tools_get_median_3)(CPL_TYPE * self)
{
#ifdef CPL_TOOLS_STRICT_ERROR_CHECKING
    cpl_ensure(self != NULL, CPL_ERROR_NULL_INPUT, (CPL_TYPE)0);
#endif

    CPL_TYPE_SORT(self[0], self[1]);
    CPL_TYPE_SORT(self[1], self[2]);
    CPL_TYPE_SORT(self[0], self[1]);

}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Optimized median computation for 5 elements (via 7 guarded swaps)
  @param  self  Array to (partially) sort for median
  @return void
  @note   Since the function is not exported its error checking is disabled
  @see    cpl_tools_get_median_double()

  http://ndevilla.free.fr/median/median/

  found on sci.image.processing
  cannot go faster unless assumptions are made

 */
/*----------------------------------------------------------------------------*/
static void ADDTYPE(cpl_tools_get_median_5)(CPL_TYPE * self)
{
#ifdef CPL_TOOLS_STRICT_ERROR_CHECKING
    cpl_ensure(self != NULL, CPL_ERROR_NULL_INPUT, (CPL_TYPE)0);
#endif

    CPL_TYPE_SORT(self[0],self[1]); CPL_TYPE_SORT(self[3],self[4]);
    CPL_TYPE_SORT(self[0],self[3]); CPL_TYPE_SORT(self[1],self[4]);
    CPL_TYPE_SORT(self[1],self[2]); CPL_TYPE_SORT(self[2],self[3]);
    CPL_TYPE_SORT(self[1],self[2]);

}



/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Optimized median computation for 6 elements (via 13 guarded swaps)
  @param  self  Array to (partially) sort for median
  @return void
  @note   Since the function is not exported its error checking is disabled
  @see    cpl_tools_get_median_double()

  http://ndevilla.free.fr/median/median/

  from Christoph_John@gmx.de
  based on a selection network which was proposed in
  "FAST, EFFICIENT MEDIAN FILTERS WITH EVEN LENGTH WINDOWS"
  J.P. HAVLICEK, K.A. SAKADY, G.R.KATZ
  If you need larger even length kernels check the paper


 */
/*----------------------------------------------------------------------------*/
static void ADDTYPE(cpl_tools_get_median_6)(CPL_TYPE * self)
{
#ifdef CPL_TOOLS_STRICT_ERROR_CHECKING
    cpl_ensure(self != NULL, CPL_ERROR_NULL_INPUT, (CPL_TYPE)0);
#endif

    CPL_TYPE_SORT(self[1], self[2]); CPL_TYPE_SORT(self[3],self[4]);
    CPL_TYPE_SORT(self[0], self[1]); CPL_TYPE_SORT(self[2],self[3]);
    CPL_TYPE_SORT(self[4],self[5]); CPL_TYPE_SORT(self[1], self[2]);
    CPL_TYPE_SORT(self[3],self[4]); CPL_TYPE_SORT(self[0], self[1]);
    CPL_TYPE_SORT(self[2],self[3]); CPL_TYPE_SORT(self[4],self[5]);
    CPL_TYPE_SORT(self[1], self[2]); CPL_TYPE_SORT(self[3],self[4]);
    CPL_TYPE_SORT(self[2], self[3]);

}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Optimized median computation for 7 elements (via 13 guarded swaps)
  @param  self  Array to (partially) sort for median
  @return void
  @note   Since the function is not exported its error checking is disabled
  @see    cpl_tools_get_median_double()

  http://ndevilla.free.fr/median/median/

  found on sci.image.processing
  cannot go faster unless assumptions are made


 */
/*----------------------------------------------------------------------------*/
static void ADDTYPE(cpl_tools_get_median_7)(CPL_TYPE * self)
{
#ifdef CPL_TOOLS_STRICT_ERROR_CHECKING
    cpl_ensure(self != NULL, CPL_ERROR_NULL_INPUT, (CPL_TYPE)0);
#endif

    CPL_TYPE_SORT(self[0], self[5]); CPL_TYPE_SORT(self[0], self[3]);
    CPL_TYPE_SORT(self[1], self[6]); CPL_TYPE_SORT(self[2], self[4]);
    CPL_TYPE_SORT(self[0], self[1]); CPL_TYPE_SORT(self[3], self[5]);
    CPL_TYPE_SORT(self[2], self[6]); CPL_TYPE_SORT(self[2], self[3]);
    CPL_TYPE_SORT(self[3], self[6]); CPL_TYPE_SORT(self[4], self[5]);
    CPL_TYPE_SORT(self[1], self[4]); CPL_TYPE_SORT(self[1], self[3]);
    CPL_TYPE_SORT(self[3], self[4]);

}



/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Optimized median computation for 9 elements (via 19 guarded swaps)
  @param  self  Array to partially sort for median
  @return void
  @note   Since the function is not exported its error checking is disabled
  @see    cpl_tools_get_median_double()

  Formula from:
  XILINX XCELL magazine, vol. 23 by John L. Smith

 */
/*----------------------------------------------------------------------------*/
static void ADDTYPE(cpl_tools_get_median_9)(CPL_TYPE * self)
{
#ifdef CPL_TOOLS_STRICT_ERROR_CHECKING
    cpl_ensure(self != NULL, CPL_ERROR_NULL_INPUT, (CPL_TYPE)0);
#endif

    CPL_TYPE_SORT(self[1], self[2]); CPL_TYPE_SORT(self[4], self[5]);
    CPL_TYPE_SORT(self[7], self[8]); CPL_TYPE_SORT(self[0], self[1]);
    CPL_TYPE_SORT(self[3], self[4]); CPL_TYPE_SORT(self[6], self[7]);
    CPL_TYPE_SORT(self[1], self[2]); CPL_TYPE_SORT(self[4], self[5]);
    CPL_TYPE_SORT(self[7], self[8]); CPL_TYPE_SORT(self[0], self[3]);
    CPL_TYPE_SORT(self[5], self[8]); CPL_TYPE_SORT(self[4], self[7]);
    CPL_TYPE_SORT(self[3], self[6]); CPL_TYPE_SORT(self[1], self[4]);
    CPL_TYPE_SORT(self[2], self[5]); CPL_TYPE_SORT(self[4], self[7]);
    CPL_TYPE_SORT(self[4], self[2]); CPL_TYPE_SORT(self[6], self[4]);
    CPL_TYPE_SORT(self[4], self[2]);

}



/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Optimized median computation for 25 elements (via 99 guarded swaps)
  @param  self  Array to (partially) sort for median
  @return void
  @note   Since the function is not exported its error checking is disabled
  @see    cpl_tools_get_median_double()

  http://ndevilla.free.fr/median/median/

  In theory, cannot go faster without assumptions on the signal.
  Code taken from Graphic Gems.


 */
/*----------------------------------------------------------------------------*/
static void ADDTYPE(cpl_tools_get_median_25)(CPL_TYPE * self)
{
#ifdef CPL_TOOLS_STRICT_ERROR_CHECKING
    cpl_ensure(self != NULL, CPL_ERROR_NULL_INPUT, (CPL_TYPE)0);
#endif

    CPL_TYPE_SORT(self[0], self[1]);   CPL_TYPE_SORT(self[3], self[4]);
    CPL_TYPE_SORT(self[2], self[4]); CPL_TYPE_SORT(self[2], self[3]);
    CPL_TYPE_SORT(self[6], self[7]);   CPL_TYPE_SORT(self[5], self[7]);
    CPL_TYPE_SORT(self[5], self[6]);   CPL_TYPE_SORT(self[9], self[10]);
    CPL_TYPE_SORT(self[8], self[10]); CPL_TYPE_SORT(self[8], self[9]);
    CPL_TYPE_SORT(self[12], self[13]); CPL_TYPE_SORT(self[11], self[13]);
    CPL_TYPE_SORT(self[11], self[12]); CPL_TYPE_SORT(self[15], self[16]);
    CPL_TYPE_SORT(self[14], self[16]); CPL_TYPE_SORT(self[14], self[15]);
    CPL_TYPE_SORT(self[18], self[19]); CPL_TYPE_SORT(self[17], self[19]);
    CPL_TYPE_SORT(self[17], self[18]); CPL_TYPE_SORT(self[21], self[22]);
    CPL_TYPE_SORT(self[20], self[22]); CPL_TYPE_SORT(self[20], self[21]);
    CPL_TYPE_SORT(self[23], self[24]); CPL_TYPE_SORT(self[2], self[5]);
    CPL_TYPE_SORT(self[3], self[6]);   CPL_TYPE_SORT(self[0], self[6]);
    CPL_TYPE_SORT(self[0], self[3]); CPL_TYPE_SORT(self[4], self[7]);
    CPL_TYPE_SORT(self[1], self[7]);   CPL_TYPE_SORT(self[1], self[4]);
    CPL_TYPE_SORT(self[11], self[14]); CPL_TYPE_SORT(self[8], self[14]);
    CPL_TYPE_SORT(self[8], self[11]); CPL_TYPE_SORT(self[12], self[15]);
    CPL_TYPE_SORT(self[9], self[15]);  CPL_TYPE_SORT(self[9], self[12]);
    CPL_TYPE_SORT(self[13], self[16]); CPL_TYPE_SORT(self[10], self[16]);
    CPL_TYPE_SORT(self[10], self[13]); CPL_TYPE_SORT(self[20], self[23]);
    CPL_TYPE_SORT(self[17], self[23]); CPL_TYPE_SORT(self[17], self[20]);
    CPL_TYPE_SORT(self[21], self[24]); CPL_TYPE_SORT(self[18], self[24]);
    CPL_TYPE_SORT(self[18], self[21]); CPL_TYPE_SORT(self[19], self[22]);
    CPL_TYPE_SORT(self[8], self[17]);  CPL_TYPE_SORT(self[9], self[18]);
    CPL_TYPE_SORT(self[0], self[18]);  CPL_TYPE_SORT(self[0], self[9]);
    CPL_TYPE_SORT(self[10], self[19]); CPL_TYPE_SORT(self[1], self[19]);
    CPL_TYPE_SORT(self[1], self[10]);  CPL_TYPE_SORT(self[11], self[20]);
    CPL_TYPE_SORT(self[2], self[20]);  CPL_TYPE_SORT(self[2], self[11]);
    CPL_TYPE_SORT(self[12], self[21]); CPL_TYPE_SORT(self[3], self[21]);
    CPL_TYPE_SORT(self[3], self[12]);  CPL_TYPE_SORT(self[13], self[22]);
    CPL_TYPE_SORT(self[4], self[22]);  CPL_TYPE_SORT(self[4], self[13]);
    CPL_TYPE_SORT(self[14], self[23]); CPL_TYPE_SORT(self[5], self[23]);
    CPL_TYPE_SORT(self[5], self[14]);  CPL_TYPE_SORT(self[15], self[24]);
    CPL_TYPE_SORT(self[6], self[24]);  CPL_TYPE_SORT(self[6], self[15]);
    CPL_TYPE_SORT(self[7], self[16]); CPL_TYPE_SORT(self[7], self[19]);
    CPL_TYPE_SORT(self[13], self[21]); CPL_TYPE_SORT(self[15], self[23]);
    CPL_TYPE_SORT(self[7], self[13]);  CPL_TYPE_SORT(self[7], self[15]);
    CPL_TYPE_SORT(self[1], self[9]); CPL_TYPE_SORT(self[3], self[11]);
    CPL_TYPE_SORT(self[5], self[17]);  CPL_TYPE_SORT(self[11], self[17]);
    CPL_TYPE_SORT(self[9], self[17]);  CPL_TYPE_SORT(self[4], self[10]);
    CPL_TYPE_SORT(self[6], self[12]); CPL_TYPE_SORT(self[7], self[14]);
    CPL_TYPE_SORT(self[4], self[6]);   CPL_TYPE_SORT(self[4], self[7]);
    CPL_TYPE_SORT(self[12], self[14]); CPL_TYPE_SORT(self[10], self[14]);
    CPL_TYPE_SORT(self[6], self[7]); CPL_TYPE_SORT(self[10], self[12]);
    CPL_TYPE_SORT(self[6], self[10]);  CPL_TYPE_SORT(self[6], self[17]);
    CPL_TYPE_SORT(self[12], self[17]); CPL_TYPE_SORT(self[7], self[17]);
    CPL_TYPE_SORT(self[7], self[10]); CPL_TYPE_SORT(self[12], self[18]);
    CPL_TYPE_SORT(self[7], self[12]);  CPL_TYPE_SORT(self[10], self[18]);
    CPL_TYPE_SORT(self[12], self[20]); CPL_TYPE_SORT(self[10], self[20]);
    CPL_TYPE_SORT(self[10], self[12]);
}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Compute the arithmetic mean of an array
  @param    a     The array
  @param    n     The (positive) array size
  @return   The mean, S(n) = (1/n) sum(a_i) (i=1 -> n), or 0 on NULL input

  Compute the arithmetic mean of a dataset using the recurrence relation
     mean_(n) = mean(n-1) + (v[n] - mean(n-1))/(n+1)
     - this has a measurable impact on the output of
       cpl_polynomial_fit_{1,2}d_create()

 */
/*----------------------------------------------------------------------------*/
double ADDTYPE(cpl_tools_get_mean)(const CPL_TYPE * a, cpl_size nn)
{
    double   mean = 0.0;
    const size_t n = (size_t)nn;
    size_t i;


    cpl_ensure(a  != NULL,       CPL_ERROR_NULL_INPUT,       0.0);
    cpl_ensure(nn >  0,          CPL_ERROR_ILLEGAL_INPUT,    0.0);
    /* Ensure that the cast was OK */
    cpl_ensure((cpl_size)n == nn, CPL_ERROR_UNSUPPORTED_MODE, 0.0);

    for (i = 0; i < n; i++)
        mean += ((double)a[i] - mean) / (double)(i + 1);

    cpl_tools_add_flops(3 * n);

    return mean;
}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Compute the summed sample variance of an array
  @param    a     The array
  @param    n     The (non-negative) array size
  @param    pmean Iff non-NULL, *pmean is the mean (at no extra cost)
  @return   The summed sample variance, S(n) = sum((a_i-mean)^2) (i=1 -> n)
  @note Even with rounding errors the returned result is always non-negative

Math explanation for ticket DFS05126, written by Lander de Bilbao.


$$\sigma2 = \frac{\sum_{i=1}^N (x_i - \overline{x})2}{N-1}$$


We concentrate on how to compute

$$\sum_{i=1}^N (x_i - \overline{x})2$$

developed as follows

$$\sum_{i=1}^N (x_i - \overline{x})2 = \sum_{i=1}^N x_i2 - 2 \, \overline{x} \, \sum_{i=1}^N x_i + N \, \overline{x}2$$

as 

$$ \overline{x} = \frac{\sum_{i=1}^N x_i}{N}$$

then we have 

$$\sum_{i=1}^N (x_i - \overline{x})2 =  \sum_{i=1}^N x_i2 - N \, \overline{x}2$$


Now we look and see if it possible to, after doing this computation for N samples, add the contribution of a new sample

We call the new sample $x_{n+1}$, and the mean taken into account this new sample, $\overline{x}_{n+1}$. For clarity, we rewrite the previous equation with $\overline{x}$ renamed as $\overline{x}_n$

$$\sum_{i=1}^N (x_i - \overline{x}_n)2 =  \sum_{i=1}^N x_i2 - N \, \overline{x}_n2$$

We want to compute now

$$\sum_{i=1}^{N+1} (x_i - \overline{x}_{n+1})2$$

Developed

$$\sum_{i=1}^{N+1} (x_i - \overline{x}_{n+1})2 =  \sum_{i=1}^{N+1} x_i2 - (N+1) \, \overline{x}_{n+1}2$$

$$\sum_{i=1}^{N+1} (x_i - \overline{x}_{n+1})2 =  \sum_{i=1}^N x_i2 + x_{n+1}2 - (N+1) \, \overline{x}_{n+1}2$$

as

$$\overline{x}_{n+1} = \frac{\sum_{i=1}^N x_i + x_{n+1}}{N+1} $$

$$\overline{x}_{n+1} = \frac{\overline{x}_n \, N + x_{n+1}}{N+1} $$

then we have

$$\sum_{i=1}^{N+1} (x_i - \overline{x}_{n+1})2 =  \sum_{i=1}^N x_i2 + x_{n+1}2 - (N+1) \, ( \, \frac{\overline{x}_n \, N + x_{n+1}}{N+1} ) \, ^2$$

$$\sum_{i=1}^{N+1} (x_i - \overline{x}_{n+1})2 =  \sum_{i=1}^N x_i2 + x_{n+1}2 - \frac{( \, \overline{x}_n \, N + x_{n+1} ) \, ^2}{N+1}$$

$$\sum_{i=1}^{N+1} (x_i - \overline{x}_{n+1})2 =  \sum_{i=1}^N x_i2 + x_{n+1}2 - \frac{( \, N2 \, \overline{x}_n2 + 2 \, N \, \overline{x}_n \, x_{n+1} + x_{n+1}2 \, )}{N+1}$$

$$\sum_{i=1}^{N+1} (x_i - \overline{x}_{n+1})2 =  \sum_{i=1}^N x_i2 + x_{n+1}2 - \frac{N2}{N+1} \, \overline{x}_n2 - \frac{N}{N+1} \, 2 \, \overline{x}_n \, x_{n+1} - \frac1{N+1} \, x_{n+1}2$$

we add in both parts of the equation

$$\sum_{i=1}^{N+1} (x_i - \overline{x}_{n+1})2 + \frac1{N+1} \, \overline{x}_n2 =  \sum_{i=1}^N x_i2 + x_{n+1}2 - \frac{N2}{N+1} \, \overline{x}_n2 - \frac{N}{N+1} \, 2 \, \overline{x}_n \, x_{n+1} - \frac1{N+1} \, x_{n+1}2 + \frac1{N+1} \, \overline{x}_n2$$

we can now group some terms

$$\sum_{i=1}^{N+1} (x_i - \overline{x}_{n+1})2 + \frac1{N+1} \, \overline{x}_n2 =  \sum_{i=1}^N x_i2 - \frac{N2 - 1}{N+1} \, \overline{x}_n2 - \frac{N}{N+1} \, 2 \, \overline{x}_n \, x_{n+1} + \frac{N}{N+1} \, x_{n+1}2$$

$$\sum_{i=1}^{N+1} (x_i - \overline{x}_{n+1})2 + \frac1{N+1} \, \overline{x}_n2 =  \sum_{i=1}^N x_i2 - (N-1) \, \overline{x}_n2 - \frac{N}{N+1} \, 2 \, \overline{x}_n \, x_{n+1} + \frac{N}{N+1} \, x_{n+1}2$$

$$\sum_{i=1}^{N+1} (x_i - \overline{x}_{n+1})2 + \frac1{N+1} \, \overline{x}_n2 =  \sum_{i=1}^N x_i2 - N \, \overline{x}_n2 + \overline{x}_n2 - \frac{N}{N+1} \, 2 \, \overline{x}_n \, x_{n+1} + \frac{N}{N+1} \, x_{n+1}2$$

$$\sum_{i=1}^{N+1} (x_i - \overline{x}_{n+1})2 =  \sum_{i=1}^N x_i2 - N \, \overline{x}_n2 + \frac{N}{N+1} \, \overline{x}_n2 - \frac{N}{N+1} \, 2 \, \overline{x}_n \, x_{n+1} + \frac{N}{N+1} \, x_{n+1}2$$

$$\sum_{i=1}^{N+1} (x_i - \overline{x}_{n+1})2 = \sum_{i=1}^N (x_i - \overline{x}_n)2 + \frac{N}{N+1} \,( \, \overline{x}_n2 - 2 \, \overline{x}_n \, x_{n+1} + x_{n+1}2 )$$

$$\sum_{i=1}^{N+1} (x_i - \overline{x}_{n+1})2 = \sum_{i=1}^N (x_i - \overline{x}_n)2 + \frac{N}{N+1} \,( \, \overline{x}_n - x_{n+1} )2 $$

 */
/*----------------------------------------------------------------------------*/
double ADDTYPE(cpl_tools_get_variancesum)(const CPL_TYPE * a,
                                          cpl_size nn, double * pmean)
{
    double   varsum = 0.0;
    double   mean = 0.0;
    const size_t n = (size_t)nn;
    size_t i;

    cpl_ensure(a != NULL, CPL_ERROR_NULL_INPUT, 0.0);
    cpl_ensure(nn >= 0,   CPL_ERROR_ILLEGAL_INPUT, 0.0);
    /* Ensure that the cast was OK */
    cpl_ensure((cpl_size)n == nn, CPL_ERROR_UNSUPPORTED_MODE, 0.0);

    for (i=0; i < n; i++) {
        const double delta = (double)a[i] - mean;

        varsum += (double)i * delta * (delta / (double)(i + 1));
        mean   += delta / (double)(i + 1);
    }

    cpl_tools_add_flops( 1 + 6 * n ); /* Assume expression reuse */

    if (pmean != NULL) *pmean = mean;

    return varsum;
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Compute the sample variance of an array
  @param    a     The array
  @param    n     The (positive) array size
  @param    pmean Iff non-NULL, *pmean is the mean (at no extra cost)
  @return   The sample variance, S(n) = (1/n) sum((a_i-mean)^2) (i=1 -> n)
  @see cpl_tools_get_variancesum_double()
 */
/*----------------------------------------------------------------------------*/
double ADDTYPE(cpl_tools_get_variance)(const CPL_TYPE * a,
                                       cpl_size n, double * pmean)
{

    const double varsum
        = ADDTYPE(cpl_tools_get_variancesum)(a, n, pmean);

    cpl_ensure(n > 0, CPL_ERROR_ILLEGAL_INPUT, 0.0);

    return varsum / (double)n;
}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Compare two numerical values for qsort()
  @param  p1 Pointer to the 1st value
  @param  p2 Pointer to the 2nd value
  @return 1, 0, -1 depending on whether *p1 is smaller than, equal to or
          greater than *p2.
  @see qsort()
  @note Since the function is not exported its error checking is disabled

 */
/*----------------------------------------------------------------------------*/
static int ADDTYPE(compar_ascn)(const void * p1, const void * p2)
{

    const CPL_TYPE a1 = *(const CPL_TYPE *)p1;
    const CPL_TYPE a2 = *(const CPL_TYPE *)p2;

    return a1 < a2 ? -1 : (a1 > a2 ? 1 : 0);

}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Compare two numerical values for qsort()
  @param  p1 Pointer to the 1st value
  @param  p2 Pointer to the 2nd value
  @return -1, 0, 1 depending on whether *p1 is smaller than, equal to or
          greater than *p2.
  @see qsort()
  @note Since the function is not exported its error checking is disabled

 */
/*----------------------------------------------------------------------------*/
static int ADDTYPE(compar_desc)(const void * p1, const void * p2)
{

    const CPL_TYPE a1 = *(const CPL_TYPE *)p1;
    const CPL_TYPE a2 = *(const CPL_TYPE *)p2;

    return a1 < a2 ? 1 : (a1 > a2 ? -1 : 0);

}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Get the median of a numerical array
  @param  self The array to permute and request from
  @param  nn    The number of elements in the array
  @param  kk    The requested value position in the sorted array, zero for 1st
  @return The median of the partially sorted array.
  @see cpl_tools_get_median_int
  @note Since the function is not exported its error checking is disabled
  @note Benchmarking up to 10M randomized elements on a Xeon E5345 show no
        significant advantage over cpl_tools_get_kth. However, for an almost
        sorted array, Quickselect can be a lot faster.

    The Quickselect algorithm is derived from the Quicksort algorithm, see e.g.

    https://en.wikipedia.org/wiki/Quickselect

    Comments on the below implementation and its performance can be found at
  
    http://ndevilla.free.fr/median/median/

 */
/*----------------------------------------------------------------------------*/
CPL_TYPE ADDTYPE(cpl_tools_quickselection)(CPL_TYPE * self,
                                           cpl_size   nn,
                                           cpl_size   kk)
{

    const size_t n = (size_t)nn;
    const size_t k = (size_t)kk;
    size_t       low  = 0;
    size_t       high = n-1;

#ifdef CPL_TOOLS_STRICT_ERROR_CHECKING
    cpl_ensure(self != NULL, CPL_ERROR_NULL_INPUT,    (CPL_TYPE)0);
    cpl_ensure(nn  >  0,     CPL_ERROR_ILLEGAL_INPUT, (CPL_TYPE)0);
    cpl_ensure(nn  >  kk -1, CPL_ERROR_ILLEGAL_INPUT, (CPL_TYPE)0);
#endif
    /* Ensure that the cast was OK */
    cpl_ensure((cpl_size)n == nn, CPL_ERROR_UNSUPPORTED_MODE, 0.0);
    cpl_ensure((cpl_size)k == kk, CPL_ERROR_UNSUPPORTED_MODE, 0.0);

    /* Control flow has been changed to a single return at the end */

    for (;low + 1 < high;) {
        /* Find median of low, middle and high items; swap into position low */
        const size_t middle = low + (high - low) / 2;
        size_t       ll = low + 1;
        size_t       hh = high;

        CPL_TYPE_SORT(self[middle], self[high]);
        CPL_TYPE_SORT(self[low],    self[high]);
        CPL_TYPE_SORT(self[middle], self[low]);

        /* Swap low item (now in position middle) into position (low+1) */
        CPL_TYPE_SWAP(self[middle], self[low+1]);

        /* Nibble from each end towards middle, swapping items when stuck */
        for (;;) {
            do ll++; while (self[low] > self[ll]);
            do hh--; while (self[hh]  > self[low]);

            if (hh < ll)
                break;

            CPL_TYPE_SWAP(self[ll], self[hh]);
        }

        /* Swap middle item (in position low) back into correct position */
        CPL_TYPE_SWAP(self[low], self[hh]);

        /* Re-set active partition */
        if (hh <= k)
            low = ll;
        if (hh >= k)
            high = hh - 1;
    }

    if (high == low + 1) {  /* Two elements only */
        CPL_TYPE_SORT(self[low], self[high]);
    }

    return self[k];
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Get the kth smallest value in a numerical array
  @param  self The array to permute and request from
  @param  n    The number of elements in the array
  @param  k    The requested value position in the sorted array, zero for 1st
  @return The kth smallest value in the partially sorted array.
  @note Since the function is not exported its error checking is disabled
  @see cpl_tools_get_median_int

  After a successful call, self is permuted so elements less than the kth have
  lower indices, while elements greater than the kth have higher indices.

  Reference:

  Author: Wirth, Niklaus 
  Title: Algorithms + data structures = programs 
  Publisher: Englewood Cliffs: Prentice-Hall, 1976 
  Physical description: 366 p. 
  Series: Prentice-Hall Series in Automatic Computation 

  See also: http://ndevilla.free.fr/median/median/

 */
/*----------------------------------------------------------------------------*/
CPL_TYPE ADDTYPE(cpl_tools_get_kth)(CPL_TYPE * self,
                                    cpl_size   n,
                                    cpl_size   k)
{
    register cpl_size l = 0;
    register cpl_size m = n - 1;
    register cpl_size i = l;
    register cpl_size j = m;

#ifdef CPL_TOOLS_STRICT_ERROR_CHECKING
    cpl_ensure(self != NULL, CPL_ERROR_NULL_INPUT,          (CPL_TYPE)0);
    cpl_ensure(k >= 0,       CPL_ERROR_ILLEGAL_INPUT,       (CPL_TYPE)0);
    cpl_ensure(k <  n,       CPL_ERROR_ACCESS_OUT_OF_RANGE, (CPL_TYPE)0);
#endif

    while (l < m) {
        register const CPL_TYPE x = self[k];

        do {
            while (self[i] < x) i++;
            while (x < self[j]) j--;
            if (i <= j) {
                CPL_TYPE_SWAP(self[i], self[j]);
                i++; j--;
            }
        } while (i <= j);

        /* assert( j < i ); */

        /* The original implementation has two index comparisons and
           two, three or four index assignments. This has been reduced
           to one or two index comparisons and two index assignments.
        */

        if (k <= j) {
            /* assert( k < i ); */
            m = j;
            i = l;
        } else {
            if (k < i) {
                m = j;
            } else {
                j = m;
            }
            l = i;
        }
    }
    return self[k];
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Sort a numerical array into ascending order using qsort
  @param    self The array to sort
  @param    n    The number of array elements
  @return   the #_cpl_error_code_ or CPL_ERROR_NONE
  @note Since the function is not exported its NULL-pointer checking is disabled

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_ILLEGAL_INPUT
 */
/*----------------------------------------------------------------------------*/
void ADDTYPE(cpl_tools_sort_ascn)(CPL_TYPE * self, int n)
{

    qsort(self, n, sizeof(*self), ADDTYPE(compar_ascn));

}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Sort a numerical array into ascending order using qsort
  @param    self The array to sort
  @param    n    The number of array elements
  @return   the #_cpl_error_code_ or CPL_ERROR_NONE
  @note Since the function is not exported its NULL-pointer checking is disabled

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_ILLEGAL_INPUT
 */
/*----------------------------------------------------------------------------*/
void ADDTYPE(cpl_tools_sort_desc)(CPL_TYPE * self, int n)
{
    qsort(self, (size_t)n, sizeof(*self), ADDTYPE(compar_desc));
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Sort a numerical array
  @param    self     The array to sort
  @param    n        The number of array elements
  @return   the #_cpl_error_code_ or CPL_ERROR_NONE
  @note Since the function is not exported its NULL-pointer checking is disabled

  On a nearly sorted array, this function is a LOT slower than qsort()
  - but on "normal" rancom data, it can be several times faster.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_ILLEGAL_INPUT
 */
/*----------------------------------------------------------------------------*/
cpl_error_code
ADDTYPE(cpl_tools_sort)(CPL_TYPE * self, cpl_size nn)
{
    const size_t n = (size_t)nn;
    size_t    i, ir, j, k, l;
    size_t    i_stack[CPL_PIX_STACK_SIZE];
    size_t    j_stack;
    CPL_TYPE  a;

    /* Ensure that the cast was OK */
    cpl_ensure_code((cpl_size)n == nn, CPL_ERROR_UNSUPPORTED_MODE);

    ir = n;
    l = 1;
    j_stack = 0;
    for (;;) {
        if (ir-l < 7) {
            for (j=l+1; j<=ir; j++) {
                a = self[j-1];
                for (i=j-1; i>=1; i--) {
                    if (self[i-1] <= a) break;
                    self[i] = self[i-1];
                }
                self[i] = a;
            }
            if (j_stack == 0) break;
            ir = i_stack[j_stack-- -1];
            l  = i_stack[j_stack-- -1];
        } else {
            k = (l+ir) >> 1;
            CPL_TYPE_SWAP(self[k-1], self[l]);
            CPL_TYPE_SORT(self[l], self[ir-1]);
            CPL_TYPE_SORT(self[l-1], self[ir-1]);
            CPL_TYPE_SORT(self[l], self[l-1]);
            i = l+1;
            j = ir;
            a = self[l-1];
            for (;;) {
                do i++; while (self[i-1] < a);
                do j--; while (self[j-1] > a);
                if (j < i) break;
                CPL_TYPE_SWAP(self[i-1], self[j-1]);
            }
            self[l-1] = self[j-1];
            self[j-1] = a;
            j_stack += 2;
            cpl_ensure_code(j_stack <= CPL_PIX_STACK_SIZE,
                            CPL_ERROR_ILLEGAL_INPUT);

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
    return CPL_ERROR_NONE;
}

#undef CPL_TYPE_SWAP
#undef CPL_TYPE_SORT

/* End of CPL_TYPE_IS_NUM */

#endif


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Sort a numerical array
  @param    self   The numerical array to sort
  @param    n      The number of array elemenst
  @param    reverse      flag indicating whether to sort ascending (zero) or
                         descending (non-zero)
  @param    stable       flag to indicate whether to guarantee stability at
                         the cost of execution time (if cpl_tools_sort_int is
                         O(n log n), this function would still be O(n log n))
  @param    sort_pattern resulting sort pattern
  @return   the #_cpl_error_code_ or CPL_ERROR_NONE

  The heap sort algorithm used here
  - has bad cache performance, but
  - is worst case O(n log n)
  - is stable (meaning that the order of equal elements is conserved 
               indepent on the value of the reverse flag)
  - is in place

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT
  - CPL_ERROR_ILLEGAL_INPUT
 */
/*----------------------------------------------------------------------------*/
cpl_error_code
ADDTYPE(cpl_tools_sort_stable_pattern)(CPL_TYPE const * self,
                                       cpl_size n,
                                       int reverse,
                                       int stable,
                                       cpl_size *sort_pattern)
{
    cpl_size i;  
    
    /* Check entries */
    cpl_ensure_code(self, CPL_ERROR_NULL_INPUT);
    
    cpl_ensure_code(sort_pattern, CPL_ERROR_NULL_INPUT);

    if (n == 0) return CPL_ERROR_NONE;

    for (i = 0; i < n; i++) {
        sort_pattern[i] = i;
    }

    /* 
     * Heap sort
     */
    for (i = n / 2 - 1; i >= 0; i--) {
        int      done = 0;
        cpl_size root = i;
        cpl_size bottom = n - 1;
        
        while ((root*2 + 1 <= bottom) && (!done)) {
            cpl_size child = root*2 + 1;

            if (child+1 <= bottom) {
                if ((!reverse && CPL_TOOLS_SORT_LT(
                         self[sort_pattern[child]],
                         self[sort_pattern[child + 1]]))
                    ||
                    (reverse && CPL_TOOLS_SORT_LT(
                        self[sort_pattern[child + 1]],
                        self[sort_pattern[child]]))
                    ) {
                    child += 1;
                }
            }

            if ((!reverse && CPL_TOOLS_SORT_LT(
                    self[sort_pattern[root]],
                    self[sort_pattern[child]])) 
                    ||
                    (reverse && CPL_TOOLS_SORT_LT(
                    self[sort_pattern[child]],
                    self[sort_pattern[root]]))
                    ) {
                CPL_INT_SWAP(sort_pattern[root], sort_pattern[child]);
                root = child;
            }
            else {
                done = 1;
            }
        }
    }
    
    for (i = n - 1; i >= 1; i--) {
        int      done = 0;
        cpl_size root = 0;
        cpl_size bottom = i - 1;
        CPL_INT_SWAP(sort_pattern[0], sort_pattern[i]);
        
        while ((root*2 + 1 <= bottom) && (!done)) {
            cpl_size child = root*2 + 1;

            if (child+1 <= bottom) {
                if ((!reverse && CPL_TOOLS_SORT_LT(
                         self[sort_pattern[child]],
                         self[sort_pattern[child + 1]]))
                    ||
                    (reverse && CPL_TOOLS_SORT_LT(
                        self[sort_pattern[child + 1]],
                        self[sort_pattern[child]]))
                    ) {
                    child += 1;
                }
            }
            if ((!reverse && CPL_TOOLS_SORT_LT(
                    self[sort_pattern[root]],
                    self[sort_pattern[child]])) 
                    ||
                (reverse && CPL_TOOLS_SORT_LT(
                    self[sort_pattern[child]],
                    self[sort_pattern[root]]))
                ) {
                CPL_INT_SWAP(sort_pattern[root], sort_pattern[child]);
                root = child;
            }
            else {
                done = 1;
            }
        }
    }

    /* 
     * Enforce stability
     */
    if (stable) {
        for (i = 0; i < n; i++) {
            cpl_size j;
            j = i + 1;
            while(j < n &&
                  !CPL_TOOLS_SORT_LT(self[sort_pattern[i]],
                                     self[sort_pattern[j]]) &&
                  !CPL_TOOLS_SORT_LT(self[sort_pattern[j]],
                                     self[sort_pattern[i]])) {
                j++;
            }
            if (j - i > 1) {
                cpl_tools_sort_cplsize(sort_pattern + i, j - i);
            }
            i = j - 1; 
        }
    }     

    return CPL_ERROR_NONE;
}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief   Apply a permutation to one or two arrays
  @param   self   The permutation array, this is destroyed by the call
  @param   n      The (positive) number of elements to permute
  @param   awrite The array to hold the permuted A-array
  @param   aread  The array to hold the input A-array, may equal awrite
  @param   bwrite The array to hold the permuted B-array, or NULL
  @param   bread  The array to hold the input B-array, may equal awrite or NULL
  @return  CPL_ERROR_NONE on success or the relevant #_cpl_error_code_ on error
  @note self is destroyed by the call. bread and bwrite must both be either
        NULL or non-NULL

  In-place permutation is done in O(n) time with O(1) extra storage
  using the fact that any permutation can be decomposed into a sequence
  of cyclic permutations.

  Uni-cycles are processed as well, to support in-place permuting of
  only one of the two arrays. This means that the below code also
  works for out-of-place permuting.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT         self, aread or awrite is NULL
  - CPL_ERROR_ILLEGAL_INPUT      size is not positive
  - CPL_ERROR_INCOMPATIBLE_INPUT only one of bread/bwrite is NULL
 */
/*----------------------------------------------------------------------------*/
cpl_error_code
ADDTYPE(cpl_tools_permute)(cpl_size * self, cpl_size n,
                           CPL_TYPE * awrite, CPL_TYPE const * aread,
                           CPL_TYPE * bwrite, CPL_TYPE const * bread)
{

    cpl_size ido = 0; /* First element in cycle to process */
    const cpl_boolean dob = bwrite != NULL;

    cpl_ensure_code(self   != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(n      > 0,     CPL_ERROR_ILLEGAL_INPUT);
    cpl_ensure_code(awrite != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(aread  != NULL, CPL_ERROR_NULL_INPUT);

    if (dob) {
        cpl_ensure_code(bread != NULL, CPL_ERROR_INCOMPATIBLE_INPUT);
    } else {
        cpl_ensure_code(bread == NULL, CPL_ERROR_INCOMPATIBLE_INPUT);
    }

    do {
        /* Save last pair in cycle */
        CPL_TYPE const aread0 = aread[ido];
        CPL_TYPE const bread0 = dob ? bread[ido] : 0;
        cpl_size ifrom = ido;

        while (self[ifrom] != ido) { /* Test here to avoid uni-cycles */
            /* Copy the pair first to support cross-wrapped bivectors */
            CPL_TYPE const areadj = aread[self[ifrom]];
            CPL_TYPE const breadj = dob ? bread[self[ifrom]] : 0;
            const cpl_size j = ifrom;

            ifrom = self[ifrom]; /* Point to next in cycle */

            assert( ifrom != -1 ); /* Can fail only on non-perm array */

            awrite[j] = areadj;
            if (dob) bwrite[j] = breadj;
            self  [j] = -1; /* This position now has the right value */

        }
        /* Cycle is finished, copy and flag last pair */
        awrite[ifrom] = aread0;
        if (dob) bwrite[ifrom] = bread0;
        self[ifrom] = -1;

        /* Find start of next cycle */
        while (++ido < n && self[ido] < 0);
    } while (ido < n);

    return CPL_ERROR_NONE;
}
