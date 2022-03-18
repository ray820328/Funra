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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

/*-----------------------------------------------------------------------------
                                   Includes
 -----------------------------------------------------------------------------*/

#include "cpl_polynomial_impl.h"

#include "cpl_tools.h"
#include "cpl_error_impl.h"
#include "cpl_matrix_impl.h"
#include "cpl_vector_impl.h"
#include "cpl_memory.h"
#include "cpl_math_const.h"

#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <math.h>
#include <assert.h>

/*----------------------------------------------------------------------------*/
/**
 * @defgroup cpl_polynomial Polynomials
 *
 * This module provides functions to handle uni- and multivariate polynomials.
 *
 *
 * Comparing the two polynomials
 * @verbatim
 * P1(x) = p0 + p1.x + p4.x^2
 * @endverbatim
   and
 * @verbatim
 * P2(x,y) = p0 + p1.x + p2.y + p3.x.y + p4.x^2 + p5.y^2
 * @endverbatim
 * P1(x) may evaluate to more accurate results than P2(x,0),
 * especially around the roots.
 *
 * Note that a polynomial like P3(z) = p0 + p1.z + p2.z^2 + p3.z^3, z=x^4
 * is preferable to p4(x) = p0 + p1.x^4 + p2.x^8 + p3.x^12.
 *
 * Polynomials are evaluated using Horner's method. For multivariate
 * polynomials the evaluation is performed one dimension at a time, starting
 * with the lowest dimension and proceeding upwards through the higher
 * dimensions.
 *
 * Access to a coefficient of an N-dimensional polynomial has complexity O(N).
 *
 */
/*----------------------------------------------------------------------------*/
/**@{*/

/*-----------------------------------------------------------------------------
                                   Type definition
 -----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/*
                  Polynomial object.

  The zero-polynomial (i.e. a zero-degree polynomial with a zero-valued
  coefficient) is regardless of its dimension stored internally as a
  NULL pointer.

  A non-zero uni-variate polynomial is stored as an array where the i'th
  element is the real-valued coefficient of the variable to the i'th power
  together with a counter of the number of its elements. An N-degree
  polynomial thus has an array with N+1 elements. The first element is the
  constant term, the last element is the most significant, it is non-zero.

  A bi-variate polynomial is also stored as an array with one element for each
  of its second dimension degrees together with a counter of the number of its
  elements.
  Each of these elements is either a pointer to a uni-variate polynomial in
  the lower dimension or a NULL-pointer if that uni-variate polynomial is the
  zero-polynomial.

  Similarly, a higher-dimension polynomial is stored as an array with one
  element for each of its own dimension degrees and a counter of the number
  of its elements.
  Each of these elements is either a pointer to a multi-variate polynomial in
  the lower dimensions, or a NULL-pointer if that lower-dimension polynomial
  is the zero-polynomial.

  A multi-variate polynomial in N-dimensions is thus stored as a tree of depth
  N-1, where each leaf is a uni-variate polynomial, and where each non-leaf
  node has as many child nodes as there are non-zero coefficients in that
  dimension. All non-NULL leaf-nodes are at the same depth (of N-1).

  An unbalanced tree is allowed to avoid that higher dimension polynomials
  store an excessive number of zero-valued coefficients.

  This storage scheme allows for the usage of the Horner rule in each dimension.

  Storing the following polynomial:
  p(x,y) = p0 + p1.x + p2.y + p3.x.y + p4.x^2 + p5.x.y^3
  would internally take:
  dim = 2 (x and y),
  a pointer to a 1D pointer-array of length 3 (Y-degrees, 0..2), where
  the first pointer is to a struct w. nc = 3 (Y-degree = 0, X-degress 0..2) and
  the double-array {p0, p1, p4},
  the second pointer is to a struct w. nc = 2 (Y-degree = 1, X-degress 0..1) and
  the double-array {p2, p3}, and
  the third pointer is NULL, indicating a zero-coefficient to Y-degree = 2 and
  the fourth pointer is to a struct w. nc = 1 (Y-degree = 3, X-degree 1) and
  the double-array {0, p5}.

  The root object (2 dimensions, fourth degree) would thus hold this tree:
  Y:
  0 -> p0, p1, p4
  1 -> p2, p3
  2 -> NULL
  3 -> 0, p5

  Additional notes:
  1) Each node in the tree has no knowledge of any higher dimensions above it,
     so the leaf node in a multi-dimensional tree is indistinguishable from a
     univariate polynomial.

  2) Each non-NULL non-leaf node in the tree has no knowledge of how many
     dimensions are stored below it, i.e. it has no knowledge of its own
     dimensionality.

  3) Thus, when iterating through a polynomial, the iterator state needs to
     include the current dimension, which is zero for the lowest and one
     higher for each dimension above.

  4) To allow for a low-complexity growth of the degree of any polynomial,
     each node has two counters, one for the number of allocated elements,
     and one for the actual count, which cannot exceed the size counter.
     If the available space for coefficients is exhausted, double as much
     space is allocated for the new coefficients.

  5) To reduce the number of memory allocations, the allocation of memory for
     a new node includes memory for the requested number of coefficients
     (child nodes) at that node (and at least a minimum default number,
     CPL_POLYNOMIAL_DEFAULT_COEFFS). If the number of coefficients (i.e. the
     degree) later grows, a new memory allocation for the coefficients is
     needed and the initial memory set aside for the cofficients remains
     allocated but unused. The rationale behind this scheme is that the
     coefficients in a polynomial typically are set either from the highest
     towards to lowest or vice versa, so typically just one allocation is
     needed per node.

  6) To allow for a simple implementation both leaf- and non-leaf nodes use
     the same object for storage, the array in this object uses a union of
     a pointer (for higher dimensions) and a double (for the lowest dimension).

  7) Reading from a given node object is only defined using the pointer member
     for higher dimensions and the double member for the lowest dimension.

  8) When during traversal of an N-dimensional tree it is necessary to know the
     powers in the dimensions above a given coefficient, a integer array with
     length N is updated at each dimension with its power.

  9) A new polynomial can be created internally from an old one, with a change
     of dimension supported. Changing the dimension means overwriting a union
     member with a value of the other type. This works as long as the nodes of
     the new tree keeps all its (non-NULL) leaf-nodes at the same level.

 10) The object that holds the root of the tree thus knows its dimensionality.
     To support O(1) access to its degree, the object also stores the degree
     of the polynomial, this redundantly stored degree is kept up to date with
     every change of the polynomial.

 11) A valid node has a non-zero coefficient count. If a node's coefficient
     count collapses to zero (indicating that the corresponding polynomial is
     the zero-polynomial), the knowledge of this collapse has to be passed
     up to the higher dimension, which will deallocate the node and set it to
     NULL.

 12) Certain operations on a polynomial may collapse a (lower dimension) node
     to the zero-polynomial, such operations include: setting a coefficient
     (to zero), adding or subtracting one polynomial from another. To ensure
     the absence of zero-polynomials in a returned polynomial, the resulting
     polynomial is passed to a pruning function that scans the whole tree and
     removes nodes that are zero-polynomials.

 13) For efficiency reasons a function may within its own scope set the
     coefficient count of its leaf-nodes to zero without the corresponding
     deallocation. While such a non-standard object may not be passed on to
     other functions (nor returned), it may be re-populated (avoiding the
     need for reallocation of arrays). Unless there is a priori knowledge
     that this repopulation will convert all zero-polynomials to actual
     polynomials, the object must be pruned (using the relevant function)
     before being returned to the caller.

 */
/*----------------------------------------------------------------------------*/


/*-----------------------------------------------------------------------------
                             Macro definitions
 -----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/**
  @internal
  @def CPL_POLYNOMIAL_DEFAULT_COEFFS
  @brief  The default allocation for a 1D-polynomial
  @hideinitializer

 */
/*----------------------------------------------------------------------------*/

#ifndef CPL_POLYNOMIAL_DEFAULT_COEFFS
#define CPL_POLYNOMIAL_DEFAULT_COEFFS 5
#endif

/*----------------------------------------------------------------------------*/
/**
  @internal
  @def CPL_POLYNOMIAL_SOLVE_MAXITE
  @brief  Maximum number of Newton-Raphson iterations
  @hideinitializer

 */
/*----------------------------------------------------------------------------*/

#ifndef CPL_POLYNOMIAL_SOLVE_MAXITE
#define CPL_POLYNOMIAL_SOLVE_MAXITE 100
#endif

/*----------------------------------------------------------------------------*/
/**
  @internal
  @def CPL_POLYNOMIAL_COEFF_IS_MALLOC
  @brief  Boolean expression, true iff the coefficients are malloc()'ed
  @param  SELF  The 1D-polynomial
  @hideinitializer

 */
/*----------------------------------------------------------------------------*/
#define CPL_POLYNOMIAL_COEFF_IS_MALLOC(SELF) \
    ((SELF)->coef != (void*)((SELF) + 1))

/*-----------------------------------------------------------------------------
                                New types
 -----------------------------------------------------------------------------*/

typedef struct cpl_polynomial_1d_ {
    union {
        double val;  /* Final, lowest dimension, coefficient */
        struct cpl_polynomial_1d_ * next; /* Next, lower dimension */
    } * coef;
    cpl_size nc; /* The non-zero number of coefficients, one plus the degree */
    cpl_size nz; /* The allocated number of coefficients, at least nc */
} cpl_polynomial_1d;

typedef struct _cpl_polynomial_ {
    cpl_polynomial_1d * tree; /* NULL for the zero-polynomial */
    cpl_size dim;             /* 1 for univariate */
    cpl_size degree;          /* Kept updated at every coefficient change */
} _cpl_polynomial_;

/*-----------------------------------------------------------------------------
                        Private function prototypes
 -----------------------------------------------------------------------------*/

inline static cpl_polynomial_1d * cpl_polynomial_new_1d(cpl_size)
    CPL_ATTR_ALLOC;

inline static void cpl_polynomial_delete_(cpl_polynomial_1d *, cpl_size);

static void cpl_polynomial_reset_(cpl_polynomial_1d *, cpl_size);

inline static void cpl_polynomial_grow_1d(cpl_polynomial_1d *, cpl_size);

inline static void cpl_polynomial_grow_1d_reset(cpl_polynomial_1d *,
                                                cpl_size,
                                                cpl_size);

inline static void cpl_polynomial_set_coeff_(cpl_polynomial_1d *,
                                             const cpl_size *,
                                             cpl_size,
                                             double)
    CPL_ATTR_NONNULL;

inline static cpl_boolean cpl_polynomial_prune_(cpl_polynomial_1d *,
                                                cpl_size,
                                                cpl_size);

inline static void cpl_polynomial_transpose_(cpl_polynomial_1d *,
                                             cpl_size *,
                                             cpl_size,
                                             const cpl_size *,
                                             cpl_size,
                                             const cpl_polynomial_1d *,
                                             cpl_size)
    CPL_ATTR_NONNULL;

static void cpl_polynomial_copy_(cpl_polynomial_1d *,
                                 cpl_size,
                                 const cpl_polynomial_1d *,
                                 cpl_size)
    CPL_ATTR_NONNULL;

static cpl_size cpl_polynomial_find_degree_(const cpl_polynomial_1d *,
                                            cpl_size)
    CPL_ATTR_NONNULL;

static int cpl_polynomial_compare_(const cpl_polynomial_1d *,
                                   const cpl_polynomial_1d *,
                                   cpl_size,
                                   double);

static cpl_boolean cpl_polynomial_add_(cpl_polynomial_1d       *,
                                       const cpl_polynomial_1d *,
                                       const cpl_polynomial_1d *,
                                       cpl_size)
#ifdef CPL_HAVE_ATTR_NONNULL
    __attribute__((nonnull(1)))
#endif
    ;

static cpl_boolean cpl_polynomial_scale_add_(cpl_polynomial_1d       *,
                                             const cpl_polynomial_1d *,
                                             cpl_size, double,
                                             const cpl_size          *)
#ifdef CPL_HAVE_ATTR_NONNULL
    __attribute__((nonnull(1,5)))
#endif
    ;

static cpl_boolean cpl_polynomial_subtract_(cpl_polynomial_1d       *,
                                            const cpl_polynomial_1d *,
                                            const cpl_polynomial_1d *,
                                            cpl_size)
#ifdef CPL_HAVE_ATTR_NONNULL
    __attribute__((nonnull(1)))
#endif
    ;

static void cpl_polynomial_multiply_(cpl_polynomial_1d       *,
                                     const cpl_polynomial_1d *,
                                     const cpl_polynomial_1d *,
                                     cpl_size *,
                                     cpl_size,
                                     const cpl_size *,
                                     cpl_size)
    CPL_ATTR_NONNULL;

static cpl_error_code cpl_polynomial_dump_(const cpl_polynomial_1d *,
                                           cpl_size *,
                                           cpl_size,
                                           const cpl_size *,
                                           cpl_size,
                                           cpl_size *,
                                           FILE *)
    CPL_ATTR_NONNULL;

static void cpl_polynomial_multiply_scalar_(cpl_polynomial_1d *,
                                            const cpl_polynomial_1d *,
                                            cpl_size,
                                            double)
    CPL_ATTR_NONNULL;

static void cpl_polynomial_multiply_scalar_self(cpl_polynomial_1d *,
                                                cpl_size,
                                                double);

static cpl_error_code cpl_polynomial_dump_coeff(const cpl_size *,
                                                cpl_size        ,
                                                FILE           *,
                                                double          )

#ifdef CPL_HAVE_ATTR_NONNULL
    __attribute__((nonnull(3)));
#endif

inline static double cpl_polynomial_get_coeff_(const cpl_polynomial_1d *,
                                               const cpl_size *,
                                               cpl_size)
#ifdef CPL_HAVE_ATTR_NONNULL
    __attribute__((nonnull(2)));
#endif

inline static cpl_long_double cpl_polynomial_eval_(const cpl_polynomial_1d *,
                                                   cpl_size,
                                                   const double *)
#ifdef CPL_HAVE_ATTR_NONNULL
    __attribute__((nonnull(3)));
#endif

static cpl_error_code cpl_polynomial_solve_1d__(const cpl_polynomial_1d *,
                                                double, double *,
                                                cpl_size,
                                                cpl_boolean)
    CPL_ATTR_NONNULL;

inline static cpl_long_double cpl_polynomial_eval_1d_(const cpl_polynomial_1d *,
                                                      double, double *)
#ifdef CPL_HAVE_ATTR_NONNULL
    __attribute__((nonnull(1)))
#endif
    ;

static void cpl_polynomial_eval_monomial_(const cpl_polynomial_1d *,
                                          cpl_size                *,
                                          cpl_size                 ,
                                          const cpl_size          *,
                                          cpl_size                 ,
                                          const double            *,
                                          cpl_long_double         *)
#ifdef CPL_HAVE_ATTR_NONNULL
    __attribute__((nonnull(2,4,6,7)))
#endif
    ;

inline static
cpl_long_double cpl_polynomial_nested_horner(const cpl_polynomial_1d *,
                                             double, double,
                                             cpl_long_double *)
    CPL_ATTR_NONNULL;

static cpl_boolean cpl_polynomial_derivative_(cpl_polynomial_1d *,
                                              cpl_size,
                                              cpl_size)
    CPL_ATTR_NONNULL;

static cpl_boolean cpl_polynomial_extract_(cpl_polynomial_1d *,
                                           const cpl_polynomial_1d *,
                                           cpl_size,
                                           cpl_size,
                                           double);

inline static void cpl_polynomial_shift_1d_(cpl_polynomial_1d *,
                                            cpl_size,
                                            double)
    CPL_ATTR_NONNULL;

inline static cpl_boolean cpl_polynomial_delete_coeff_(cpl_polynomial_1d *,
                                                       const cpl_size *,
                                                       cpl_size)
    CPL_ATTR_NONNULL;

static void cpl_polynomial_delete_coeff(cpl_polynomial *,
                                        const cpl_size *,
                                        cpl_size)
    CPL_ATTR_NONNULL;

static
void cpl_matrix_fill_normal_vandermonde(cpl_matrix *, cpl_matrix *,
                                        const cpl_vector *, cpl_boolean,
                                        cpl_size, const cpl_vector *)
    CPL_ATTR_NONNULL;

static
cpl_error_code cpl_polynomial_fit_2d(cpl_polynomial *, const cpl_bivector *,
                                     const cpl_vector *, cpl_boolean,
                                     const cpl_size *,
                                     const cpl_size *)
#ifdef CPL_HAVE_ATTR_NONNULL
    __attribute__((nonnull(5)))
#endif
    ;

static int cpl_vector_is_eqdist(const cpl_vector *);

/*-----------------------------------------------------------------------------
                              Function codes
 -----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/**
  @brief   Create a new cpl_polynomial
  @param   dim    The positive polynomial dimension (number of variables)
  @return  A newly allocated cpl_polynomial or NULL on error
  @see cpl_polynomial_delete()
  @note The returned object must be deallocated after use

  A newly created polynomial has degree 0 and evaluates as 0.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_ILLEGAL_INPUT if dim is negative or zero
 */
/*----------------------------------------------------------------------------*/
cpl_polynomial * cpl_polynomial_new(cpl_size dim)
{
    cpl_polynomial * self;

    cpl_ensure(dim > 0, CPL_ERROR_ILLEGAL_INPUT, NULL);

    /* Allocate struct */
    self = (cpl_polynomial *)cpl_malloc(sizeof(*self));
    self->dim = dim;
    self->tree = NULL;
    self->degree = 0;

    return self;
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Reset (delete all coefficients of) a cpl_polynomial
  @param    self   Polynomial to modify
  @return   void
  @see cpl_polynomial_free()
  @note The dimension is unchanged, the degree is zero

 */
/*----------------------------------------------------------------------------*/
void cpl_polynomial_empty(cpl_polynomial * self)
{
    if (self != NULL) {
        assert(self->dim > 0);
        cpl_polynomial_delete_(self->tree, self->dim - 1);
        self->tree = NULL;
        self->degree = 0;
    }
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Delete a cpl_polynomial
  @param    self    Polynomial to deallocate
  @return   void
  @see cpl_polynomial_new()
  @note If @em self is @c NULL, nothing is done and no error is set.

  The function deallocates the memory used by the polynomial @em self.

 */
/*----------------------------------------------------------------------------*/
void cpl_polynomial_delete(cpl_polynomial * self)
{
    if (self != NULL) {
        cpl_polynomial_empty(self);
        cpl_free((void*)self);
    }
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Dump a polynomial as ASCII to a stream, fail on zero-polynomial(s)
  @param    self    The polynomial to process
  @param    stream  Output stream eq. @c stdout or @c stderr
  @return   CPL_ERROR_NONE or the relevant #_cpl_error_code_
  @note Since this function is mostly for testing during development, it checks
        for illegal sub-zero-polynomials and sets a CPL error if one is found

  Each coefficient is preceded by its integer power(s) and
  written on a single line.
  If the polynomial has non-zero coefficients, only those are printed,
  otherwise the (zero-valued) constant term is printed.

  For an N-dimensional polynomial each line thus consists of N power(s) and
  their coefficient.

  Comment lines start with the hash character.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_FILE_IO if the write operation fails
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_polynomial_dump(const cpl_polynomial * self,
                                   FILE                 * stream)
{
    cpl_size* pows;
    cpl_error_code code;
    int retval;
    cpl_size icoeff = 0;

    cpl_ensure_code(self   != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(stream != NULL, CPL_ERROR_NULL_INPUT);

    retval = fprintf(stream, "#----- %" CPL_SIZE_FORMAT " dimensional "
                     "polynomial of degree %" CPL_SIZE_FORMAT " -----\n",
                     self->dim, self->degree);
    cpl_ensure_code(retval > 0, CPL_ERROR_FILE_IO);

    for (cpl_size i = 0; i < self->dim; i++) {
        retval = fprintf(stream, "%d.dim.power  ", (int)i+1);
        cpl_ensure_code(retval > 0, CPL_ERROR_FILE_IO);
    }
    retval = fprintf(stream, "coefficient\n");
    cpl_ensure_code(retval > 0, CPL_ERROR_FILE_IO);

    if (self->tree == NULL) {
        code = cpl_polynomial_dump_coeff(NULL, self->dim, stream, 0.0);
    } else {
        pows = cpl_malloc((size_t)self->dim * sizeof(*pows));

        code = cpl_polynomial_dump_(self->tree,
                                    pows + self->dim - 1, self->dim - 1,
                                    pows, self->dim, &icoeff, stream);
        cpl_free(pows);
    }

    if (code) return cpl_error_set_where_();

    retval = fprintf(stream, "#----- %" CPL_SIZE_FORMAT " coefficient(s) "
                     "-----\n", icoeff);
    cpl_ensure_code(retval > 0, CPL_ERROR_FILE_IO);

    retval = fprintf(stream, "#------------------------------------\n");
    cpl_ensure_code(retval > 0, CPL_ERROR_FILE_IO);

    return CPL_ERROR_NONE;
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Duplicate a polynomial
  @param    self   The non-NULL polynomial to process
  @return   A newly allocated cpl_polynomial or NULL on error
  @see cpl_polynomial_new()

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
*/
/*----------------------------------------------------------------------------*/
cpl_polynomial * cpl_polynomial_duplicate(const cpl_polynomial * self)
{
    cpl_polynomial * other;

    cpl_ensure(self != NULL, CPL_ERROR_NULL_INPUT, NULL);

    other = cpl_polynomial_new(self->dim);

    if (self->tree == NULL) return other;

    other->tree = cpl_polynomial_new_1d(self->tree->nc);

    cpl_polynomial_copy_(other->tree, self->dim - 1,
                         self->tree,  self->dim - 1);
    other->degree = self->degree;

    return other;
}

/*----------------------------------------------------------------------------*/
/**
  @brief  Copy the contents of one polynomial into another one
  @param  self  Pre-allocated output polynomial
  @param  other Input polynomial
  @return CPL_ERROR_NONE on success or else the relevant #_cpl_error_code_

  self and other must point to different polynomials.

  If self already contains coefficients, then they are overwritten.

  This is the only function that can modify the dimension of a polynomial.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_INCOMPATIBLE_INPUT if in and out point to the same polynomial
*/
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_polynomial_copy(cpl_polynomial * self,
                                   const cpl_polynomial * other)
{
    cpl_ensure_code(self  != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(other != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(other != self, CPL_ERROR_INCOMPATIBLE_INPUT);

    if (other->tree == NULL) {
        cpl_polynomial_empty(self); /* Deallocate any pre-existing buffers */
    } else {
        if (self->tree == NULL) 
            self->tree = cpl_polynomial_new_1d(other->tree->nc);
        cpl_polynomial_copy_(self->tree,  self->dim  - 1,
                             other->tree, other->dim - 1);
        self->degree = other->degree;
    }
    self->dim = other->dim;

    return CPL_ERROR_NONE;
}


/*----------------------------------------------------------------------------*/
/**
  @brief    Compare the coefficients of two polynomials
  @param    self   The 1st polynomial
  @param    other  The 2nd polynomial
  @param    tol  The absolute (non-negative) tolerance
  @return   0 when equal, positive when different, negative on error.

  The two polynomials are considered equal iff they have identical
  dimensions and the absolute difference between their coefficients
  does not exceed the given tolerance.

  This means that the following two polynomials per definition are
  considered different:
  P1(x1) = 3*x1 different from P2(x1,x2) = 3*x1.

  If all input parameters are valid and self and other point to the same
  polynomial the function returns 0.

  If two polynomials have different dimensions, the return value is this
  (positive) difference.

  If two 1D-polynomials differ, the return value is 1 plus the degree of the
  lowest order differing coefficient.

  If for a higher dimension they differ, it is 1 plus the degree of a
  differing coefficient.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_ILLEGAL_INPUT if tol is negative
 */
/*----------------------------------------------------------------------------*/
int cpl_polynomial_compare(const cpl_polynomial * self,
                           const cpl_polynomial * other,
                           double tol)
{

    cpl_ensure(self  != NULL, CPL_ERROR_NULL_INPUT, -1);
    cpl_ensure(other != NULL, CPL_ERROR_NULL_INPUT, -2);
    cpl_ensure(tol   >= 0.0,  CPL_ERROR_ILLEGAL_INPUT, -5);

    if (self == other) return 0;

    if (self->dim > other->dim) {
        return self->dim - other->dim;
    } else if (other->dim > self->dim) {
        return other->dim - self->dim;
    }

    /* The two polynomials have equal dimension */
    return cpl_polynomial_compare_(self->tree,
                                   other->tree,
                                   self->dim - 1,
                                   tol);
}

/*----------------------------------------------------------------------------*/
/**
  @brief    The degree of the polynomial
  @param    self    The polynomial to access
  @return   The degree or negative on error

  The degree is the highest sum of exponents (with a non-zero coefficient).

  If there are no non-zero coefficients the degree is zero.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
 */
/*----------------------------------------------------------------------------*/
cpl_size cpl_polynomial_get_degree(const cpl_polynomial * self)
{
    cpl_ensure(self != NULL, CPL_ERROR_NULL_INPUT, -1);

    return self->degree;
}

/*----------------------------------------------------------------------------*/
/**
  @brief    The dimension of the polynomial
  @param    self    The polynomial to access
  @return   The dimension or negative on error

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
 */
/*----------------------------------------------------------------------------*/
cpl_size cpl_polynomial_get_dimension(const cpl_polynomial * self)
{
    cpl_ensure(self != NULL, CPL_ERROR_NULL_INPUT, -1);

    return self->dim;
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Get a coefficient of the polynomial
  @param    self    The polynomial to process
  @param    pows    The non-negative power(s) of the variable(s)
  @return   The coefficient or undefined on error
  @note     For an N-dimensional polynomial the complexity is O(N)
  @see cpl_polynomial_set_coeff

  Requesting the value of a coefficient that has not been set is allowed,
  in this case zero is returned.

  Example of usage:
    @code

    const cpl_size power       = 3;
    double         coefficient = cpl_polynomial_get_coeff(poly1d, &power);

    @endcode

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_ILLEGAL_INPUT if pows contains negative values
 */
/*----------------------------------------------------------------------------*/
double cpl_polynomial_get_coeff(const cpl_polynomial * self,
                                const cpl_size       * pows)
{
    cpl_ensure(self != NULL, CPL_ERROR_NULL_INPUT, 0.0);
    cpl_ensure(pows != NULL, CPL_ERROR_NULL_INPUT, 0.0);

    for (cpl_size mydim = 0; mydim < self->dim; mydim++) {
        if (pows[mydim] < 0) {
            (void)cpl_error_set_message_(CPL_ERROR_ILLEGAL_INPUT, "Dimension %"
                                         CPL_SIZE_FORMAT " of %"CPL_SIZE_FORMAT
                                         " has negative power %"CPL_SIZE_FORMAT,
                                         mydim+1, self->dim, pows[mydim]);
            return 0.0;
        }
    }

    return cpl_polynomial_get_coeff_(self->tree, pows, self->dim - 1);
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Set a coefficient of the polynomial
  @param    self    The polynomial to modify
  @param    pows    The non-negative power(s) of the variable(s)
  @param    value   The coefficient
  @return   CPL_ERROR_NONE on success or else the relevant #_cpl_error_code_
  @note     For an N-dimensional polynomial the complexity is O(N)
  @see cpl_polynomial_get_coeff

  The array pows is assumed to have the size of the polynomial dimension.

  If the coefficient is already there, it is overwritten, if not, a new
  coefficient is added to the polynomial. This may cause the degree of the
  polynomial to be increased, or if the new coefficient is zero, to decrease.

  Setting the coefficient of x1^4 * x3^2 in the 4-dimensional polynomial
  poly4d to 12.3 would be performed by:

    @code

    const cpl_size pows[] = {4, 0, 2, 0};
    cpl_error_code error  = cpl_polynomial_set_coeff(poly4d, pows, 12.3);

    @endcode

  Setting the coefficient of x^3 in the 1-dimensional polynomial poly1d to
  12.3 would be performed by:
    @code

    const cpl_size power = 3;
    cpl_error_code error = cpl_polynomial_set_coeff(poly1d, &power, 12.3);

    @endcode


  For efficiency reasons, multiple coefficients are best inserted with the
  of the highest powers first.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_ILLEGAL_INPUT if pows contains negative values
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_polynomial_set_coeff(cpl_polynomial * self,
                                        const cpl_size * pows,
                                        double           value)
{
    cpl_size powdegree = 0;

    cpl_ensure_code(self != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(pows != NULL, CPL_ERROR_NULL_INPUT);

    for (cpl_size mydim = 0; mydim < self->dim; mydim++) {
        if (pows[mydim] < 0) return
            cpl_error_set_message_(CPL_ERROR_ILLEGAL_INPUT, "Dimension %"
                                   CPL_SIZE_FORMAT " of %" CPL_SIZE_FORMAT
                                   " has negative power %" CPL_SIZE_FORMAT,
                                   mydim+1, self->dim, pows[mydim]);
        powdegree += pows[mydim];
    }

    if (value != 0.0) {

        if (self->tree == NULL) 
            self->tree = cpl_polynomial_new_1d(1 + pows[self->dim - 1]);

        cpl_polynomial_set_coeff_(self->tree, pows, self->dim - 1, value);

        if (powdegree > self->degree) self->degree = powdegree;
    } else {
        cpl_polynomial_delete_coeff(self, pows, powdegree);
    }

    return CPL_ERROR_NONE;
}

/*----------------------------------------------------------------------------*/
/**
  @brief  Evaluate the polynomial at the given point using Horners rule.
  @param  self  The polynomial to access
  @param  x     Point of evaluation
  @note   The length of x must match the polynomial dimension.
  @return The computed value or undefined on error.

  A polynomial with no non-zero coefficients evaluates as 0.

  With n coefficients the complexity is about 2n FLOPs.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_INCOMPATIBLE_INPUT if the length of x differs from the dimension
    of the polynomial
 */
/*----------------------------------------------------------------------------*/
double cpl_polynomial_eval(const cpl_polynomial * self, const cpl_vector * x)
{
    cpl_ensure(self != NULL, CPL_ERROR_NULL_INPUT, -1.0);
    cpl_ensure(x    != NULL, CPL_ERROR_NULL_INPUT, -2.0);
    cpl_ensure(self->dim == cpl_vector_get_size_(x),
               CPL_ERROR_INCOMPATIBLE_INPUT, -3.0);

    return cpl_polynomial_eval_(self->tree, self->dim - 1,
                                cpl_vector_get_data_const_(x));
}

/*----------------------------------------------------------------------------*/
/**
  @brief   Collapse one dimension of a multi-variate polynomial by composition
  @param   self  The multi-variate polynomial
  @param   dim   The dimension to collapse (zero for first dimension)
  @param   other The polynomial to replace dimension dim of self
  @return  The collapsed polynomial or NULL on error
  @see cpl_polynomial_eval()

  The dimension of the polynomial self must be one greater than that of the
  other polynomial. Given these two polynomials the dimension dim of self is
  collapsed by creating a new polynomial from
    self(x0, x1, ..., x{dim-1},
         other(x0, x1, ..., x{dim-1}, x{dim+1}, x{dim+2}, ..., x{n-1}),
         x{dim+1}, x{dim+2}, ..., x{n-1}).

  The created polynomial thus has a dimension which is one less than the
  polynomial self and which is equal to that of the other polynomial.
  Collapsing one dimension of a 1D-polynomial is equivalent to evaluating it,
  which can be done with cpl_polynomial_eval_1d().

  FIXME: The other polynomial must currently have a degree of zero, i.e. it must
  be a constant.

  The collapse uses Horner's rule and requires for n coefficients requires
  about 2n FLOPs.

  The returned object is a newly allocated cpl_polynomial that
  must be deallocated by the caller using cpl_polynomial_delete().

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_INVALID_TYPE if the polynomial is uni-variate.
  - CPL_ERROR_ILLEGAL_INPUT if dim is negative.
  - CPL_ERROR_ACCESS_OUT_OF_RANGE if dim exceeds the dimension of self.
  - CPL_ERROR_INCOMPATIBLE_INPUT if other has the wrong dimension.
  - CPL_ERROR_UNSUPPORTED_MODE if other is not of degree 0.

 */
/*----------------------------------------------------------------------------*/
cpl_polynomial * cpl_polynomial_extract(const cpl_polynomial * self,
                                        cpl_size dim,
                                        const cpl_polynomial * other)
{

    cpl_polynomial * collapsed;
    cpl_size         newdim;


    cpl_ensure(self  != NULL,    CPL_ERROR_NULL_INPUT,          NULL);
    cpl_ensure(other != NULL,    CPL_ERROR_NULL_INPUT,          NULL);
    cpl_ensure(self->dim > 1,    CPL_ERROR_INVALID_TYPE,        NULL);
    cpl_ensure(dim >= 0,         CPL_ERROR_ILLEGAL_INPUT,       NULL);
    cpl_ensure(dim < self->dim,  CPL_ERROR_ACCESS_OUT_OF_RANGE, NULL);

    newdim = self->dim - 1;

    cpl_ensure(other->dim == newdim, CPL_ERROR_INCOMPATIBLE_INPUT, NULL);

    /* FIXME: Generalize this */
    cpl_ensure(cpl_polynomial_get_degree(other) == 0,
               CPL_ERROR_UNSUPPORTED_MODE, NULL);

    collapsed = cpl_polynomial_new(newdim);

    if (self->tree != NULL) {
        cpl_size * pows = cpl_calloc(newdim, sizeof(*pows));
        const double value = cpl_polynomial_get_coeff(other, pows);

        cpl_free(pows);
        collapsed->tree = cpl_polynomial_new_1d(self->tree->nc);

        if (cpl_polynomial_extract_(collapsed->tree, self->tree,
                                    self->dim - 1, dim, value)) {
            cpl_polynomial_empty(collapsed);
        } else {
            collapsed->degree = cpl_polynomial_find_degree_(collapsed->tree,
                                                            collapsed->dim - 1);
        }
    }

    return collapsed;

}

/*----------------------------------------------------------------------------*/
/**
  @brief    Compute a first order partial derivative
  @param    self  The polynomial to be modified in place
  @param    dim   The dimension to differentiate (zero for first dimension)
  @return   CPL_ERROR_NONE or the relevant #_cpl_error_code_

  The dimension of the polynomial is preserved, even if the operation may cause
  the polynomial to become independent of the dimension dim of the variable.

  The call requires n FLOPs, where n is the number of (non-zero) polynomial
  coefficients whose power in dimension dim is at least 1.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_ILLEGAL_INPUT if dim is negative.
  - CPL_ERROR_ACCESS_OUT_OF_RANGE if dim exceeds the dimension of self.
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_polynomial_derivative(cpl_polynomial * self, cpl_size dim)
{
    cpl_ensure_code(self != NULL,     CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(dim  >= 0,        CPL_ERROR_ILLEGAL_INPUT);
    cpl_ensure_code(dim  < self->dim, CPL_ERROR_ACCESS_OUT_OF_RANGE);

    if (self->tree != NULL) {
        if (cpl_polynomial_derivative_(self->tree, self->dim - 1, dim)) {
            cpl_polynomial_empty(self);
        } else {
            self->degree = cpl_polynomial_find_degree_(self->tree,
                                                       self->dim - 1);
        }
    }

    return CPL_ERROR_NONE;
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Evaluate a univariate (1D) polynomial using Horners rule.
  @param    self  The 1D-polynomial
  @param    x     The point of evaluation
  @param    pd    Iff pd is non-NULL, the derivative evaluated at x
  @return   The result or undefined on error.
  @see cpl_polynomial_eval()

  A polynomial with no non-zero coefficents evaluates to 0 with a
  derivative that does likewise.

  The result is computed as p_0 + x * ( p_1 + x * ( p_2 + ... x * p_n ))
  and requires 2n FLOPs where n+1 is the number of coefficients.

  If the derivative is requested it is computed using a nested Horner rule.
  This requires about 4n FLOPs.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_INVALID_TYPE if the polynomial is not (1D) univariate
 */
/*----------------------------------------------------------------------------*/
double cpl_polynomial_eval_1d(const cpl_polynomial * self, double x,
                              double * pd)
{
    cpl_ensure(self      != NULL, CPL_ERROR_NULL_INPUT,   -1.0);
    cpl_ensure(self->dim == 1,    CPL_ERROR_INVALID_TYPE, -3.0);

    if (self->tree == NULL) {
        if (pd != NULL) *pd = 0.0;
        return 0.0;
    }

    return cpl_polynomial_eval_1d_(self->tree, x, pd);
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Evaluate p(a) - p(b) using Horners rule.
  @param    self  The 1D-polynomial
  @param    a     The evaluation point of the minuend
  @param    b     The evaluation point of the subtrahend
  @param    ppa   Iff ppa is not NULL, p(a)
  @return   The difference or undefined on error

  The call requires about 4n FLOPs where n is the number of coefficients in
  self, which is the same as that required for two separate polynomial
  evaluations. cpl_polynomial_eval_1d_diff() is however more accurate.

  ppa may be NULL. If it is not, *ppa is set to self(a), which is calculated at
  no extra cost.

  The underlying algorithm is the same as that used in cpl_polynomial_eval_1d()
  when the derivative is also requested.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_INVALID_TYPE if the polynomial has the wrong dimension
 */
/*----------------------------------------------------------------------------*/
double cpl_polynomial_eval_1d_diff(const cpl_polynomial * self, double a,
                                   double b, double * ppa)
{
    double result;

    cpl_ensure(self      != NULL, CPL_ERROR_NULL_INPUT,   -1.0);
    cpl_ensure(self->dim == 1,    CPL_ERROR_INVALID_TYPE, -3.0);

    if (self->tree == NULL) {
        if (ppa != NULL) *ppa = 0.0;
        result = 0.0;
    } else {
        cpl_long_double pa;

        result = cpl_polynomial_nested_horner(self->tree, a, b, &pa) * (a - b);
        if (ppa != NULL) *ppa = (double)pa;
    }

    return result;
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Evaluate a 1D-polynomial on equidistant points using Horners rule
  @param    v  Preallocated vector to contain the result
  @param    p  The 1D-polynomial
  @param    x0 The first point of evaluation
  @param    d  The increment between points of evaluation
  @return   CPL_ERROR_NONE or the relevant #_cpl_error_code_
  @see cpl_vector_fill

  The evaluation points are x_i = x0 + i * d, i=0, 1, ..., n-1,
  where n is the length of the vector.

  If d is zero it is preferable to simply use
  cpl_vector_fill(v, cpl_polynomial_eval_1d(p, x0, NULL)).

  The call requires about 2nm FLOPs, where m+1 is the number of coefficients in
  p.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_INVALID_TYPE if the polynomial has the wrong dimension
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_vector_fill_polynomial(cpl_vector * v,
                                          const cpl_polynomial * p,
                                          double x0, double d)
{
    cpl_size i;
    double * dv;

    cpl_ensure_code(v != NULL,   CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(p != NULL,   CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(p->dim == 1, CPL_ERROR_INVALID_TYPE);

    i  = cpl_vector_get_size_(v);
    dv = cpl_vector_get_data_(v);

    cpl_tools_add_flops( 2 * i );

    do {
        i--;
        dv[i] = cpl_polynomial_eval_1d(p, x0 + (double)i * d, NULL);
    } while (i > 0);

    return CPL_ERROR_NONE;
}

/*----------------------------------------------------------------------------*/
/**
  @brief    A real solution to p(x) = 0 using Newton-Raphsons method
  @param    p    The 1D-polynomial
  @param    x0   First guess of the solution
  @param    px   The solution, on error see below
  @param    mul  The root multiplicity (or 1 if unknown)
  @return   CPL_ERROR_NONE or the relevant #_cpl_error_code_

  Even if a real solution exists, it may not be found if the first guess is
  too far from the solution. But a solution is guaranteed to be found if all
  roots of p are real. If the constant term is zero, the solution 0 will be
  returned regardless of the first guess.

  No solution is found when the iterative process stops because:
  1) It can not proceed because p`(x) = 0 (CPL_ERROR_DIVISION_BY_ZERO).
  2) Only a finite number of iterations are allowed (CPL_ERROR_CONTINUE).
  Both cases may be due to lack of a real solution or a bad first guess.
  In these two cases *px is set to the value where the error occurred.
  In case of other errors *px is unmodified.

  The accuracy and robustness deteriorates with increasing multiplicity
  of the solution. This is also the case with numerical multiplicity,
  i.e. when multiple solutions are located close together.

  mul is assumed to be the multiplicity of the solution. Knowledge of the
  root multiplicity often improves the robustness and accuracy. If there
  is no knowledge of the root multiplicity mul should be 1.
  Setting mul to a too high value should be avoided.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_INVALID_TYPE if the polynomial has the wrong dimension
  - CPL_ERROR_ILLEGAL_INPUT if the multiplicity is non-positive
  - CPL_ERROR_DIVISION_BY_ZERO if a division by zero occurs
  - CPL_ERROR_CONTINUE if the algorithm does not converge
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_polynomial_solve_1d(const cpl_polynomial * p,
                                       double x0, double * px, cpl_size mul)
{
    return cpl_polynomial_solve_1d_(p, x0, px, mul, CPL_FALSE)
        ? cpl_error_set_where_() : CPL_ERROR_NONE;
}

/*----------------------------------------------------------------------------*/
/**
  @brief  Fit a polynomial to a set of samples in a least squares sense
  @param  self    Polynomial of dimension d to hold the coefficients
  @param  samppos Matrix of p sample positions, with d rows and p columns
  @param  sampsym NULL, or d booleans, true iff the sampling is symmetric
  @param  fitvals Vector of the p values to fit
  @param  fitsigm Uncertainties of the sampled values, or NULL for all ones
  @param  dimdeg  True iff there is a fitting degree per dimension
  @param  mindeg  Pointer to 1 or d minimum fitting degree(s), or NULL
  @param  maxdeg  Pointer to 1 or d maximum fitting degree(s), at least mindeg
  @return CPL_ERROR_NONE on success, else the relevant #_cpl_error_code_
  @note Currently only uni- and bi-variate polynomials are supported,
        fitsigm must be NULL. For all but uni-variate polynomials mindeg must
        be zero.
  @see cpl_vector_fill_polynomial_fit_residual()

  Any pre-set non-zero coefficients in self are overwritten or reset by the fit.

  For 1D-polynomials N = 1 + maxdeg - mindeg coefficients are fitted. A non-zero
  mindeg ensures that the fitted polynomial has a fix-point at zero.

  For multi-variate polynomials the fit depends on dimdeg:

  If dimdeg is false, an n-degree coefficient is fitted iff
  mindeg <= n <= maxdeg. For a 2D-polynomial this means that
  N * (N + 1) / 2 coefficients are fitted.

  If dimdeg is true, nci = 1 + maxdeg[i] + mindeg[i] coefficients are fitted
  for dimension i, i.e. for a 2D-polynomial N = nc1 * nc2 coefficients are
  fitted.

  The number of distinct samples should exceed the number of coefficients to
  fit. The number of distinct samples may also equal the number of coefficients
  to fit, but in this case the fit has another meaning (any non-zero residual
  is due to rounding errors, not a fitting error).
  It is an error to try to fit more coefficients than there are distinct
  samples.

  If the relative uncertainties of the sampled values are known, they may be
  passed via fitsigm. NULL means that all uncertainties equals one.

  sampsym is ignored if mindeg is nonzero, otherwise
  the caller may use sampsym to indicate an a priori knowledge that the sampling
  positions are symmetric. NULL indicates no knowledge of such symmetry.
  sampsym[i] may be set to true iff the sampling is symmetric around u_i, where
  u_i is the mean of the sampling positions in dimension i.

  In 1D this implies that the sampling points as pairs average u_0 (with an odd
  number of samples one sample must equal u_0). E.g. both x = (1, 2, 4, 6, 7)
  and x = (1, 6, 4, 2, 7) have sampling symmetry, while x = (1, 2, 4, 6) does
  not.

  In 2D this implies that the sampling points are symmetric in the 2D-plane.
  For the first dimension sampling symmetry means that the sampling is line-
  symmetric around y = u_1, while for the second dimension, sampling symmetry
  implies line-symmetry around x = u_2. Point symmetry around
  (x,y) = (u_1, u_2) means that both sampsym[0] and sampsym[1] may be set to
  true.

  Knowledge of symmetric sampling allows the fit to be both faster and
  eliminates certain round-off errors.

  For higher order fitting the fitting problem known as "Runge's phenomenon"
  is minimized using the socalled "Chebyshev nodes" as sampling points.
  For Chebyshev nodes sampsym can be set to CPL_TRUE.

  Warning: An increase in the polynomial degree will normally reduce the
  fitting error. However, due to rounding errors and the limited accuracy
  of the solver of the normal equations, an increase in the polynomial degree
  may at some point cause the fitting error to _increase_. In some cases this
  happens with an increase of the polynomial degree from 8 to 9.

  The fit is done in the following steps:
  1) If mindeg is zero, the sampling positions are first transformed into
     Xhat_i = X_i - mean(X_i), i=1, .., dimension.
  2) The Vandermonde matrix is formed from Xhat.
  3) The normal equations of the Vandermonde matrix is solved.
  4) If mindeg is zero, the resulting polynomial in Xhat is transformed
      back to X.

  For a univariate (1D) fit the call requires 6MN + N^3/3 + 7/2N^2 + O(M) FLOPs
  where M is the number of data points and where N is the number of polynomial
  coefficients to fit, N = 1 + maxdeg - mindeg.

  For a bivariate fit the call requires MN^2 + N^3/3 + O(MN) FLOPs where M
  is the number of data points and where N is the number of polynomial
  coefficients to fit.

  Examples of usage:
    @code

    cpl_polynomial  * fit1d     = cpl_polynomial_new(1);
    cpl_matrix      * samppos1d = my_sampling_points_1d(); // 1-row matrix
    cpl_vector      * fitvals   = my_sampling_values();
    const cpl_boolean sampsym   = CPL_TRUE;
    const cpl_size    maxdeg1d  = 4; // Fit 5 coefficients
    cpl_error_code    error1d
        = cpl_polynomial_fit(fit1d, samppos1d, &sampsym, fitvals, NULL,
                             CPL_FALSE, NULL, &maxdeg1d);
    @endcode


    @code

    cpl_polynomial  * fit2d      = cpl_polynomial_new(2);
    cpl_matrix      * samppos2d  = my_sampling_points_2d(); // 2-row matrix
    cpl_vector      * fitvals    = my_sampling_values();
    const cpl_size    maxdeg2d[] = {2, 1}; // Fit 6 coefficients
    cpl_error_code    error2d
        = cpl_polynomial_fit(fit2d, samppos2d, NULL, fitvals, NULL, CPL_FALSE,
                             NULL, maxdeg2d);

    @endcode

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_ILLEGAL_INPUT if a mindeg value is negative, or if a maxdeg value
    is less than the corresponding mindeg value.
  - CPL_ERROR_DATA_NOT_FOUND if the number of columns in samppos is less than
    the number of coefficients to be determined.
  - CPL_ERROR_INCOMPATIBLE_INPUT if samppos, fitvals or fitsigm have
    incompatible sizes, or if samppos, self or sampsym have incompatible sizes.
  - CPL_ERROR_SINGULAR_MATRIX if samppos contains too few distinct values
  - CPL_ERROR_DIVISION_BY_ZERO if an element in fitsigm is zero
  - CPL_ERROR_UNSUPPORTED_MODE if the polynomial dimension exceeds two
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_polynomial_fit(cpl_polynomial    * self,
                                  const cpl_matrix  * samppos,
                                  const cpl_boolean * sampsym,
                                  const cpl_vector  * fitvals,
                                  const cpl_vector  * fitsigm,
                                  cpl_boolean         dimdeg,
                                  const cpl_size    * mindeg,
                                  const cpl_size    * maxdeg)
{

    const cpl_size mdim = cpl_polynomial_get_dimension(self);
    const cpl_size np   = cpl_vector_get_size(fitvals);

    cpl_ensure_code(self    != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(samppos != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(fitvals != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(maxdeg  != NULL, CPL_ERROR_NULL_INPUT);

    if (cpl_matrix_get_ncol(samppos) != np)
        return cpl_error_set_message_(CPL_ERROR_INCOMPATIBLE_INPUT, "Number of "
                                      "fitting values = %" CPL_SIZE_FORMAT
                                      " <=> % " CPL_SIZE_FORMAT " = samppos col"
                                      "umns", np, cpl_matrix_get_ncol(samppos));

    if (cpl_matrix_get_nrow(samppos) != mdim)
        return cpl_error_set_message_(CPL_ERROR_INCOMPATIBLE_INPUT, "Fitting "
                                      "dimension = %" CPL_SIZE_FORMAT " <=> %"
                                      CPL_SIZE_FORMAT " = samppos rows", mdim,
                                      cpl_matrix_get_nrow(samppos));

    if (fitsigm != NULL && cpl_vector_get_size_(fitsigm) != np)
        return cpl_error_set_message_(CPL_ERROR_INCOMPATIBLE_INPUT, "Number of "
                                      "fitting values = %" CPL_SIZE_FORMAT " <="
                                      "> % " CPL_SIZE_FORMAT " = number of er"
                                      "rors", np, cpl_vector_get_size(fitsigm));

    if (fitsigm != NULL)
        return cpl_error_set_message_(CPL_ERROR_UNSUPPORTED_MODE, "fitsigm = "
                                      "%p != NULL is not supported (yet)",
                                      (const void*)fitsigm);

CPL_DIAG_PRAGMA_PUSH_IGN(-Wcast-qual);
    if (mdim == 1) {
        cpl_vector * x_pos
            = cpl_vector_wrap(np, cpl_matrix_get_data((cpl_matrix*)samppos));
        const cpl_boolean isampsym = sampsym ? sampsym[0] : CPL_FALSE;
        const int         mindeg0  = mindeg ? mindeg[0] : 0;
        const cpl_error_code error = cpl_polynomial_fit_1d(self, x_pos, fitvals,
                                                           mindeg0, maxdeg[0],
                                                           isampsym, NULL);
CPL_DIAG_PRAGMA_POP;

        (void)cpl_vector_unwrap(x_pos);
        if (error) return cpl_error_set_where_();
CPL_DIAG_PRAGMA_PUSH_IGN(-Wcast-qual);
    } else if (mdim == 2) {
        cpl_vector * x_pos
            = cpl_vector_wrap(np, cpl_matrix_get_data((cpl_matrix *)samppos));
        cpl_vector * y_pos = cpl_vector_wrap(np, np
                              + cpl_matrix_get_data((cpl_matrix *)samppos));
        cpl_bivector * xy_pos = cpl_bivector_wrap_vectors(x_pos, y_pos);
        const cpl_size      mindeg0[2] = {mindeg ? mindeg[0] : 0,
                                          mindeg ? mindeg[1] : 0};
        const cpl_error_code error = cpl_polynomial_fit_2d(self, xy_pos,
                                                           fitvals, dimdeg,
                                                           mindeg0,
                                                           maxdeg);
CPL_DIAG_PRAGMA_POP;
        (void)cpl_vector_unwrap(x_pos);
        (void)cpl_vector_unwrap(y_pos);
        cpl_bivector_unwrap_vectors(xy_pos);

        if (error) return cpl_error_set_where_();
    } else {
        assert( mdim > 2 );
        return cpl_error_set_message_(CPL_ERROR_UNSUPPORTED_MODE, "The fitting "
                                      "dimension %d > 2 is not supported",
                                      (int)mdim);
    }

    return CPL_ERROR_NONE;
}

/*----------------------------------------------------------------------------*/
/**
  @brief  Compute the residual of a polynomial fit
  @param  self    Vector to hold the fitting residuals, fitvals may be used
  @param  fitvals Vector of the p fitted values
  @param  fitsigm Uncertainties of the sampled values or NULL for a uniform
                  uncertainty
  @param  fit     The fitted polynomial
  @param  samppos Matrix of p sample positions, with d rows and p columns
  @param  rechisq If non-NULL, the reduced chi square of the fit
  @return CPL_ERROR_NONE on success, else the relevant #_cpl_error_code_
  @note If necessary, self is resized to the length of fitvals.
  @see cpl_polynomial_fit()

  It is allowed to pass the same vector as both fitvals and as self,
  in which case fitvals is overwritten with the residuals.

  If the relative uncertainties of the sampled values are known, they may be
  passed via fitsigm. NULL means that all uncertainties equal one. The
  uncertainties are taken into account when computing the reduced
  chi square value.

  If rechisq is non-NULL, the reduced chi square of the fit is computed as
  well.

  The mean square error, which was computed directly by the former CPL functions
  cpl_polynomial_fit_1d_create() and cpl_polynomial_fit_2d_create() can be
  computed from the fitting residual like this:

  @code
    const double mse = cpl_vector_product(fitresidual, fitresidual)
                     / cpl_vector_get_size(fitresidual);
  @endcode

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer (other than fitsigm) is NULL
  - CPL_ERROR_INCOMPATIBLE_INPUT if samppos, fitvals, fitsigm or fit
    have incompatible sizes
  - CPL_ERROR_DIVISION_BY_ZERO if an element in fitsigm is zero
  - CPL_ERROR_DATA_NOT_FOUND if the number of columns in samppos is less than
    the number of coefficients in the fitted polynomial.
 */
/*----------------------------------------------------------------------------*/
cpl_error_code
cpl_vector_fill_polynomial_fit_residual(cpl_vector           * self,
                                        const cpl_vector     * fitvals,
                                        const cpl_vector     * fitsigm,
                                        const cpl_polynomial * fit,
                                        const cpl_matrix     * samppos,
                                        double               * rechisq)
{
    const cpl_size mdim = cpl_polynomial_get_dimension(fit);
    const cpl_size np = cpl_vector_get_size(fitvals);
    const cpl_size nc = fit && fit->tree ? fit->tree->nc : 0;
    const double * dsamppos = cpl_matrix_get_data_const(samppos);
    const double * dfitvals = cpl_vector_get_data_const(fitvals);
    double       * dself;
    cpl_size       i, j;

    cpl_ensure_code(self     != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(dfitvals != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(fit      != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(dsamppos != NULL, CPL_ERROR_NULL_INPUT);

    cpl_ensure_code(cpl_matrix_get_ncol(samppos) == np,
                    CPL_ERROR_INCOMPATIBLE_INPUT);

    cpl_ensure_code(cpl_matrix_get_nrow(samppos) == mdim,
                    CPL_ERROR_INCOMPATIBLE_INPUT);

    /* May not be under-determined */
    cpl_ensure_code(np >= nc, CPL_ERROR_DATA_NOT_FOUND);

    cpl_vector_set_size(self, np);
    dself = cpl_vector_get_data_(self);

    if (mdim == 1) {
        for (i = 0; i < np; i++) {
            dself[i] = dfitvals[i] - cpl_polynomial_eval_1d(fit, dsamppos[i],
                                                            NULL);
        }
    } else {
        double     * dsampoint = cpl_malloc((size_t)mdim * sizeof(*dsampoint));
        cpl_vector * sampoint  = cpl_vector_wrap(mdim, dsampoint);

        for (i = 0; i < np; i++) {
            for (j = 0; j < mdim; j++) {
                dsampoint[j] = dsamppos[i + j * np];
            }
            dself[i] = dfitvals[i] - cpl_polynomial_eval(fit, sampoint);
        }

        cpl_vector_delete(sampoint);
    }

    cpl_tools_add_flops( np );

    if (rechisq != NULL) {
        /* Assuming that the np sampling points are distinct! */
        const int nfree = np - nc;

        cpl_ensure_code(nfree > 0, CPL_ERROR_DATA_NOT_FOUND);

        if (fitsigm == NULL) {
            *rechisq = cpl_vector_product(self, self) / (double)nfree;
        } else {
            const double * dsigm = cpl_vector_get_data_const_(fitsigm);
            double dot = 0.0;

            cpl_ensure_code(cpl_vector_get_size_(fitsigm) == np,
                            CPL_ERROR_INCOMPATIBLE_INPUT);

            for (i = 0; i < np; i++) {
                double delta;
                /* Sigmas may be negative, the sign is ignored */
                cpl_ensure_code(dsigm[i] != 0.0, CPL_ERROR_DIVISION_BY_ZERO);
                delta = dself[i] / dsigm[i];
                dot += delta * delta;
            }
            *rechisq = dot / (double)nfree;
            cpl_tools_add_flops( 3 * np );
        }
    }

    return CPL_ERROR_NONE;
}

/*----------------------------------------------------------------------------*/
/**
  @brief  Modify p, p(x0, x1, ..., xi, ...) := (x0, x1, ..., xi+u, ...)
  @param  p    The polynomial to be modified in place
  @param  i    The dimension to shift (0 for first)
  @param  u    The shift
  @return   CPL_ERROR_NONE or the relevant #_cpl_error_code_

  Shifting the polynomial p(x) = x^n with u = 1 will generate the binomial
  coefficients for n.

  Shifting the coordinate system to (x,y) for the 2D-polynomium poly2d:

    @code
        cpl_polynomial_shift_1d(poly2d, 0, x);
        cpl_polynomial_shift_1d(poly2d, 1, y);
     @endcode

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_ILLEGAL_INPUT if i is negative
  - CPL_ERROR_ACCESS_OUT_OF_RANGE if i exceeds the dimension of p
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_polynomial_shift_1d(cpl_polynomial * p, cpl_size i, double u)
{

    const cpl_size ndim = cpl_polynomial_get_dimension(p);

    cpl_ensure_code(p != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(i >= 0,    CPL_ERROR_ILLEGAL_INPUT);
    cpl_ensure_code(i < ndim,  CPL_ERROR_ACCESS_OUT_OF_RANGE);

    if (p->tree != NULL) {
        cpl_ifalloc         mybuf;
        cpl_size          * pows = NULL;
        cpl_polynomial_1d * self = p->tree;
        cpl_polynomial_1d * transtree = NULL;

        if (i > 0) {
            /* Need to transpose the dimension to shift into lowest dimension */
            cpl_ifalloc_set(&mybuf, (size_t)p->dim * sizeof(*pows));
            pows = (cpl_size*)cpl_ifalloc_get(&mybuf);

            transtree = cpl_polynomial_new_1d(p->tree->nc); /* Guess for size */

            cpl_polynomial_transpose_(transtree, pows + p->dim - 1, p->dim - 1,
                                      pows, p->dim, p->tree, i);
            self = transtree;
        }

#ifdef CPL_POLYNOMIAL_UNION_IS_DOUBLE_SZ
        /* Assume no padding in a union of same sized primitive elements */
        assert((char*)&(self->coef[0].val) + (size_t)self->nc * sizeof(double) ==
               (char*)&(self->coef[self->nc].val));
#endif

        cpl_polynomial_shift_1d_(self, p->dim - 1, u);

        if (i > 0) {
            /* Need to transpose lowest dimension back into place */

            cpl_polynomial_reset_(p->tree, p->dim - 1);

            /* The transpose may leave some old coefficients untouched
               - and the reset will set all X-coefficient counters to zero,
               so if any remains zero, they must be pruned */

            cpl_polynomial_transpose_(p->tree, pows + p->dim - 1, p->dim - 1,
                                      pows, p->dim, transtree, i);

            (void)cpl_polynomial_prune_(p->tree, p->dim - 1, p->tree->nc);

            cpl_ifalloc_free(&mybuf);
            cpl_polynomial_delete_(transtree, p->dim - 1);
        }
    }

    return CPL_ERROR_NONE;
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Fit a 1D-polynomial to a 1D-signal in a least squares sense
  @param    x_pos   Vector of positions of the signal to fit.
  @param    values  Vector of values of the signal to fit.
  @param    degree  Non-negative polynomial degree.
  @param    pmse    Iff pmse is not null, the mean squared error on success
  @return   The fitted polynomial or NULL on error
  @see cpl_polynomial_fit()
  @deprecated Replace this call with cpl_polynomial_fit() and
     optionally cpl_vector_fill_polynomial_fit_residual().

 */
/*----------------------------------------------------------------------------*/
cpl_polynomial * cpl_polynomial_fit_1d_create(const cpl_vector    *   x_pos,
                                              const cpl_vector    *   values,
                                              cpl_size                degree,
                                              double              *   pmse)
{

    cpl_polynomial * self = cpl_polynomial_new(1);
    const cpl_boolean is_eqdist = cpl_vector_is_eqdist(x_pos) == 1;
    const cpl_error_code error = cpl_polynomial_fit_1d(self, x_pos, values, 0,
                                                       degree, is_eqdist, pmse);

    if (error != CPL_ERROR_NONE) {
        cpl_polynomial_delete(self);
        self = NULL;
        cpl_error_set_(error);
    }

    return self;
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Fit a 2D-polynomial to a 2D-surface in a least squares sense
  @param    xy_pos  Bivector  positions of the surface to fit.
  @param    values  Vector of values of the surface to fit.
  @param    degree  Non-negative polynomial degree.
  @param    pmse    Iff pmse is not null, the mean squared error on success
  @return   The fitted polynomial or NULL on error
  @see cpl_polynomial_fit()
  @deprecated Replace this call with cpl_polynomial_fit() and
     optionally cpl_vector_fill_polynomial_fit_residual().

 */
/*----------------------------------------------------------------------------*/
cpl_polynomial * cpl_polynomial_fit_2d_create(const cpl_bivector * xy_pos,
                                              const cpl_vector   * values,
                                              cpl_size             degree,
                                              double             * pmse)
{
    cpl_polynomial * self = cpl_polynomial_new(2);
    const cpl_size   mindeg0 = 0;
    const cpl_error_code error = cpl_polynomial_fit_2d(self, xy_pos, values,
                                                       CPL_FALSE, &mindeg0,
                                                       &degree);
    if (error != CPL_ERROR_NONE) {
        cpl_polynomial_delete(self);
        self = NULL;
        cpl_error_set_(error);
    } else if (pmse != NULL) {
        /* Compute mean squared error */
        const cpl_size     np      = cpl_vector_get_size(values);
        const cpl_vector * x_pos   = cpl_bivector_get_x_const(xy_pos);
        const cpl_vector * y_pos   = cpl_bivector_get_y_const(xy_pos);
        const double     * dx_pos  = cpl_vector_get_data_const(x_pos);
        const double     * dy_pos  = cpl_vector_get_data_const(y_pos);
        const double     * dvalues = cpl_vector_get_data_const(values);
        double             xyval[2];
        cpl_vector       * x_val   = cpl_vector_wrap(2, xyval);

        *pmse = 0.0;
        for (cpl_size i = 0; i < np; i++) {
            double residue;
            xyval[0] = dx_pos[i];
            xyval[1] = dy_pos[i];
            /* Subtract from the true value, square, accumulate */
            residue = dvalues[i] - cpl_polynomial_eval(self, x_val);
            *pmse += residue * residue;
        }
        (void)cpl_vector_unwrap(x_val);

        /* Average the error term */
        *pmse /= (double)np;

        cpl_tools_add_flops( 3 * np + 1 );
    }

    return self;
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Add two polynomials of the same dimension
  @param    self      The polynomial to hold the result
  @param    first     The 1st polynomial to add
  @param    second    The 2nd polynomial to add
  @return   CPL_ERROR_NONE or the relevant CPL error code
  @note self may be passed also as first and/or second

  Possible CPL error code set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_INCOMPATIBLE_INPUT if the polynomials do not have identical
    dimensions
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_polynomial_add(cpl_polynomial * self,
                                  const cpl_polynomial * first,
                                  const cpl_polynomial * second)
{
    cpl_ensure_code(self   != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(first  != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(second != NULL, CPL_ERROR_NULL_INPUT);

    cpl_ensure_code(cpl_polynomial_get_dimension(self) ==
                    cpl_polynomial_get_dimension(first),
                    CPL_ERROR_INCOMPATIBLE_INPUT);
    cpl_ensure_code(cpl_polynomial_get_dimension(self) ==
                    cpl_polynomial_get_dimension(second),
                    CPL_ERROR_INCOMPATIBLE_INPUT);

    if (first->tree == NULL && second->tree == NULL) {
        cpl_polynomial_empty(self);
    } else {
        if (self->tree == NULL) {
            const cpl_size nfirst  = first->tree  ? first->tree->nc  : 0;
            const cpl_size nsecond = second->tree ? second->tree->nc : 0;

            self->tree = cpl_polynomial_new_1d(CPL_MAX(nfirst, nsecond));
        }

        if (cpl_polynomial_add_(self->tree, first->tree, second->tree,
                                self->dim - 1)) {
            cpl_polynomial_empty(self);
        } else {
            self->degree = cpl_polynomial_find_degree_(self->tree,
                                                       self->dim - 1);
        }
    }

    return CPL_ERROR_NONE;
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Subtract two polynomials of the same dimension
  @param    self      The polynomial to hold the result
  @param    first     The polynomial to subtract from, or NULL
  @param    second    The polynomial to subtract, or NULL
  @return   CPL_ERROR_NONE or the relevant CPL error code
  @note self may be passed also as first and/or second

  Possible CPL error code set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_INCOMPATIBLE_INPUT if the polynomials do not have identical
    dimensions
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_polynomial_subtract(cpl_polynomial * self,
                                       const cpl_polynomial * first,
                                       const cpl_polynomial * second)
{
    cpl_ensure_code(self   != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(first  != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(second != NULL, CPL_ERROR_NULL_INPUT);

    cpl_ensure_code(cpl_polynomial_get_dimension(self) ==
                    cpl_polynomial_get_dimension(first),
                    CPL_ERROR_INCOMPATIBLE_INPUT);
    cpl_ensure_code(cpl_polynomial_get_dimension(self) ==
                    cpl_polynomial_get_dimension(second),
                    CPL_ERROR_INCOMPATIBLE_INPUT);

    if (first->tree == NULL && second->tree == NULL) {
        cpl_polynomial_empty(self);
    } else {
        if (self->tree == NULL) {
            const cpl_size nfirst  = first->tree  ? first->tree->nc  : 0;
            const cpl_size nsecond = second->tree ? second->tree->nc : 0;

            self->tree = cpl_polynomial_new_1d(CPL_MAX(nfirst, nsecond));
        }

        if (cpl_polynomial_subtract_(self->tree, first->tree,
                                     second->tree, self->dim - 1)) {
            cpl_polynomial_empty(self);
        } else {
            self->degree = cpl_polynomial_find_degree_(self->tree,
                                                       self->dim - 1);
        }
    }

    return CPL_ERROR_NONE;
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Multiply two polynomials of the same dimension
  @param    self      The polynomial to hold the result
  @param    first     The polynomial to multiply
  @param    second    The polynomial to multiply
  @return   CPL_ERROR_NONE or the relevant CPL error code
  @note self may be passed also as first and/or second
  @see cpl_polynomial_add()

  Possible CPL error code set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_INCOMPATIBLE_INPUT if the polynomials do not have identical
    dimensions
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_polynomial_multiply(cpl_polynomial * self,
                                       const cpl_polynomial * first,
                                       const cpl_polynomial * second)
{
    cpl_polynomial * original = self;
    cpl_polynomial * result = NULL;

    cpl_ensure_code(self   != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(first  != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(second != NULL, CPL_ERROR_NULL_INPUT);

    cpl_ensure_code(cpl_polynomial_get_dimension(self) ==
                    cpl_polynomial_get_dimension(first),
                    CPL_ERROR_INCOMPATIBLE_INPUT);
    cpl_ensure_code(cpl_polynomial_get_dimension(self) ==
                    cpl_polynomial_get_dimension(second),
                    CPL_ERROR_INCOMPATIBLE_INPUT);

    if (self == first || self == second) {
        /* For an inplace multiplication, we need some temporary storage */
        self = result = cpl_polynomial_new(cpl_polynomial_get_dimension(self));
    } else {
        cpl_polynomial_empty(self);
    }

    /* self is now empty.
       Both first and second must be non-empty for that to change */

    if (first->tree != NULL && second->tree != NULL) {
        cpl_size* pows = cpl_malloc((size_t)self->dim * sizeof(*pows));

        if (self->tree == NULL) {
            assert(first->tree->nc + second->tree->nc > 0);

            self->tree = cpl_polynomial_new_1d(first->tree->nc +
                                               second->tree->nc);
        }

        cpl_polynomial_multiply_(self->tree, first->tree, second->tree,
                                 pows + self->dim - 1, self->dim - 1,
                                 pows, self->dim);
        cpl_free(pows);

        self->degree = cpl_polynomial_find_degree_(self->tree, self->dim - 1);
    }

    if (result != NULL) {
        void * swap = cpl_malloc(sizeof(cpl_polynomial));

        assert(original != self);

        memcpy(swap, self,     sizeof(cpl_polynomial));
        memcpy(self, original, sizeof(cpl_polynomial));
        memcpy(original, swap, sizeof(cpl_polynomial));

        /* Delete the buffers originally used by self - and the swap space */
        cpl_polynomial_delete(result);
        cpl_free(swap);
    }

    return CPL_ERROR_NONE;
}

/*----------------------------------------------------------------------------*/
/**
  @brief    Multiply a polynomial with a scalar
  @param    self      The polynomial to hold the result
  @param    other     The polynomial to scale of same dimension, may equal self
  @param    factor    The factor to multiply with
  @return   CPL_ERROR_NONE or the relevant CPL error code
  @note If factor is zero all coefficients are reset, if it is 1 all are copied

  Possible CPL error code set in this function:
  - CPL_ERROR_NULL_INPUT if an input pointer is NULL
  - CPL_ERROR_INCOMPATIBLE_INPUT if the two dimensions do not match
 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_polynomial_multiply_scalar(cpl_polynomial * self,
                                              const cpl_polynomial * other,
                                              double factor)
{

    cpl_ensure_code(self  != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(other != NULL, CPL_ERROR_NULL_INPUT);

    cpl_ensure_code(cpl_polynomial_get_dimension(self) ==
                    cpl_polynomial_get_dimension(other),
                    CPL_ERROR_INCOMPATIBLE_INPUT);

    if (factor != 0.0 && other->tree != NULL) {
        if (self == other) {
            cpl_polynomial_multiply_scalar_self(self->tree,
                                                self->dim - 1, factor);
        } else {
            if (self->tree == NULL)
                self->tree = cpl_polynomial_new_1d(other->tree->nc);

            cpl_polynomial_multiply_scalar_(self->tree, other->tree,
                                            self->dim - 1, factor);
            self->degree = other->degree;
        }
    } else {
        cpl_polynomial_empty(self);
    }

    return CPL_ERROR_NONE;
}

/**@}*/

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    The size of the numeric coefficients of the 1D-polynomial
  @param    self    The non-empty 1D polynomial to access
  @return   The size in bytes
  @see cpl_polynomial_shift_1d_()
  @note No error checks in this internal function

 */
/*----------------------------------------------------------------------------*/
cpl_size cpl_polynomial_get_1d_size(const cpl_polynomial * self)
{
    return
        (char*)&(self->tree->coef[self->tree->nc].val) -
        (char*)&(self->tree->coef[0             ].val);
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Fit a 1D-polynomial to a 1D-signal in a least squares sense
  @param  self    1D-polynomial to hold the fit
  @param  x_pos   Vector of positions of the signal to fit.
  @param  values  Vector of values of the signal to fit.
  @param  mindeg  The non-negative minimum fitting degree
  @param  degree  The polynomial fitting degree, at least mindeg
  @param  symsamp True iff the x_pos values are symmetric around their mean
  @param  pmse    Iff pmse is not null, the mean squared error on success
  @return The fitted polynomial or NULL on error
  @see cpl_polynomial_fit()

  symsamp is ignored if mindeg is nonzero, otherwise
  symsamp may to be set to CPL_TRUE if and only if the values in x_pos are
  known a-priori to be symmetric around their mean, e.g. (1, 2, 4, 6, 10,
  14, 16, 18, 19), but not (1, 2, 4, 6, 10, 14, 16). Setting symsamp to
  CPL_TRUE while mindeg is zero eliminates certain round-off errors.
  For higher order fitting the fitting problem known as "Runge's phenomenon"
  is minimized using the socalled "Chebyshev nodes" as sampling points.
  For Chebyshev nodes symsamp can be set to CPL_TRUE.

 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_polynomial_fit_1d(cpl_polynomial * self,
                                     const cpl_vector * x_pos,
                                     const cpl_vector * values,
                                     cpl_size mindeg, cpl_size degree,
                                     cpl_boolean symsamp, double * pmse)
{

    /* Number of unknowns to determine */
    const cpl_size     nc = 1 + degree - mindeg;
    const cpl_size     np = cpl_vector_get_size(x_pos);
    double             mean;
    double             delta, xval;
    cpl_vector       * xhat;
    const cpl_vector * xuse;
    cpl_matrix       * mh;   /* Hankel matrix */
    cpl_matrix       * mx;
    double           * dx;
    cpl_size           i, j;
    cpl_error_code     error;


    cpl_ensure_code(np > 0,      cpl_error_get_code());
    cpl_ensure_code(values,      CPL_ERROR_NULL_INPUT);

    cpl_ensure_code(cpl_vector_get_size_(values) == np,
                    CPL_ERROR_INCOMPATIBLE_INPUT);

    cpl_ensure_code(mindeg >= 0,      CPL_ERROR_ILLEGAL_INPUT);
    cpl_ensure_code(degree >= mindeg, CPL_ERROR_ILLEGAL_INPUT);

    cpl_ensure_code(cpl_polynomial_get_dimension(self) == 1,
                    CPL_ERROR_INCOMPATIBLE_INPUT);

    symsamp = symsamp && (mindeg == 0); /* symsamp not usable with mindeg > 0 */

    if (self->tree != NULL) {
        /* Reset coefficients */
        cpl_polynomial_reset_(self->tree, self->dim - 1);
        self->degree = 0;

        /* When a coefficient is set with cpl_polynomial_set_coeff(),
           the tree is set properly, or deallocated */
    }

    if (degree == mindeg) {
        /* Handle this one-coefficient polynomial as a special case */

        if (degree == 0) {
            /* Handle this zero-degree polynomial as a special case */

            /* If requested, compute mean squared error */
            if (pmse != NULL) *pmse =
                cpl_tools_get_variance_double(cpl_vector_get_data_const_(values),
                                              np, &mean);
            else
                mean = cpl_vector_get_mean(values);

        } else {
            /* A polynomial with just one coefficient and a positive degree
               is requested. The coefficient must therefore be non-zero. */

            /* Raise values to the power of mindeg */
            const double * xpos = cpl_vector_get_data_const_(x_pos);
            const double * dval = cpl_vector_get_data_const_(values);
            double h = 0.0; /* Hankel = Transpose(Vandermonde) * Vandermonde */
            double vtv = 0.0; /* Transpose(Vandermonde) * values */

            for (i=0; i < np; i++) {
                const double xn = cpl_tools_ipow(xpos[i], (int)mindeg);

                vtv += xn * dval[i];
                h   += xn * xn;
            }

            if (h > 0.0) {
                mean = vtv / h;
            } else {
                cpl_polynomial_empty(self);
                return cpl_error_set_message_(CPL_ERROR_DIVISION_BY_ZERO,
                                             "mindeg=%" CPL_SIZE_FORMAT ". "
                                              "degree=%" CPL_SIZE_FORMAT ". "
                                              "nc=%" CPL_SIZE_FORMAT ". "
                                              "np=%" CPL_SIZE_FORMAT ". "
                                              "Coeff = %g / %g",
                                              mindeg, degree,
                                              nc, np, vtv, h);
            }
        }
        /* Should not be able to fail now, nevertheless propagate */
        return cpl_error_set_(cpl_polynomial_set_coeff(self, &degree, mean));
    }
    cpl_ensure_code(np >= nc,   CPL_ERROR_DATA_NOT_FOUND);

    /* The Hankel matrix may be singular in such a fashion, that the pivot
       points in its Cholesky decomposition are positive due to rounding errors.
       To ensure that such singular systems are robustly detected, the number of
       distinct sampling points is counted.
    */

    cpl_ensure_code(!cpl_vector_ensure_distinct(x_pos, nc),
               CPL_ERROR_SINGULAR_MATRIX);

    /* Try to detect if the x-points are equidistant
       - in which every other skew diagonal of the Hankel matrix is zero */
    xval = cpl_vector_get(x_pos, 1);
    delta = xval - cpl_vector_get(x_pos, 0);
    for (i=1; i < np-1; i++) {
        const double dprev = delta;
        const double xprev = xval;

        xval = cpl_vector_get_(x_pos, i+1);
        delta = xval - xprev;
        if (delta != dprev) break;
    }

    if (mindeg == 0) {
        /* Transform: xhat = x - mean(x) */
        xhat = cpl_vector_transform_mean(x_pos, &mean);
        xuse = xhat;
    } else {
        mean = 0.0;
        xhat = NULL;
        xuse = x_pos;
    }

    assert( xuse != NULL );

    /* Generate Hankel matrix, H = V' * V, where V is the Vandermonde matrix */
    /* FIXME: It is faster and likely more accurate to compute the QR
       factorization of the Vandermonde matrix, QR = V, see
       C.J. Demeure: Fast QR Factorization of Vandermonde Matrices */

    /* mh is initialized only if xuse is not equidistant */
    mh = symsamp ? cpl_matrix_new(nc, nc) :
        cpl_matrix_wrap(nc, nc, cpl_malloc((size_t)(nc * nc) * sizeof(double)));
    dx = (double*)cpl_malloc((size_t)(nc *  1) * sizeof(*dx));
    mx = cpl_matrix_wrap(nc, 1, dx);

    cpl_matrix_fill_normal_vandermonde(mh, mx, xuse, symsamp, mindeg, values);
#ifdef CPL_POLYNOMIAL_FIT_DEBUG
    cpl_msg_warning(cpl_func, "MINDEG=%" CPL_SIZE_FORMAT ". degree=%"
                    CPL_SIZE_FORMAT ". nc=%" CPL_SIZE_FORMAT ". np=%"
                    CPL_SIZE_FORMAT ". mean=%g", mindeg, degree, nc, np, mean);
    cpl_matrix_dump(mh, stdout);
    cpl_matrix_dump(mx, stdout);
#endif

    cpl_vector_delete(xhat);

    error = cpl_matrix_solve_spd(mh, mx);

    cpl_matrix_delete(mh);

    if (error) {
        cpl_matrix_delete(mx);
        cpl_polynomial_empty(self);
        return cpl_error_set_message_(error, "mindeg=%" CPL_SIZE_FORMAT
                                      ". degree=%" CPL_SIZE_FORMAT ". nc=%"
                                      CPL_SIZE_FORMAT ". np=%" CPL_SIZE_FORMAT
                                      ". mean=%g", mindeg, degree, nc, np,
                                      mean);
    }


    /* Scale back - and store coefficients - with leading coefficient first,
       see doxygen of cpl_polynomial_set_coeff() */
    /* - make sure there is enough space, then fill up from below */
    if (self->tree == NULL) {
        self->tree = cpl_polynomial_new_1d(mindeg + nc);
    } else {
        cpl_polynomial_grow_1d(self->tree, mindeg + nc);
    }
    for (j = 0; j < nc; j++) {
        if (dx[j] != 0.0) {
            /* Since the tree may be a zero-polynomial,
               deleting a coefficient (and determining the new
               degree) can cause an assert() to fail */

            const cpl_size k = j + mindeg;

            cpl_polynomial_set_coeff(self, &k, dx[j]);
        }
    }

    if (self->tree->nc == 0) {
        /* All fitted coefficients are zero */
        cpl_polynomial_empty(self);
    }

    cpl_matrix_delete(mx);

    if (mindeg == 0) {
        /* Shift back */
        cpl_polynomial_shift_1d(self, 0, -mean);
    }

    /* If requested, compute mean squared error */
    if (pmse != NULL) {
        const double * xpos = cpl_vector_get_data_const_(x_pos);
        const double * dval = cpl_vector_get_data_const_(values);
        *pmse = 0.0;
        for (i = 0; i < np; i++) {
            /* Subtract from the true value, square, accumulate */
            const double residue = dval[i]
                - cpl_polynomial_eval_1d(self, xpos[i], NULL);
            *pmse += residue * residue;
        }
        /* Average the error term */
        *pmse /= (double)np;

        cpl_tools_add_flops( 3 * np + 1 );
    }

    return CPL_ERROR_NONE;
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Fit a 2D-polynomial to a 2D-surface in a least squares sense
  @param  xy_pos  Bivector  positions of the surface to fit.
  @param  values  Vector of values of the surface to fit.
  @param  dimdeg  True iff there is a fitting degree per dimension
  @param  mindeg  Pointer to 1 or d maximum fitting degree(s), at least 0
  @param  maxdeg  Pointer to 1 or d maximum fitting degree(s), at least mindeg
  @return The fitted polynomial or NULL on error
  @see cpl_polynomial_fit()
 */
/*----------------------------------------------------------------------------*/
static cpl_error_code cpl_polynomial_fit_2d(cpl_polynomial * self,
                                            const cpl_bivector * xy_pos,
                                            const cpl_vector * values,
                                            cpl_boolean dimdeg,
                                            const cpl_size * mindeg,
                                            const cpl_size * maxdeg)
{

    const cpl_size    np = cpl_bivector_get_size(xy_pos);
    cpl_size          degree; /* The degree of the fitted polynomial */
    cpl_boolean       is_mindeg = CPL_FALSE; /* Whether mindeg is non-zero */
    cpl_size          nc;   /* Number of unknowns to determine */
    cpl_size          ncy;  /* Number of Y-coefficients (incl. mindeg ones) */
    cpl_matrix      * mv;   /* The transpose of the Vandermonde matrix */
    cpl_matrix      * mh;   /* Block-Hankel matrix, V'*V */
    cpl_matrix      * mb;
    cpl_matrix      * mx;
    const double    * coeffs1d;
    double          * dmv;
    cpl_vector      * xhat;
    cpl_vector      * yhat;
    const cpl_vector* xuse;
    const cpl_vector* yuse;
    const double    * dxuse;
    const double    * dyuse;
    double            xmean;
    double            ymean;
    cpl_size          powers[2];
    cpl_size          degx, degy;
    cpl_size          i, j;
    cpl_boolean       do_prune = CPL_FALSE;
    cpl_error_code    error;


    cpl_ensure_code(self   != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(cpl_polynomial_get_dimension(self) == 2,
                    CPL_ERROR_INVALID_TYPE);
    cpl_ensure_code(np > 0,         cpl_error_get_code());
    cpl_ensure_code(values != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(maxdeg != NULL, CPL_ERROR_NULL_INPUT);

    cpl_ensure_code(cpl_vector_get_size_(values) == np,
                    CPL_ERROR_INCOMPATIBLE_INPUT);

    cpl_ensure_code(mindeg[0] >= 0,         CPL_ERROR_ILLEGAL_INPUT);
    cpl_ensure_code(maxdeg[0] >= mindeg[0], CPL_ERROR_ILLEGAL_INPUT);

    if (dimdeg) {

        cpl_ensure_code(mindeg[1] >= 0,         CPL_ERROR_ILLEGAL_INPUT);
        cpl_ensure_code(maxdeg[1] >= mindeg[1], CPL_ERROR_ILLEGAL_INPUT);

        if (mindeg[0] > 0 || mindeg[1] > 0) is_mindeg = CPL_TRUE;

        degree = maxdeg[0] + maxdeg[1];
        ncy = maxdeg[1] + 1;
        nc = (maxdeg[0] + 1 - mindeg[0]) * (ncy - mindeg[1]);

    } else {
        const cpl_size nc0 = mindeg[0] * (mindeg[0] + 1) / 2;

        degree = maxdeg[0];
        ncy = degree + 1;
        nc  = ncy * (maxdeg[0] + 2) / 2 - nc0;

        if (mindeg[0] > 0) is_mindeg = CPL_TRUE;
    }

    if (np < nc) {
        return cpl_error_set_message_(CPL_ERROR_DATA_NOT_FOUND, "Too few data "
                                      "points %d < %d for %d-degree 2D-fit "
                                      "(dimdeg=%d, mindeg=%d)",
                                      (int)np, (int)nc, (int)degree, (int)dimdeg,
                                      (int)mindeg[0]);
    }

    if (self->tree != NULL) {
        /* Reset coefficients, keeping allocations.
           Pruning or emptying needed before return! */
        cpl_polynomial_reset_(self->tree, self->dim - 1);
        self->degree = 0;
        do_prune = CPL_TRUE;
    }

    if (degree == 0) {
        /* Handle this as a special case */

        assert(nc == 1);

        xmean = cpl_vector_get_mean(values);

        if (xmean != 0.0) {

            powers[0] = powers[1] = 0;

            (void)cpl_polynomial_set_coeff(self, powers, xmean);

            assert(self->tree != NULL);

            if (do_prune)
                cpl_polynomial_prune_(self->tree, self->dim - 1, nc);

            assert(self->tree->nc == 1);

        } else {
            cpl_polynomial_empty(self);
        }

        return CPL_ERROR_NONE;
    }

    if (is_mindeg) {
        xmean = ymean = 0.0;
        xhat = yhat = NULL;
        xuse = cpl_bivector_get_x_const(xy_pos);
        yuse = cpl_bivector_get_y_const(xy_pos);
    } else {
        /* Transform: xhat = x - mean(x) */
        xhat = cpl_vector_transform_mean(cpl_bivector_get_x_const(xy_pos),
                                         &xmean);
        assert( xhat != NULL );
        xuse = xhat;

        /* Transform: yhat = y - mean(y) */
        yhat = cpl_vector_transform_mean(cpl_bivector_get_y_const(xy_pos),
                                         &ymean);
        assert( yhat != NULL );
        yuse = yhat;
    }

    dxuse = cpl_vector_get_data_const_(xuse);
    dyuse = cpl_vector_get_data_const_(yuse);

    /* Initialize matrices */
    /* mv contains the polynomial terms in the order described */
    /* above in each row, for each input point. */
    dmv = (double*)cpl_malloc((size_t)(nc * np) * sizeof(*dmv));
    mv = cpl_matrix_wrap(nc, np, dmv);

    /* Has redundant FLOPs, appears to improve accuracy */
    for (i=0; i < np; i++) {
        const double x = dxuse[i];
        const double y = dyuse[i];
        double yvalue = 1.0;
        j = 0;
        for (degy = 0; degy < ncy; degy++) {
            const cpl_size ncx = 1 + (dimdeg ? maxdeg[0] : degree - degy);
            double xvalue = 1.0;
            for (degx = 0; degx < ncx; degx++) {
                if (dimdeg ? (degx >= mindeg[0] && degy >= mindeg[1])
                    : degx + degy >= mindeg[0])
                    dmv[np * j++ + i] = xvalue * yvalue;
                xvalue *= x;
            }
            yvalue *= y;
        }
        assert( j == nc );
    }
    cpl_tools_add_flops( np * (nc * 2 + ncy));

    cpl_vector_delete(xhat);
    cpl_vector_delete(yhat);

    /* mb contains the values, it is _not_ modified */
    CPL_DIAG_PRAGMA_PUSH_IGN(-Wcast-qual);
    mb = cpl_matrix_wrap(np, 1, cpl_vector_get_data_((cpl_vector*)values));
    CPL_DIAG_PRAGMA_POP;

    /* Form the right hand side of the normal equations */
    mx = cpl_matrix_product_create(mv, mb);

    (void)cpl_matrix_unwrap(mb);

    /* Form the matrix of the normal equations */
    mh = cpl_matrix_product_normal_create(mv);
    cpl_matrix_delete(mv);

    /* Solve XA=B by a least-square solution (aka pseudo-inverse). */
    error = cpl_matrix_solve_spd(mh, mx);

    cpl_matrix_delete(mh);

    if (error) {
        cpl_matrix_delete(mx);
        cpl_polynomial_empty(self);
        return cpl_error_set_message_(error, "degree=%d, nc=%d <= np=%d",
                                      (int)degree, (int)nc, (int)np);
    }

    coeffs1d = cpl_matrix_get_data_const(mx);

    /* Check coefficients for anomalies */
    for (j = 0; j < nc; j++) {
        if (isnan(coeffs1d[j])) {
            cpl_matrix_delete(mx);
            cpl_polynomial_empty(self);
            return cpl_error_set_(CPL_ERROR_DIVISION_BY_ZERO);
        }
    }

    /* Store coefficients for output */
    /* - make sure there is enough upper dim space, then fill up from below */
    if (self->tree == NULL) {
        self->tree = cpl_polynomial_new_1d(ncy);
    } else {
        cpl_polynomial_grow_1d(self->tree, ncy);
    }

    j = 0;
    for (degy = 0; degy < ncy; degy++) {
        const cpl_size ncx = 1 + (dimdeg ? maxdeg[0] : degree - degy);
        powers[1] = degy;
        for (degx = 0; degx < ncx; degx++) {
            if (dimdeg ? (degx >= mindeg[0] && degy >= mindeg[1])
                : degx + degy >= mindeg[0]) {
                const double coeff = cpl_matrix_get(mx, j, 0);
                if (coeff != 0.0) {
                    /* Since the tree may contain zero-polynomials,
                       deleting a coefficient (and determining the new
                       degree) can cause an assert() to fail */
                    powers[0] = degx;
                    if (coeffs1d[j] != coeff) break;
                    if (cpl_polynomial_set_coeff(self, powers, coeff)) break;
                }
                j++;
            }
        }
        if (degx < ncx) break;
    }
    cpl_matrix_delete(mx);

    assert(j == nc);

    if (do_prune && cpl_polynomial_prune_(self->tree, self->dim -1, ncy))
        cpl_polynomial_empty(self);

    if (!is_mindeg) {
        /* Transform the polynomial back */
        cpl_polynomial_shift_1d(self, 0, -xmean);
        cpl_polynomial_shift_1d(self, 1, -ymean);
    }

    return CPL_ERROR_NONE;
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Delete (set to zero) a coefficient of a polynomial
  @param    self    The polynomial to modify
  @param    pows    The non-negative power(s) of the variable(s)
  @param    deldeg  The degree of the coefficient to be deleted, sum of pows
  @return   void
  @note  @see cpl_polynomial_set_coeff()

 */
/*----------------------------------------------------------------------------*/
static void cpl_polynomial_delete_coeff(cpl_polynomial * self,
                                        const cpl_size * pows,
                                        cpl_size deldeg)
{
    if (self->tree == NULL) return;

    if (cpl_polynomial_delete_coeff_(self->tree, pows, self->dim - 1)) {
        cpl_polynomial_delete_(self->tree, self->dim - 1);
        self->tree = NULL;
        self->degree = 0;
    } else if (deldeg == self->degree) {
        self->degree = cpl_polynomial_find_degree_(self->tree, self->dim - 1);
        assert(self->degree <= deldeg);
    }
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Fill the Hankel Matrix H=V'*V, where V is a 1D-Vandermonde matrix
  @param    self      The matrix H
  @param    mx        A right multiplication with V', mx = V' * values
  @param    xhat      The mean-transformed x-values
  @param    is_eqdist True iff xhat contains equidistant points
  @param    mindeg    The non-negative minimum fitting degree
  @param    values    The values to be interpolated
  @return   void
  @note self must have its elements initialized to zero iff is_eqdist is true.
  @see cpl_polynomial_fit_1d()

 */
/*----------------------------------------------------------------------------*/
static void cpl_matrix_fill_normal_vandermonde(cpl_matrix * self,
                                               cpl_matrix * mx,
                                               const cpl_vector * xhat,
                                               cpl_boolean is_eqdist,
                                               cpl_size mindeg,
                                               const cpl_vector * values)
{


    const double * dval = cpl_vector_get_data_const_(values);
    const double * xval = cpl_vector_get_data_const_(xhat);
    cpl_vector   * phat = cpl_vector_duplicate(xhat); /* Powers of xhat */
    cpl_vector   * qhat = NULL;                       /* mindeg Power of xhat */
    double       * dhat = cpl_vector_get_data_(phat);
    double       * ehat = NULL;
    const cpl_size nc   = cpl_matrix_get_ncol(self);
    const cpl_size np   = cpl_vector_get_size_(xhat);
    cpl_size       i,j;


    assert( nc == cpl_matrix_get_nrow(self) );
    assert( nc == cpl_matrix_get_nrow(mx) );
    assert( 1  == cpl_matrix_get_ncol(mx) );
    assert( np == cpl_vector_get_size_(values) );


    /* Fill Hankel matrix from top-left to main skew diagonal
       - on and above (non-skew) main diagonal */
    /* Also compute transpose(V) * b */
    /* Peel off 1st iteration */
    if (mindeg > 0) {
        double hsum = 0.0;
        cpl_size k;

        qhat = mindeg == 1 ? cpl_vector_duplicate(xhat) : cpl_vector_new(np);
        ehat = cpl_vector_get_data_(qhat);

        /* Raise xhat to the power of mindeg */
        for (k=0; k < np; k++) {
            const double x = xval[k];

            if (mindeg > 1) ehat[k] = cpl_tools_ipow(x, (int)mindeg);
            dhat[k] *= ehat[k];

            hsum += ehat[k] * ehat[k];
        }
        cpl_matrix_set(self, 0, 0, hsum);
    } else {
        cpl_matrix_set(self, 0, 0, (double)np);
    }
    /* qhat is xhat to the power of mindeg, iff mindeg > 0 */
    /* dhat is xhat to the power of 1+mindeg, iff mindeg > 0 */
    for (j=1; j < 2; j++) {
        double vsum0 = 0.0;
        double hsum = 0.0;
        double vsum = 0.0;
        cpl_size k;

        for (k=0; k < np; k++) {
            const double y = dval[k];

            hsum += mindeg > 0 ? ehat[k] * dhat[k] : dhat[k];
            vsum += y * dhat[k];
            vsum0 += mindeg > 0 ? ehat[k] * y : y;
        }
        cpl_matrix_set(mx, 0, 0, vsum0);
        cpl_matrix_set(mx, j, 0, vsum);
        if (is_eqdist) continue;
        k = j;
        for (i=0; i <= k; i++, k--) {
            cpl_matrix_set(self, i, k, hsum);
        }
    }
    for (; j < nc; j++) {
        double   hsum = 0.0;
        double   vsum = 0.0;
        cpl_size k;

        for (k=0; k < np; k++) {
            const double x = xval[k];
            const double y = dval[k];

            dhat[k] *= x;
            hsum += mindeg > 0 ? ehat[k] * dhat[k] : dhat[k];
            vsum += y * dhat[k];
        }
        cpl_matrix_set(mx, j, 0, vsum);
        if (is_eqdist && (j&1)) continue;
        k = j;
        for (i=0; i <= k; i++, k--) {
            cpl_matrix_set(self, i, k, hsum);
        }
    }
    /* Fill remaining Hankel matrix - on and above (non-skew) main diagonal */

    if (mindeg > 0) cpl_vector_multiply(phat, qhat);
    cpl_vector_delete(qhat);
    for (i = 1; i < nc; i++) {
        cpl_size k;
        double   hsum = 0.0;

        if (is_eqdist && ((i+nc)&1)==0) {
            cpl_vector_multiply(phat, xhat);
            continue;
        }

        for (k=0; k < np; k++) {
            const double x = xval[k];

            dhat[k] *= x;
            hsum += dhat[k];
        }
        k = i;
        for (j = nc-1; k <= j; k++, j--) {
            cpl_matrix_set(self, k, j, hsum);
        }
    }

    cpl_tools_add_flops( 6 * np * ( nc - 1) );

    cpl_vector_delete(phat);

}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Try to detect if the x-points are equidistant
  @param    self  The points to check
  @return   1 if equidistant, 0 if not, -1 on error.
  @see cpl_polynomial_fit_1d_create()
  @deprecated Used only by cpl_polynomial_fit_1d_create()
  @note If yes, then every other skew diagonal of the Hankel matrix is zero,
        H = V' * V, where V is the 1D-Vandermonde matrix from the x-points.

 */
/*----------------------------------------------------------------------------*/
static int cpl_vector_is_eqdist(const cpl_vector * self)
{

    double xval, delta;
    const cpl_size np = cpl_vector_get_size(self);
    cpl_size i;

    cpl_ensure(self, CPL_ERROR_NULL_INPUT, -1);

    if (cpl_vector_get_size_(self) == 1) return 1;

    xval = cpl_vector_get(self, 1);
    delta = xval - cpl_vector_get(self, 0);
    for (i=1; i < np-1; i++) {
        const double dprev = delta;
        const double xprev = xval;

        xval = cpl_vector_get(self, i+1);
        delta = xval - xprev;
        if (delta != dprev) break;
    }

    return i == np-1 ? 1 : 0;

}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Given p and u, modify the non-zero polynomial to p(x) := p(x+u)
  @param    p  The polynomial coefficients to be modified in place
  @param    n  The number of coefficients, _must_ be positive
  @param    u  The shift
  @return   void
  @see      cpl_polynomial_shift_1d
  @note NULL-input and non-positive n causes undefined behaviour

 */
/*----------------------------------------------------------------------------*/
inline void cpl_polynomial_shift_double(double * coeffs, cpl_size n, double u)
{
    for (cpl_size j = 0; j < n-1; j++)
        for (cpl_size i = 1; i < n - j; i++ )
            coeffs[n-1-i] += coeffs[n-i] * u;

    cpl_tools_add_flops( n * ( n - 1) );
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    A real solution to p(x) = 0 using Newton-Raphsons method
  @param    self The 1D-polynomial
  @param    x0   First guess of the solution
  @param    px   The solution, on error see above
  @param    mul  The root multiplicity (or 1 if unknown)
  @param    bpos Iff CPL_TRUE, then fail if a derivative is non-positive
  @return   CPL_ERROR_NONE or the relevant #_cpl_error_code_
  @see cpl_polynomial_solve_1d()

 */
/*----------------------------------------------------------------------------*/
cpl_error_code cpl_polynomial_solve_1d_(const cpl_polynomial * self,
                                        double x0, double * px, cpl_size mul,
                                        cpl_boolean bpos)
{
    cpl_error_code code = CPL_ERROR_NONE;

    cpl_ensure_code(px        != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(self      != NULL, CPL_ERROR_NULL_INPUT);
    cpl_ensure_code(self->dim == 1,    CPL_ERROR_INVALID_TYPE);
    cpl_ensure_code(mul       >  0,    CPL_ERROR_ILLEGAL_INPUT);

    /* Iterating towards zero is not as simple as it sounds, so don't */
    if (self->tree == NULL) {
       *px = 0.0;
    } else if (self->tree->coef[0].val != 0.0) {
        code = cpl_polynomial_solve_1d__(self->tree, x0, px, mul, bpos);
    } else {
       *px = 0.0;
    }

    return code ? cpl_error_set_where_() : CPL_ERROR_NONE;
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    A real solution to p(x) = 0 using Newton-Raphsons method
  @param    p    The 1D-polynomial
  @param    x0   First guess of the solution
  @param    px   The solution, on error see above
  @param    mul  The root multiplicity (or 1 if unknown)
  @param    bpos Iff CPL_TRUE, then fail if a derivative is non-positive
  @return   CPL_ERROR_NONE or the relevant #_cpl_error_code_
  @see cpl_polynomial_solve_1d_()

  Setting bpos is useful for avoiding a non-physical solution when the solution
  needs to be found in an interval where the polynomial is strictly growing,
  e.g. a dispersion relation.

 */
/*----------------------------------------------------------------------------*/
static cpl_error_code cpl_polynomial_solve_1d__(const cpl_polynomial_1d * p,
                                                double x0, double * px,
                                                cpl_size mul,
                                                cpl_boolean bpos)
{

    double         x    = x0;
    /* Initialize to ensure at least one iteration */
    double         r    = 1.0;
    double         d    = 0.0;
    const double   dmul = (double)mul; /* Root multiplicity */
    const cpl_size mite = p->nc * (CPL_POLYNOMIAL_SOLVE_MAXITE);
    cpl_size       i;


    for (i = 0; i < mite; i++) {
        const double xprev = x;
        const double rprev = r;
        const double dprev = d;

        /* Compute residual, r = p(x) and derivative, d = p`(x) */
        r = cpl_polynomial_eval_1d_(p, x, &d);

        /* Stop if:
           0) If bpos is true and d <= 0.0. This indicates a failure to solve
           1) Correction did not decrease - unless p`(x) changed sign
           2) p`(x) == 0. It is insufficient to implement this as d == 0,
              because some non-zero divisors can still result in inf. */
        if (bpos && d <= 0.0) break;
        if (d * dprev >= 0.0 && fabs(r * dprev) >= fabs(rprev * d)) break;

        /* Compute and apply the accelerated Newton-Raphson correction */
        x -= dmul * r / d;

        /* x can become NaN - in which case the iteration goes on
           until the maximum number of iterations is reached.
           In one case this happens because a p'(x) == 0 is pertubed by
           round-off. */

        /* Stop also if:
           3) The correction did not change the solution
              - will typically save at most one Horner-evaluation  */
        if (fabs(x - xprev) < fabs(x) * DBL_EPSILON) break;

        /* if x_i == x_j for i > j > 0 the iteration cannot converge.
           This can only happen with at least two sign-changes for p`(x_k)
           i >= k >= j. This is not checked for. */

    }

    *px = x;

    cpl_tools_add_flops( i * 12 );

    if (i == mite) return
        cpl_error_set_message_(CPL_ERROR_CONTINUE, "x0=%g, mul=%d, degree=%d, "
                               "i=%d, p(x=%g)=%g", x0, (int)mul, (int)p->nc-1,
                               (int)i, x,
                               (double)cpl_polynomial_eval_1d_(p, x, NULL));

    /* At this point:
       In absence of rounding r or d is zero.
       Due to rounding r or d is zero or close to zero.
       If there is no solution only d is (close to) zero.
       If there is a single solution only r is (close to) zero.
       If there is a multiple solution both r and d are (close to) zero
       - in this case |r| cannot be bigger than |d| because p is one
       degree higher than p'. */

    if (bpos && d <= 0.0) return
        cpl_error_set_message_(CPL_ERROR_ILLEGAL_INPUT, "x0=%g, mul=%d, "
                               "degree=%d, i=%d, p(x=%g)=%g, p'(x)=%g <= 0.0",
                               x0, (int)mul, (int)p->nc-1, (int)i, x, r, d);

    if (fabs(r) > fabs(d)) {
        /* When d is computed in long double precision at a multiple root,
           |r| _can_ be bigger than |d|.

           Quick fix: Assume that solution is still OK, if |r| is small
           compared to the largest coefficient */

        /* Since r is non-zero, at least one coefficient must be non-zero */
        cpl_size n = p->nc;
        double max = 0.0;
        while (n) if (fabs(p->coef[--n].val) > max) max = fabs(p->coef[n].val);

        if (fabs(r) > max * DBL_EPSILON) return
            cpl_error_set_message_(CPL_ERROR_DIVISION_BY_ZERO, "x0=%g, mul=%d, "
                                   "degree=%d, i=%d, p(x=%g)=%g, p'(x)=%g", x0,
                                   (int)mul, (int)p->nc-1, (int)i, x, r, d);
    }

    return CPL_ERROR_NONE;
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Create a new, zero-valued polynomial with memory for coefficients
  @param    nc   The positive number of coefficients
  @return   1 newly allocated polynomial
  @note No error checking in this internal function

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_ILLEGAL_INPUT if dim is negative or zero
 */
/*----------------------------------------------------------------------------*/
inline static cpl_polynomial_1d * cpl_polynomial_new_1d(cpl_size nc)
{
    const cpl_size nz = nc > CPL_POLYNOMIAL_DEFAULT_COEFFS ? nc : 
        CPL_POLYNOMIAL_DEFAULT_COEFFS;

    cpl_polynomial_1d * self;

    /* Allocate struct - and within the same allocation also space for the
       requested number of coefficients (and at least the default number) */
    self       = cpl_malloc(sizeof(*self)
                            + (size_t)nz * sizeof(*(self->coef)));
    self->coef = (void*)(self + 1);

    assert(!CPL_POLYNOMIAL_COEFF_IS_MALLOC(self));
    assert(nc > 0);

    self->nc = 0;
    self->nz = nz;

    return self;
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Grow (double) the length of the 1D array
  @param    self     Polynomial to modify
  @parma    newsize  New size, zero to double old
  @return   void
  @note Requests to shrink are ignored
  @see cpl_polynomial_new_1d()

 */
/*----------------------------------------------------------------------------*/
inline static
void cpl_polynomial_grow_1d(cpl_polynomial_1d * self, cpl_size newsize)
{
    if (newsize == 0 || newsize > self->nz) {
        /* No shrinking, only growing */
        const cpl_size oldsize = self->nz;

        self->nz = newsize ? newsize : 2 * self->nz;

        if (self->nc == 0) {
            /* No memcpy(), no overhead */
            if (CPL_POLYNOMIAL_COEFF_IS_MALLOC(self))
                cpl_free((void*)self->coef);
            self->coef = cpl_malloc((size_t)self->nz * sizeof(*(self->coef)));
        } else if (!CPL_POLYNOMIAL_COEFF_IS_MALLOC(self)) {
            /* Copy (possibly full), but no deallocation */
            self->coef = memcpy(cpl_malloc((size_t)self->nz
                                           * sizeof(*(self->coef))),
                                self->coef,
                                (size_t)self->nc * sizeof(*(self->coef)));
        } else if (self->nc < oldsize) {
            /* Partial memcpy(), allocation overhead */
            void * oldbuf = self->coef;
            self->coef = cpl_malloc((size_t)self->nz * sizeof(*(self->coef)));
            (void)memcpy(self->coef, oldbuf,
                         (size_t)self->nc * sizeof(*(self->coef)));
            cpl_free(oldbuf);
        } else {
            /* Full memcpy(), allocation overhead handled by realloc() */
            self->coef = cpl_realloc(self->coef, (size_t)self->nz
                                     * sizeof(*(self->coef)));
        }
    }
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Grow (double) the length of the 1D array, reset new elements
  @param  self    Polynomial to modify
  @parma  newsize New size, zero to double old
  @parma  zeroto  Index of the last element not to reset, at least nc, 0 for all
  @return void
  @see cpl_polynomial_grow_1d()

 */
/*----------------------------------------------------------------------------*/
inline static void cpl_polynomial_grow_1d_reset(cpl_polynomial_1d * self,
                                                cpl_size newsize,
                                                cpl_size zeroto)
{

    cpl_polynomial_grow_1d(self, newsize);

    if (zeroto == 0) zeroto = self->nz;

    assert(self->nz >  self->nc);
    assert(self->nz >= zeroto);
    assert(zeroto   >= self->nc);

    (void)memset(self->coef + (size_t)self->nc, 0,
                 (size_t)(zeroto - self->nc) * sizeof(*self->coef));
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Delete and deallocate all memory of a polynomial
  @param    self   Polynomial to delete
  @param    mydim  This dimension, zero for deepest
  @return   void
  @see cpl_polynomial_delete_()

 */
/*----------------------------------------------------------------------------*/
inline static
void cpl_polynomial_delete_(cpl_polynomial_1d * self, cpl_size mydim)
{
    if (self != NULL) {
        if (mydim > 0) {
            if (self->nc > 0) { /* Could be a zero-polynomial from a collapse */
                assert(self->coef != NULL);
                do {
                    self->nc--;
                    cpl_polynomial_delete_(self->coef[self->nc].next, mydim - 1);
                } while (self->nc);
            }
            assert(self->nc == 0);
        }
        if (CPL_POLYNOMIAL_COEFF_IS_MALLOC(self))
            cpl_free((void*)self->coef);
        cpl_free((void*)self);
    }
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Set to zero all coefficients, keeping all nodes allocated
  @param    self   Polynomial to modify
  @param    mydim  This dimension, zero for deepest
  @return   void
  @note Tree of arrays is not deallocated, just emptied
  @see cpl_polynomial_reset_()

  This function only modifies the lowest dimension, where all coefficient
  counters are set to zero. The resulting polynomial is not valid for return
  to the user, but may be repopulated without reallocating just reset nodes.

  Unless a priori knowledge guarantees that each node is repopulated, the
  pruning function must be called before the polynomial is returned via the
  public API.

 */
/*----------------------------------------------------------------------------*/
static void cpl_polynomial_reset_(cpl_polynomial_1d * self, cpl_size mydim)
{
    if (self != NULL) {
        if (mydim > 0) {
            for (cpl_size i = 0; i < self->nc; i++) {
                cpl_polynomial_reset_(self->coef[i].next, mydim - 1);
            }
        } else {
            self->nc = 0;
        }
    }
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Ensure leading coefficient(s) are non-zero recursively, set size
  @param    self     Polynomial to modify
  @param    mydim    This dimension, zero for deepest
  @parma    newsize  New size, zero to empty completely
  @return   CPL_TRUE iff there is an underlying dimension to be deallocated
  @note The new size may not exceed the allocated size!

 */
/*----------------------------------------------------------------------------*/
inline static cpl_boolean cpl_polynomial_prune_(cpl_polynomial_1d * self,
                                                cpl_size mydim,
                                                cpl_size newsize)
{
    cpl_boolean do_del = CPL_FALSE;

    if (self != NULL) {
        cpl_size i;

        if (mydim > 0) {
            /* If new size exceeds old size: */
            /* Delete zero-valued coefficients */
            for (i = self->nc; i > newsize; i--) {
                cpl_polynomial_delete_(self->coef[i-1].next, mydim - 1);
            }
        }

        i = newsize;

        /* Delete zero-valued leading coefficients */
        if (mydim == 0) {
            for (; i > 0; i--) {
                if (self->coef[i-1].val != 0.0) break;
            }
        } else {
            for (; i > 0; i--) {

                if (self->coef[i-1].next == NULL) continue;

                if (self->coef[i-1].next->nc == 0 ||
                    cpl_polynomial_prune_(self->coef[i-1].next,
                                          mydim - 1,
                                          self->coef[i-1].next->nc)) {
                    cpl_polynomial_delete_(self->coef[i-1].next, mydim - 1);
                    self->coef[i-1].next = NULL;
                    continue;
                }

                break;
            }
        }

        self->nc = i; /* Adjust degree of self */

        if (mydim > 0) {
            /* Deallocate remaining, empty nodes */
            for (; i > 0; i--) {
                if (self->coef[i-1].next == NULL) continue;

                if (self->coef[i-1].next->nc == 0 ||
                    cpl_polynomial_prune_(self->coef[i-1].next,
                                          mydim - 1,
                                          self->coef[i-1].next->nc)) {
                    cpl_polynomial_delete_(self->coef[i-1].next, mydim - 1);
                    self->coef[i-1].next = NULL;
                }
            }

        }
        do_del = self->nc == 0 ? CPL_TRUE : CPL_FALSE;
    }
    return do_del;
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Delete (set to zero) a coefficient of a polynomial
  @param    self   The polynomial to modify
  @param    pows   The non-negative power(s) of the variable(s)
  @param    mydim  This dimension, zero for deepest
  @return   CPL_TRUE iff there is an underlying dimension to be deallocated
  @see cpl_polynomial_delete_coeff()

 */
/*----------------------------------------------------------------------------*/
inline static cpl_boolean cpl_polynomial_delete_coeff_(cpl_polynomial_1d * self,
                                                       const cpl_size * pows,
                                                       cpl_size mydim)
{
    cpl_boolean do_del = CPL_FALSE;


    assert(mydim       >= 0);
    assert(pows[mydim] >= 0);

    if (pows[mydim] < self->nc) {

        if (mydim == 0) {
            self->coef[pows[mydim]].val = 0.0;
        } else if (self->coef[pows[mydim]].next != NULL &&
                   cpl_polynomial_delete_coeff_(self->coef[pows[mydim]].next,
                                                pows,
                                                mydim - 1)) {
            cpl_polynomial_delete_(self->coef[pows[mydim]].next, mydim - 1);
            self->coef[pows[mydim]].next = NULL;
        }

        if (pows[mydim] == self->nc-1) {
            /* Leading coefficient is zero - find the new leading */

            if (cpl_polynomial_prune_(self, mydim, self->nc)) {
                do_del = CPL_TRUE;
            }
        }
    }

    return do_del;
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Set a non-zero coefficient of the polynomial
  @param    self   The polynomial to modify
  @param    pows   The non-negative power(s) of the variable(s)
  @param    mydim  This dimension, zero for deepest
  @param    value  The new, non-zero coefficient
  @return   void
  @see cpl_polynomial_set_coeff()
 */
/*----------------------------------------------------------------------------*/
inline static void cpl_polynomial_set_coeff_(cpl_polynomial_1d * self,
                                             const cpl_size * pows,
                                             cpl_size mydim,
                                             double value)
{
    assert(mydim       >= 0);
    assert(pows[mydim] >= 0);
    assert(value != 0.0);

    if (pows[mydim] >= self->nc) {
        if (pows[mydim] >= self->nz) {
            cpl_polynomial_grow_1d(self, 2 * pows[mydim]);
        }

        /* Reset coefficients above existing up to new one  */
        /* ... setting the last one to zero is redundant when mydim == 0 */
        (void)memset(self->coef + (size_t)self->nc, 0,
                     (size_t)(1 + pows[mydim] - self->nc) *
                     sizeof(*self->coef));

        self->nc = 1 + pows[mydim];
    }

    if (mydim == 0) {
        self->coef[pows[mydim]].val = value;
    } else {
        assert(mydim > 0);

        if (self->coef[pows[mydim]].next == NULL)
            self->coef[pows[mydim]].next =
                cpl_polynomial_new_1d(1 + pows[mydim - 1]);

        cpl_polynomial_set_coeff_(self->coef[pows[mydim]].next,
                                  pows, mydim - 1, value);
    }
}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    The degree of the polynomial
  @param    self    The polynomial to access
  @param    mydim  This dimension, zero for deepest
  @return   The degree, possibly zero
  @see cpl_polynomial_get_degree()
 */
/*----------------------------------------------------------------------------*/
static cpl_size cpl_polynomial_find_degree_(const cpl_polynomial_1d * self,
                                            cpl_size mydim)
{
    cpl_size mydegree = self->nc - 1;

    assert(self->nc > 0);

    if (mydim > 0) {
        for (cpl_size i = 0; i < self->nc; i++) {
            if (self->coef[i].next != NULL) {
                const cpl_size idegree = i +
                    cpl_polynomial_find_degree_(self->coef[i].next, mydim - 1);
                if (idegree > mydegree) mydegree = idegree;
            }
        }
    }

    return mydegree;
}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Copy one polynomial to another
  @param    self     The polynomial to modify
  @param    selfdim  The dimension of self
  @param    other    The non-NULL polynomial to process
  @param    otherdim The dimension of other
  @return   void
  @see cpl_polynomial_copy()
 */
/*----------------------------------------------------------------------------*/
static void cpl_polynomial_copy_(cpl_polynomial_1d * self,
                                 cpl_size selfdim,
                                 const cpl_polynomial_1d * other,
                                 cpl_size otherdim)
{

    if (otherdim == 0) {
        if (selfdim > 0) {
            while (self->nc) {
                self->nc--;
                cpl_polynomial_delete_(self->coef[self->nc].next, selfdim - 1);
            };
        }
        cpl_polynomial_grow_1d(self, other->nc);
        (void)memcpy((void*)self->coef, (const void*)other->coef,
                     (size_t)other->nc * sizeof(*self->coef));
    } else {
        /* Nothing to keep */
        if (selfdim == 0) {
            self->nc = 0;
        } else {
            /* - except reusable lower dimension storage structure */
            while (self->nc > other->nc) {
                self->nc--;
                cpl_polynomial_delete_(self->coef[self->nc].next, selfdim - 1);
            }
        }

        cpl_polynomial_grow_1d(self, other->nc);

        selfdim > 0
            ? assert(self->nc <= other->nc)
            : assert(self->nc == 0);

        for (cpl_size i = 0; i < other->nc; i++) {
            if (other->coef[i].next != NULL) {
                if (i >= self->nc || self->coef[i].next == NULL)
                    self->coef[i].next =
                        cpl_polynomial_new_1d(other->coef[i].next->nc);
                cpl_polynomial_copy_(self->coef[i].next, selfdim - 1,
                                     other->coef[i].next, otherdim - 1);
            } else {
                if (selfdim > 0 && i < self->nc)
                    cpl_polynomial_delete_(self->coef[i].next, selfdim - 1);
                self->coef[i].next = NULL;
            }
        }
    }
    self->nc = other->nc;
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Compare the coefficients of two polynomials
  @param    self   The 1st polynomial or NULL
  @param    other  The 2nd polynomial or NULL
  @param    tol  The absolute (non-negative) tolerance
  @return   0 when equal, positive when different, negative on error
  @note  If the degrees differ the leading coefficients must be smaller than tol
  @see cpl_polynomial_compare()
 */
/*----------------------------------------------------------------------------*/
static int cpl_polynomial_compare_(const cpl_polynomial_1d * self,
                                   const cpl_polynomial_1d * other,
                                   cpl_size mydim,
                                   double tol)
{

    if (self != NULL || other != NULL) {
        const cpl_size nself  = self  == NULL ? 0 : self->nc;
        const cpl_size nother = other == NULL ? 0 : other->nc;
        const cpl_size nmin   = CPL_MIN(nself, nother);
        cpl_size i = 0;

        if (mydim == 0) {
            for (; i < nmin; i++)
                if (fabs(self->coef[i].val - other->coef[i].val) > tol) return i+1;

            for (; i < nother; i++)
                if (fabs(other->coef[i].val) > tol) return i + 1;

            for (; i < nself; i++)
                if (fabs(self->coef[i].val ) > tol) return i + 1;
        } else {
            int cmp = 0;

            for (; i < nmin; i++) {
                cmp = cpl_polynomial_compare_(self->coef[i].next,
                                              other->coef[i].next,
                                              mydim - 1, tol);
                if (cmp) return i + cmp;
            }

            for (; i < nother; i++) {
                cmp = cpl_polynomial_compare_(NULL,
                                              other->coef[i].next,
                                              mydim - 1, tol);
                if (cmp) return i + cmp;
            }
            for (; i < nself; i++) {
                cmp = cpl_polynomial_compare_(self->coef[i].next,
                                              NULL,
                                              mydim - 1, tol);
                if (cmp) return i + cmp;
            }
        }
    }

    return 0;
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Compute a first order partial derivative
  @param    self  The polynomial to be modified in place
  @param    mydim This dimension, zero for deepest
  @param    dodim The dimension to differentiate, zero for first dimension
  @return   CPL_TRUE iff the operation collapsed the dimension to zero
  @see cpl_polynomial_derivative()
 */
/*----------------------------------------------------------------------------*/
static cpl_boolean cpl_polynomial_derivative_(cpl_polynomial_1d * self,
                                              cpl_size mydim,
                                              cpl_size dodim)
{
    if (mydim > dodim) {
        cpl_size nc = 0;

        for (cpl_size i = 0; i < self->nc; i++) {
            if (self->coef[i].next != NULL) {
                if (cpl_polynomial_derivative_(self->coef[i].next, mydim - 1,
                                               dodim)) {
                    /* Underlying dimension(s) collapsed */
                    cpl_polynomial_delete_(self->coef[i].next, mydim - 1);
                    self->coef[i].next = NULL;
                } else {
                    nc = i + 1; /* Remember highest non-zero, i.e. new lead */
                }
            }
        }

        /* Zero-valued leading coefficients have already been deleted */
        self->nc = nc;
    } else if (dodim > 0) {

        assert(dodim    == mydim);
        assert(self->nc >  0);

        /* Constant term goes away */
        cpl_polynomial_delete_(self->coef[0].next, mydim - 1);

        self->nc--;

        for (cpl_size i = 0; i < self->nc; i++) {
            self->coef[i].next = self->coef[i + 1].next; /* Decrement power */

            cpl_polynomial_multiply_scalar_self(self->coef[i].next,
                                                mydim - 1, (double)(i+1));
        }
    } else {
        assert(dodim    == 0);
        assert(mydim    == 0);
        assert(self->nc >  0);

        /* Constant term goes away */
        self->nc--;

        for (cpl_size i = 0; i < self->nc; i++) {
            self->coef[i].val = (double)(i+1) * self->coef[i + 1].val;
        }

        cpl_tools_add_flops( self->nc );
    }

    return self->nc == 0 ? CPL_TRUE : CPL_FALSE;
}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief   Dump one coefficient as ASCII to a stream
  @param   pows   The non-negative power(s) of the variable(s), or NULL for zero
  @param   dim    The positive polynomial dimension (number of variables)
  @param   stream Output stream eq. @c stdout or @c stderr
  @param   value  The coefficient
  @return  CPL_ERROR_NONE or the relevant #_cpl_error_code_
  @note    pows not accessed (and zeros used instead) on zero valued coefficient
  @see cpl_polynomial_dump()

 */
/*----------------------------------------------------------------------------*/
static cpl_error_code cpl_polynomial_dump_coeff(const cpl_size * pows,
                                                cpl_size         dim,
                                                FILE           * stream,
                                                double           value)
{
    int retval;

    for (cpl_size i = 0; i < dim; i++) {
        retval = fprintf(stream, "  %5" CPL_SIZE_FORMAT "      ",
                         pows != NULL ? pows[i] : 0);
        cpl_ensure_code(retval > 0, CPL_ERROR_FILE_IO);
    }

    retval = fprintf(stream, "%g\n", value);
    cpl_ensure_code(retval > 0, CPL_ERROR_FILE_IO);

    return CPL_ERROR_NONE;
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief Swap the given dimension with the lowest one
  @param self    The (empty) polynomial to hold the result
  @param mypow   The power of this dimension, aliased with pows + mydim
  @param mydim   This dimension, zero for deepest
  @param pows    The non-negative power(s) of the variable(s)
  @param dim     The positive polynomial dimension (number of variables)
  @param other   The polynomial to read from
  @param dodim   The positive polynomial dimension to swap into zero (not 1!)
  @return void

 */
/*----------------------------------------------------------------------------*/
inline static void cpl_polynomial_transpose_(cpl_polynomial_1d       * self,
                                             cpl_size                * mypow,
                                             cpl_size                  mydim,
                                             const cpl_size          * pows,
                                             cpl_size                  dim,
                                             const cpl_polynomial_1d * other,
                                             cpl_size                  dodim)
{

    if (mydim == 0) {
        for (cpl_size i = 0; i < other->nc; i++) {
            const double value = other->coef[i].val;
            if (value != 0.0) {
                mypow[dodim] = i;
                (void)cpl_polynomial_set_coeff_(self, pows, dim - 1, value);
            }
        }
    } else {
        for (cpl_size i = 0; i < other->nc; i++) {
            if (other->coef[i].next == NULL) continue;
            /* Write the dimension to swap into zero'th place */
            *(mydim == dodim ? mypow - mydim : mypow) = i;

            cpl_polynomial_transpose_(self, mypow - 1, mydim - 1, pows,
                                      dim, other->coef[i].next, dodim);
        }
    }
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief Dump a polynomial as ASCII to a stream, fail on zero-polynomial(s)
  @param self   The polynomial to process
  @param mypow  The power of this dimension, aliased with pows + mydim
  @param mydim  This dimension, zero for deepest
  @param pows   The non-negative power(s) of the variable(s)
  @param dim    The positive polynomial dimension (number of variables)
  @param ncoeff Counter of non-zero coefficients
  @param stream Output stream eq. @c stdout or @c stderr
  @return CPL_ERROR_NONE or the relevant #_cpl_error_code_
  @note Since this function is mostly for testing, it checks for illegal
        sub-zero-polynomials and sets a CPL error if one is found
  @see cpl_polynomial_dump()

 */
/*----------------------------------------------------------------------------*/
static cpl_error_code cpl_polynomial_dump_(const cpl_polynomial_1d * self,
                                           cpl_size                * mypow,
                                           cpl_size                  mydim,
                                           const cpl_size          * pows,
                                           cpl_size                  dim,
                                           cpl_size                * ncoeff,
                                           FILE                    * stream)
{

    if (self->nc == 0) {
        const cpl_size edim = dim - mydim - 1;
        if (mydim < dim) {
            *mypow = 0;
            if (cpl_polynomial_dump_coeff(pows + mydim, dim - mydim, stream, 0.0))
                return cpl_error_set_where_();
        }
        return cpl_error_set_message_(CPL_ERROR_ILLEGAL_INPUT, "Polynomial of "
                                      "dimension %d contains a zero-valued %d-"
                                      "dimensional polynomial in dimension %d",
                                      (int)dim, (int)edim, (int)mydim+ 1);
    } else if (mydim == 0) {
        for (cpl_size i = 0; i < self->nc; i++) {
            const double   value = self->coef[i].val;
            cpl_boolean    doit  = CPL_FALSE;
            cpl_error_code code  = CPL_ERROR_NONE;

            *mypow = i;
            assert(pows[mydim] == i);

            if (value != 0.0) {
                ++*ncoeff;
                doit = CPL_TRUE;
            } else if (i + 1 == self->nc) {
                /* Leading must be non-zero */
                doit = CPL_TRUE;
                code = cpl_error_set_message_(CPL_ERROR_ILLEGAL_INPUT,
                                              "Polynomial of dimension %d "
                                              "contains a zero-valued leading "
                                              "coefficient at power %d in the "
                                              "lowest dimension", (int)dim,
                                              (int)i);
            }
            if (doit && cpl_polynomial_dump_coeff(pows, dim, stream, value))
                return cpl_error_set_where_();
            if (code) return code;
        }
    } else {
        for (cpl_size i = 0; i < self->nc; i++) {

            *mypow = i;
            assert(pows[mydim] == i);

            if (self->coef[i].next == NULL) {
                /* Leading must be non-zero */
                if (i + 1 == self->nc) {
                    const cpl_error_code code =
                        cpl_error_set_message_(CPL_ERROR_ILLEGAL_INPUT,
                                               "Polynomial of dimension %d "
                                               "contains a zero-valued leading "
                                               "coefficient at power %d in "
                                               "dimension %d", (int)dim,
                                               (int)i, (int)mydim + 1);

                    if (cpl_polynomial_dump_coeff(pows + mydim, dim - mydim,
                                                  stream, 0.0))
                        return cpl_error_set_where_();
                    return code;
                }
            } else if (cpl_polynomial_dump_(self->coef[i].next,
                                            mypow - 1, mydim - 1,
                                            pows, dim, ncoeff, stream)) {
                return cpl_error_set_where_();
            }
        }
    }

    return CPL_ERROR_NONE;
}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Get a coefficient of the polynomial
  @param    self    The polynomial to modify
  @param    pows    The non-negative power(s) of the variable(s)
  @param    mydim  This dimension, zero for deepest
  @return   The coefficient, or zero for not found
  @note Input with NULL/member nc == 0 means zero-polynomial
  @see cpl_polynomial_get_coeff

 */
/*----------------------------------------------------------------------------*/
inline static double cpl_polynomial_get_coeff_(const cpl_polynomial_1d * self,
                                               const cpl_size          * pows,
                                               cpl_size                  mydim)
{
    double value = 0.0;

    assert(mydim       >= 0);
    assert(pows[mydim] >= 0);

    if (self != NULL && pows[mydim] < self->nc) {

        value = mydim == 0
            ? self->coef[pows[mydim]].val
            : cpl_polynomial_get_coeff_(self->coef[pows[mydim]].next,
                                        pows, mydim - 1);
    }

    return value;
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Nested Horner algorithm for evaluation with gradient
  @param    self  The non-empty 1D-polynomial to evaluate
  @param    a     The first evaluation point
  @param    b     The second evaluation point
  @param    ppa   On return set to p(a)
  @return   The gradient between p(a) and p(b), when a == b, p'(a)
  @see cpl_polynomial_eval_1d_()

 */
/*----------------------------------------------------------------------------*/
inline static
cpl_long_double cpl_polynomial_nested_horner(const cpl_polynomial_1d * self,
                                             double a,
                                             double b,
                                             cpl_long_double * ppa)
{
    cpl_size        i  = self->nc - 1;
    cpl_long_double pa = self->coef[i].val;
    cpl_long_double pb = pa;

    cpl_tools_add_flops( 4 * i );

    while (i > 1) {
        pa = pa * a + self->coef[--i].val;
        pb = pb * b + pa;
    }

    *ppa = pa * a + self->coef[0].val;

    return pb;
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Evaluate a univariate (1D) polynomial using Horners rule.
  @param    self  The 1D-polynomial
  @param    x     The point of evaluation
  @param    pd    Iff pd is non-NULL, the derivative evaluated at x
  @return   The result or undefined on error.
  @see cpl_polynomial_eval_1d()

 */
/*----------------------------------------------------------------------------*/
inline static
cpl_long_double cpl_polynomial_eval_1d_(const cpl_polynomial_1d * self,
                                        double x,
                                        double * pd)
{
    cpl_long_double result;

    assert(self->nc > 0);

    if (pd == NULL) {
        cpl_size i = self->nc - 1;

        result = self->coef[i].val;

        cpl_tools_add_flops( 2 * i );

        while (i) result = x * result + self->coef[--i].val;

    } else if (self->nc > 1) {
        *pd = (double)cpl_polynomial_nested_horner(self, x, x, &result);
    } else {
        result = self->coef[0].val;
        *pd = 0.0;
    }

    return result;
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Evaluate the polynomial at the given point, using Horner's rule
  @param    self   The polynomial to access
  @param    mydim  This dimension, zero for deepest
  @param    px     Point of evaluation, lowest dimension variable is at px[0]
  @return   The computed value or undefined on error.
  @see cpl_polynomial_eval()
  @note Input with NULL/member nc == 0 means zero-polynomial

 */
/*----------------------------------------------------------------------------*/
inline static
cpl_long_double cpl_polynomial_eval_(const cpl_polynomial_1d * self,
                                     cpl_size                  mydim,
                                     const double            * px)
{
    cpl_long_double result = 0.0;

    if (self != NULL && self->nc > 0) {

        if (mydim == 0) {
            result = cpl_polynomial_eval_1d_(self, *px, NULL);
        } else {
            cpl_size i = self->nc - 1;

            assert(self->coef[i].next != NULL);

            result = cpl_polynomial_eval_(self->coef[i].next, mydim - 1, px);

            cpl_tools_add_flops(2 * i);

            while (i) {
                result = px[mydim] * result +
                    cpl_polynomial_eval_(self->coef[--i].next, mydim - 1, px);
            }
        }
    }

    return result;
}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Evaluate the polynomial at the given point one monomial at a time
  @param  self  The polynomial to access
  @param  x     Point of evaluation
  @note   The length of x must match the polynomial dimension.
  @return The computed value or undefined on error.
  @see cpl_polynomial_eval()
  @note This function only exists to benchmark the Horner version

 */
/*----------------------------------------------------------------------------*/
double cpl_polynomial_eval_monomial(const cpl_polynomial * self,
                                    const cpl_vector * x)
{
    cpl_long_double result = 0.0;

    cpl_ensure(self != NULL, CPL_ERROR_NULL_INPUT, -1.0);
    cpl_ensure(x    != NULL, CPL_ERROR_NULL_INPUT, -2.0);
    cpl_ensure(self->dim == cpl_vector_get_size_(x),
               CPL_ERROR_INCOMPATIBLE_INPUT, -3.0);

    if (self->tree != NULL) {
        cpl_size * pows = cpl_malloc((size_t)self->dim * sizeof(*pows));

        cpl_polynomial_eval_monomial_(self->tree, pows + self->dim - 1,
                                      self->dim - 1, pows, self->dim, 
                                      cpl_vector_get_data_const_(x), &result);
        cpl_free(pows);
    }

    return result;
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief Evaluate the polynomial at the given point one monomial at a time
  @param self    The polynomial to access
  @param mypow   The power of this dimension, aliased with pows + mydim
  @param mydim   This dimension, zero for deepest
  @param pows    The non-negative power(s) of the variable(s)
  @param dim     The positive polynomial dimension (number of variables)
  @param px      Point of evaluation
  @param presult The accumulated result
  @return void
  @see cpl_polynomial_eval_monomial()
  @note This function only exists to benchmark the Horner version

 */
/*----------------------------------------------------------------------------*/
static void cpl_polynomial_eval_monomial_(const cpl_polynomial_1d * self,
                                          cpl_size                * mypow,
                                          cpl_size                  mydim,
                                          const cpl_size          * pows,
                                          cpl_size                  dim,
                                          const double            * px,
                                          cpl_long_double         * presult)
{
    if (self != NULL && self->nc > 0) {
        if (mydim == 0) {
            for (cpl_size i = 0; i < self->nc; i++) {
                cpl_long_double monomial = self->coef[i].val;
                if (monomial != 0.0) {
                    *mypow = i;
                    assert(pows[mydim] == i);

                    for (cpl_size j = 0; j < dim; j++) {
                        monomial *= cpl_tools_ipow(px[j], pows[j]);
                    }
                    *presult += monomial;
                }
            }
            cpl_tools_add_flops(self->nc * (1 + dim)); /* Approximate */
        } else {
            for (cpl_size i = 0; i < self->nc; i++) {

                *mypow = i;
                assert(pows[mydim] == i);

                cpl_polynomial_eval_monomial_(self->coef[i].next,
                                              mypow - 1, mydim - 1,
                                              pows, dim, px, presult);
            }
        }
    }
}
/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Add two polynomials of the same dimension
  @param    self   The polynomial to hold the result
  @param    first  The 1st polynomial to add, or NULL
  @param    second The 2nd polynomial to add, or NULL
  @param    mydim  This dimension, zero for deepest
  @return   CPL_TRUE iff the operation collapsed the dimension to zero
  @note self may be passed also as first and/or second
  @see cpl_polynomial_add()
  @note Input with NULL/member nc == 0 means zero-polynomial
 */
/*----------------------------------------------------------------------------*/
static cpl_boolean cpl_polynomial_add_(cpl_polynomial_1d       * self,
                                       const cpl_polynomial_1d * first,
                                       const cpl_polynomial_1d * second,
                                       cpl_size                  mydim)
{
    cpl_boolean do_del = CPL_FALSE;

    if (first != NULL || second != NULL) {
        const cpl_size nfirst  = first  == NULL ? 0 : first->nc;
        const cpl_size nsecond = second == NULL ? 0 : second->nc;
        const cpl_size nmin    = CPL_MIN(nfirst, nsecond);
        const cpl_size nmax    = CPL_MAX(nfirst, nsecond);
        cpl_size i = 0;

        if (mydim == 0) {

            if (self->nc < nmax) {
                cpl_polynomial_grow_1d(self, nmax);
            }

            for (; i < nmin; i++)
                self->coef[i].val = first->coef[i].val + second->coef[i].val;

            /* One operand is zero, so just copy the other */
            if (i < nsecond) {
                if (self->coef != second->coef)
                    (void)memcpy(self->coef + i, second->coef + i,
                             (size_t)(nsecond - i) * sizeof(*self->coef));
                i = nsecond;
                assert(i == second->nc);
            } else if (i < nfirst) {
                if (self->coef != first->coef)
                    (void)memcpy(self->coef + i, first->coef + i,
                                 (size_t)(nfirst - i) * sizeof(*self->coef));
                i = nfirst;
                assert(i == first->nc);
            }

            cpl_tools_add_flops( nmin );
        } else {

            if (self->nc < nmax) {
                /* Need to reset new elements to NULL */
                cpl_polynomial_grow_1d_reset(self, nmax, 0);
            }

            for (; i < nmin; i++) {
                if (self->coef[i].next == NULL)
                    self->coef[i].next =
                        cpl_polynomial_new_1d(CPL_MAX(first->coef[i].next->nc,
                                                      second->coef[i].next->nc));

                if (cpl_polynomial_add_(self->coef[i].next,
                                        first->coef[i].next,
                                        second->coef[i].next,
                                        mydim - 1)) {
                    cpl_polynomial_delete_(self->coef[i].next, mydim - 1);
                    self->coef[i].next = NULL;
                }
            }

            /* One operand is zero, so just copy the other */
            for (; i < nsecond; i++) {
                if (self->coef[i].next == NULL)
                    self->coef[i].next =
                        cpl_polynomial_new_1d(second->coef[i].next->nc);

                cpl_polynomial_copy_(self->coef[i].next,
                                    mydim - 1,
                                    second->coef[i].next,
                                    mydim - 1);
            }
            for (; i < nfirst; i++) {
                if (self->coef[i].next == NULL)
                    self->coef[i].next =
                        cpl_polynomial_new_1d(first->coef[i].next->nc);

                cpl_polynomial_copy_(self->coef[i].next,
                                    mydim - 1,
                                    first->coef[i].next,
                                    mydim - 1);
            }
        }
        /* Adjust degree of self and */
        /* Delete zero-valued leading coefficients */
        if (cpl_polynomial_prune_(self, mydim, i))
            do_del = CPL_TRUE;
    } else {
        do_del = CPL_TRUE;
    }
    return do_del;
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Subtract two polynomials of the same dimension
  @param    self   The polynomial to hold the result
  @param    first  The polynomial to subtract from, or NULL
  @param    second The polynomial to subtract, or NULL
  @param    mydim  This dimension, zero for deepest
  @return   CPL_TRUE iff the operation collapsed the dimension to zero
  @note self may be passed also as first and/or second
  @see cpl_polynomial_add_()
  @note Input with NULL/member nc == 0 means zero-polynomial
 */
/*----------------------------------------------------------------------------*/
static cpl_boolean cpl_polynomial_subtract_(cpl_polynomial_1d       * self,
                                            const cpl_polynomial_1d * first,
                                            const cpl_polynomial_1d * second,
                                            cpl_size                  mydim)
{
    cpl_boolean do_del = CPL_FALSE;

    if (first != NULL || second != NULL) {
        const cpl_size nfirst  = first  == NULL ? 0 : first->nc;
        const cpl_size nsecond = second == NULL ? 0 : second->nc;
        const cpl_size nmin    = CPL_MIN(nfirst, nsecond);
        cpl_size i = 0;

        if (mydim == 0) {

            if (self->nc < CPL_MAX(nfirst, nsecond))
                cpl_polynomial_grow_1d(self, CPL_MAX(nfirst, nsecond));

            for (; i < nmin; i++)
                self->coef[i].val = first->coef[i].val - second->coef[i].val;

            if (i < nsecond) {
                /* First operand is zero, so just negate the other */
                for (; i < nsecond; i++)
                    self->coef[i].val =  - second->coef[i].val;

                assert(i == second->nc);
                self->nc = i; /* Adjust degree of self */
            } else if (i < nfirst) {
                /* Second operand is zero, so just copy the first */
                if (self->coef != first->coef)
                    (void)memcpy(self->coef + i, first->coef + i,
                             (size_t)(nfirst - i) * sizeof(*self->coef));
                i = nfirst;
                assert(i == first->nc);
                self->nc = i; /* Adjust degree of self */
            }

            cpl_tools_add_flops( nmin );
        } else {

            if (self->nc < CPL_MAX(nfirst, nsecond)) {
                /* Need to reset new elements to NULL */
                cpl_polynomial_grow_1d_reset(self, CPL_MAX(nfirst, nsecond), 0);
            }

            for (; i < nmin; i++) {
                if (self->coef[i].next == NULL)
                    self->coef[i].next =
                        cpl_polynomial_new_1d(CPL_MAX(first->coef[i].next->nc,
                                                      second->coef[i].next->nc));

                if (cpl_polynomial_subtract_(self->coef[i].next,
                                    first->coef[i].next,
                                    second->coef[i].next,
                                             mydim - 1)) {
                    cpl_polynomial_delete_(self->coef[i].next, mydim - 1);
                    self->coef[i].next = NULL;
                }
            }

            for (; i < nsecond; i++) {
                /* First operand is zero, so just negate the other */
                if (self->coef[i].next == NULL)
                    self->coef[i].next =
                        cpl_polynomial_new_1d(second->coef[i].next->nc);

                if (cpl_polynomial_subtract_(self->coef[i].next,
                                             NULL,
                                             second->coef[i].next,
                                             mydim - 1)) {
                    cpl_polynomial_delete_(self->coef[i].next, mydim - 1);
                    self->coef[i].next = NULL;
                }
            }
            for (; i < nfirst; i++) {
                /* Second operand is zero, so just copy the first */
                if (self->coef[i].next == NULL)
                    self->coef[i].next =
                        cpl_polynomial_new_1d(first->coef[i].next->nc);

                cpl_polynomial_copy_(self->coef[i].next,
                                    mydim - 1,
                                    first->coef[i].next,
                                    mydim - 1);
            }
        }
        /* Adjust degree of self and */
        /* Delete zero-valued leading coefficients */
        if (cpl_polynomial_prune_(self, mydim, i))
            do_del = CPL_TRUE;
    } else {
        do_del = CPL_TRUE;
    }
    return do_del;
}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Multiply a polynomial with a scalar
  @param    self      The polynomial to hold the result
  @param    other     The polynomial to scale of same dimension, may equal self
  @param    mydim     This dimension, zero for deepest
  @param    factor    The non-zero factor to multiply with
  @return   void
  @note self and other may be aliased, must be non-NULL. member nc == 0 is OK.
  @see cpl_polynomial_multiply_scalar()

 */
/*----------------------------------------------------------------------------*/
static void cpl_polynomial_multiply_scalar_(cpl_polynomial_1d * self,
                                            const cpl_polynomial_1d * other,
                                            cpl_size mydim,
                                            double factor)
{
    if (mydim == 0) {
        if (self->nc < other->nc) {
            /* Self is too short, grow */
            cpl_polynomial_grow_1d(self, other->nc);
        }
        self->nc = other->nc;

        for (cpl_size i = 0; i < self->nc; i++) {
            self->coef[i].val = factor * other->coef[i].val;
        }
        cpl_tools_add_flops( self->nc );
    } else {
        if (self->nc < other->nc) {
            /* Self is too short, grow */
            cpl_polynomial_grow_1d_reset(self, other->nc, 0);
            self->nc = other->nc;
        } else {
            while (self->nc > other->nc) {
                /* Self is too long, shrink */
                self->nc--;
                cpl_polynomial_delete_(self->coef[self->nc].next, mydim - 1);
            }
        }

        for (cpl_size i = 0; i < self->nc; i++) {
            if (other->coef[i].next == NULL) {
                cpl_polynomial_delete_(self->coef[i].next, mydim - 1);
                self->coef[i].next = NULL;
                continue;
            }

            if (self->coef[i].next == NULL) self->coef[i].next =
                cpl_polynomial_new_1d(other->coef[i].next->nc);

            cpl_polynomial_multiply_scalar_(self->coef[i].next,
                                            other->coef[i].next,
                                            mydim - 1, factor);
        }
    }
    assert(self->nc == other->nc);
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Multiply a polynomial with a scalar
  @param    self      The polynomial to scale, or NULL
  @param    mydim     This dimension, zero for deepest
  @param    factor    The non-zero factor to multiply with
  @return   void
  @note member nc == 0 is OK.
  @see cpl_polynomial_multiply_scalar_()

 */
/*----------------------------------------------------------------------------*/
static void cpl_polynomial_multiply_scalar_self(cpl_polynomial_1d * self,
                                                cpl_size mydim,
                                                double factor)
{
    if (self != NULL) {
        if (mydim == 0) {
            for (cpl_size i = 0; i < self->nc; i++) {
                self->coef[i].val *= factor;
            }
            cpl_tools_add_flops( self->nc );
        } else {
            for (cpl_size i = 0; i < self->nc; i++) {
                cpl_polynomial_multiply_scalar_self(self->coef[i].next,
                                                    mydim - 1, factor);
            }
        }
    }
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Scale a polynomial and add it to another of the same dimension
  @param    self   The polynomial to add to
  @param    second The polynomial to scale and add, or NULL
  @param    mydim  This dimension, zero for deepest
  @param    factor The monomial scaling factor
  @param    pows   The non-negative power(s) of the monomial
  @return   CPL_TRUE iff the operation collapsed the dimension to zero
  @note self may be passed also as second
  @see cpl_polynomial_add()
 */
/*----------------------------------------------------------------------------*/
static cpl_boolean cpl_polynomial_scale_add_(cpl_polynomial_1d       * self,
                                             const cpl_polynomial_1d * second,
                                             cpl_size                  mydim,
                                             double                    factor,
                                             const cpl_size          * pows)
{
    cpl_boolean do_del = CPL_FALSE;

    if (second != NULL) {
        if (mydim == 0) {
            cpl_size i = 0;

            /* Multiplication of powers means adding their index */

            for (; i + pows[mydim] < self->nc; i++)
                self->coef[i + pows[mydim]].val += factor * second->coef[i].val;

            if (i < second->nc) {
                /* First operand is zero, so just scale the other - need to
                   reset extension, since some elements may remain zero */
                cpl_polynomial_grow_1d_reset(self, second->nc + pows[mydim], 0);

                for (; i < second->nc; i++)
                    self->coef[i + pows[mydim]].val
                        = factor * second->coef[i].val;

                cpl_tools_add_flops(second->nc + self->nc);
                assert(i == second->nc);
                self->nc = second->nc + pows[mydim]; /* Adjust degree of self */
            } else {
                cpl_tools_add_flops(2 * self->nc);
            }
        } else {
            cpl_size i;
            assert(mydim > 0);

            /* Multiplication of powers means adding their index */

            if (self->nc < second->nc + pows[mydim]) {
                /* Need to reset new elements to NULL */
                cpl_polynomial_grow_1d_reset(self, second->nc + pows[mydim], 0);

                self->nc = second->nc + pows[mydim]; /* Adjust degree of self */
            }

            for (i = 0; i < second->nc; i++) {
                if (self->coef[i + pows[mydim]].next == NULL)
                    self->coef[i + pows[mydim]].next =
                        cpl_polynomial_new_1d(second->coef[i].next->nc
                                              + pows[mydim - 1]);

                if (cpl_polynomial_scale_add_(self->coef[i + pows[mydim]].next,
                                              second->coef[i].next,
                                              mydim - 1, factor, pows)) {
                    cpl_polynomial_delete_(self->coef[i + pows[mydim]].next,
                                           mydim - 1);
                    self->coef[i + pows[mydim]].next = NULL;
                }
            }
        }
        /* Delete zero-valued leading coefficients */
        if (cpl_polynomial_prune_(self, mydim, self->nc))
            do_del = CPL_TRUE;
    } else {
            do_del = CPL_TRUE;
    }
    return do_del;
}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Multiply two polynomials of the same dimension
  @param    self   The polynomial to hold the result, at top-level
  @param    first  The 1st polynomial to multiply, at top-level
  @param    second The 2nd polynomial to multiply
  @param    mypow  The power of this dimension, aliased with pows + mydim
  @param    mydim  This dimension, zero for deepest
  @param    pows   The non-negative power(s) of the variable(s) of second
  @param    dim    The positive polynomial dimension (number of variables)
  @return   void
  @note self may be passed also as first and/or second
  @see cpl_polynomial_add_()

  Iterate through each monomial element of the second polynomial, multiply
  each term onto the first and add the result to self.

 */
/*----------------------------------------------------------------------------*/
static void cpl_polynomial_multiply_(cpl_polynomial_1d       * self,
                                     const cpl_polynomial_1d * first,
                                     const cpl_polynomial_1d * second,
                                     cpl_size                * mypow,
                                     cpl_size                  mydim,
                                     const cpl_size          * pows,
                                     cpl_size                  dim)
{
    if (mydim == 0) {
        for (cpl_size i = 0; i < second->nc; i++) {
            const double value = second->coef[i].val;
            if (value != 0.0) {
                *mypow = i;

                /* Multiply the monomial element of the second polynomial
                   onto the first and add the result to self */

                if (cpl_polynomial_scale_add_(self, first, dim - 1,
                                              value, pows)) {
                    /* FIXME: This branch is untested.
                       Can this point actually be reached ?! */

                    (void)cpl_polynomial_prune_(self, dim - 1, self->nc);
                }
            }
        }
    } else {
        for (cpl_size i = 0; i < second->nc; i++) {
            if (second->coef[i].next == NULL) continue;
            *mypow = i;

            cpl_polynomial_multiply_(self, first, second->coef[i].next,
                                     mypow - 1, mydim - 1,
                                     pows, dim);
        }
    }
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Modify self, self(x0, x1, ..., xn) := (x0+u, x1, ..., xn)
  @param  self  The polynomial to be modified in place
  @param  mydim This dimension, zero for deepest
  @param  u     The shift
  @return void
  @see cpl_polynomial_shift_1d()
  @note Member nc == 0 (for mydim == 0)/NULL-input causes undefined behaviour

 */
/*----------------------------------------------------------------------------*/
inline static void cpl_polynomial_shift_1d_(cpl_polynomial_1d * self,
                                            cpl_size mydim,
                                            double u)
{
    if (mydim > 0) {
        for (cpl_size i = 0; i < self->nc; i++) {
            if (self->coef[i].next == NULL) continue;

            cpl_polynomial_shift_1d_(self->coef[i].next, mydim - 1, u);
        }
    } else {
#ifdef CPL_POLYNOMIAL_UNION_IS_DOUBLE_SZ
        cpl_polynomial_shift_double(&(self->coef[0].val), self->nc, u);
#else
        for (cpl_size j = 0; j < self->nc - 1; j++)
            for (cpl_size i = 1; i < self->nc - j; i++)
                self->coef[self->nc - 1 - i].val += 
                    self->coef[self->nc - i].val * u;

        cpl_tools_add_flops( n * ( n - 1) );
#endif
    }
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief   Collapse one dimension of a multi-variate polynomial by composition
  @param   self  Pre-allocated (empty) output polynomial
  @param   other Polynomial to extract from
  @param   mydim This dimension, zero for lowest
  @param   dodim The dimension to collapse, zero for lowest
  @param   value The value that replaces the independent variable to collapse
  @return  CPL_TRUE iff the operation collapsed the dimension to zero
  @see cpl_polynomial_extract()
 */
/*----------------------------------------------------------------------------*/
static cpl_boolean cpl_polynomial_extract_(cpl_polynomial_1d * self,
                                           const cpl_polynomial_1d * other,
                                           cpl_size mydim,
                                           cpl_size dodim,
                                           double value)
{
    cpl_boolean do_del = CPL_FALSE;

    if (mydim == dodim) {
        /* Collapse the current dimension into the lower one, using Horner */

        /* When value is zero, only the constant term survives */
        const cpl_size icopy = value != 0.0 ? other->nc - 1 : 0;

        cpl_polynomial_copy_(self, mydim - 1,
                             other->coef[icopy].next, mydim - 1);

        for (cpl_size i = icopy; i > 0; i--) {
            cpl_polynomial_multiply_scalar_self(self, mydim - 1, value);

            if (other->coef[i-1].next == NULL) continue;

            cpl_polynomial_add_(self, self, other->coef[i-1].next,
                                mydim - 1);
        }

        if (icopy > 0) {
            /* Above addition may have caused cancellation of terms */
            /* Delete zero-valued leading coefficients */
            /* NB: Dimension of self is one less that other! */
            if (cpl_polynomial_prune_(self, mydim - 1, self->nc))
                do_del = CPL_TRUE;
        }
    } else if (mydim == 1) {
        /* Collapse the lowest dimension into the current one,
           ignoring all leading coefficients that evaluate to zero */

        cpl_size i = other->nc;

        assert(dodim == 0);
        assert(self->nc == 0);
        assert(self->nz >= other->nc);

        for (; i > 0; i--) {
            const double leading = cpl_polynomial_eval_(other->coef[i-1].next,
                                                        mydim - 1, &value);
            if (leading != 0.0) {
                self->nc = i;
                i--;
                self->coef[i].val = leading;
                break;
            }
        }

        for (; i > 0; i--) {
            self->coef[i-1].val = cpl_polynomial_eval_(other->coef[i-1].next,
                                                       mydim - 1, &value);
        }
        do_del = self->nc == 0;
    } else {
        assert(mydim > 1);
        /* Copy this dimension */
        for (cpl_size i = 0; i < other->nc; i++) {
            if (other->coef[i].next == NULL) {
                self->coef[i].next = NULL;
            } else {
                self->coef[i].next =
                    cpl_polynomial_new_1d(other->coef[i].next->nc);

                if (cpl_polynomial_extract_(self->coef[i].next,
                                            other->coef[i].next,
                                            mydim - 1,
                                            dodim, value)) {
                    assert(self->coef[i].next->nc == 0);
                    cpl_polynomial_delete_(self->coef[i].next, mydim - 1);
                    self->coef[i].next = NULL;
                }
            }
        }
        /* Increase size of self from zero to actual */
        /* NB: Dimension of self is one less that other! */
        if (cpl_polynomial_prune_(self, mydim - 1, other->nc))
            do_del = CPL_TRUE;
    }
    return do_del;
}
