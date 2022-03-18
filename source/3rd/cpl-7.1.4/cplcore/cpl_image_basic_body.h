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

#if defined(CPL_OPERATION) && CPL_OPERATION == CPL_IMAGE_BASIC_HYPOT

/* Declaration */
static cpl_error_code CPL_CONCAT2X(cpl_image_hypot, CPL_CONCAT2X(CPL_TYPE1,
                                   CPL_CONCAT2X(CPL_TYPE2, CPL_TYPE3)))
    (cpl_image *,
     const cpl_image *,
     const cpl_image *)
#ifdef CPL_HAVE_ATTR_NONNULL
    __attribute__((nonnull(1, 3)))
#endif
;

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Type-specific version of the function
  @param    self     Pre-allocated image to hold the result
  @param    first    First operand (may be aliased to self)
  @param    second   Second operand
  @return   CPL_ERROR_NONE or the relevant the #_cpl_error_code_ on error
  @see      cpl_image_hypot()
  
 */
/*----------------------------------------------------------------------------*/
static cpl_error_code CPL_CONCAT2X(cpl_image_hypot, CPL_CONCAT2X(CPL_TYPE1,
                                   CPL_CONCAT2X(CPL_TYPE2, CPL_TYPE3)))
    (cpl_image * self,
     const cpl_image * first,
     const cpl_image * second)
{

    CPL_TYPE1       * p1 = (      CPL_TYPE1 *)self->pixels;
    const CPL_TYPE3 * p3 = (const CPL_TYPE3 *)second->pixels;
    const size_t     nxy = (size_t)(self->nx * self->ny);
    size_t           i;

    if (first == NULL) {
        for (i = 0; i < nxy; i++)
            p1[i] = (CPL_TYPE1)CPL_HYPOT(p1[i], p3[i]);

        /* Handle bad pixels map */
        if (second->bpm != NULL) {
            if (self->bpm == NULL) {
                self->bpm = cpl_mask_duplicate(second->bpm);
            } else {
                cpl_mask_or(self->bpm, second->bpm);
            }
        }
    } else {
        const CPL_TYPE2 * p2  = (const CPL_TYPE2 *)first->pixels;

        for (i = 0; i < nxy; i++)
            p1[i] = (CPL_TYPE1)CPL_HYPOT(p2[i], p3[i]);

        /* Handle bad pixels map */
        if (first->bpm != NULL && second->bpm != NULL) {
            if (self->bpm == NULL) self->bpm =
                cpl_mask_wrap(self->nx, self->ny, (cpl_binary*)cpl_malloc(nxy));

            cpl_mask_or_(cpl_mask_get_data(self->bpm),
                         cpl_mask_get_data_const(first->bpm),
                         cpl_mask_get_data_const(second->bpm), nxy);
        } else if (first->bpm != NULL) {
            assert( second->bpm == NULL );
            if (self->bpm == NULL) {
                self->bpm = cpl_mask_duplicate(first->bpm);
            } else {
                (void)memcpy(cpl_mask_get_data(self->bpm),
                             cpl_mask_get_data_const(first->bpm), nxy);
            }
        } else if (second->bpm != NULL) {
            assert( first-> bpm == NULL );
            if (self->bpm == NULL) {
                self->bpm = cpl_mask_duplicate(second->bpm);
            } else {
                (void)memcpy(cpl_mask_get_data(self->bpm),
                             cpl_mask_get_data_const(second->bpm), nxy);
            }
        } else if (self->bpm != NULL) {
            /* The input has no bad pixel map. We already have an output
               bpm and we don't know where it comes from, so instead of
               deallocating it, we just clear it. If the caller does not
               want a bad pixel map it should be deallocated by the caller */
            (void)memset(cpl_mask_get_data(self->bpm), 0, nxy);
        }
    }

    return CPL_ERROR_NONE;
}


#elif defined(CPL_OPERATION) && CPL_OPERATION == CPL_IMAGE_BASIC_OPERATE
case CPL_TYPE_T: {
    const CPL_TYPE * p2  = (const CPL_TYPE *)image2->pixels;
    const size_t     nxy = (size_t)(image1->nx * image1->ny);
    size_t           i;

    for (i = 0; i < nxy; i++)
        CPL_OPERATOR(pout[i], p1[i], p2[i]);
    break;
}

#elif defined(CPL_OPERATION) && CPL_OPERATION == CPL_IMAGE_BASIC_DIVIDE
case CPL_TYPE_T: {
    const CPL_TYPE * p2  = (const CPL_TYPE *)image2->pixels;
    const size_t     nxy = (size_t)(image1->nx * image1->ny);
    size_t           i;

    for (i = 0; i < nxy; i++) {
        if (p2[i] != (CPL_TYPE)0) {
            CPL_IMAGE_DIVISION(pout[i], p1[i], p2[i]);
        } else {
            pout[i] = (CPL_TYPE)0;
            pzeros[i] = CPL_BINARY_1;
            nzero++;
        }
    }
    break;
}

#elif defined(CPL_OPERATION) && CPL_OPERATION == CPL_IMAGE_BASIC_OPERATE_LOCAL
case CPL_TYPE_T: {
    const CPL_TYPE * p2  = (const CPL_TYPE *)im2->pixels;
    const size_t     nxy = (size_t)(im1->nx * im1->ny);
    size_t           i;

    for (i = 0; i < nxy; i++) 
        CPL_OPERATOR(p1[i], p2[i]);
    break;
}

#elif defined(CPL_OPERATION) && CPL_OPERATION == CPL_IMAGE_BASIC_DIVIDE_LOCAL
case CPL_TYPE_T: {
    const CPL_TYPE * p2  = (const CPL_TYPE * )im2->pixels;
    const size_t     nxy = (size_t)(im1->nx * im1->ny);
    size_t           i;

    for (i = 0; i < nxy; i++) {
        if (p2[i] != (CPL_TYPE)0) {
            CPL_IMAGE_DIVISIONASSIGN(p1[i], p2[i]);
        } else {
            pzeros[i] = CPL_BINARY_1;
            nzero++;
        }
    }
    break;
}
#else

/* Type dependent macros */
#if defined CPL_CLASS && CPL_CLASS == CPL_CLASS_DOUBLE
#define CPL_TYPE double
#define CPL_TYPE_CONCAT double
#define CPL_TYPE_T CPL_TYPE_DOUBLE
#define CPL_MATH_ABS fabs
#define CPL_TYPE_IS_FPOINT

#elif defined CPL_CLASS && CPL_CLASS == CPL_CLASS_FLOAT
#define CPL_TYPE float
#define CPL_TYPE_CONCAT float
#define CPL_TYPE_T CPL_TYPE_FLOAT
#define CPL_MATH_ABS fabsf
#define CPL_TYPE_IS_FPOINT

#elif defined CPL_CLASS && CPL_CLASS == CPL_CLASS_INT
#define CPL_TYPE int
#define CPL_TYPE_CONCAT int
#define CPL_TYPE_T CPL_TYPE_INT
#define CPL_MATH_ABS abs

#elif defined CPL_CLASS && CPL_CLASS == CPL_CLASS_DOUBLE_COMPLEX
#define CPL_TYPE double complex
#define CPL_TYPE_CONCAT double_complex
#define CPL_MATH_ABS cabs
#define CPL_TYPE_T CPL_TYPE_DOUBLE_COMPLEX
#define CPL_TYPE_IS_FPOINT

#elif defined CPL_CLASS && CPL_CLASS == CPL_CLASS_FLOAT_COMPLEX
#define CPL_TYPE float complex
#define CPL_TYPE_CONCAT float_complex
#define CPL_MATH_ABS cabsf
#define CPL_TYPE_T CPL_TYPE_FLOAT_COMPLEX
#define CPL_TYPE_IS_FPOINT

#else
#undef CPL_TYPE
#undef CPL_TYPE_T
#undef CPL_MATH_ABS
#undef CPL_TYPE_ADD
#endif

#define CPL_TYPE_ADD(a) CPL_CONCAT2X(a, CPL_TYPE_CONCAT)

#if CPL_OPERATION == CPL_IMAGE_BASIC_DECLARE

/*-----------------------------------------------------------------------------
                            Private Function prototypes
 -----------------------------------------------------------------------------*/

static cpl_image *
CPL_TYPE_ADD(cpl_image_collapse_window_create)(const cpl_image *, cpl_size,
                                               cpl_size, cpl_size, cpl_size,
                                               int);

static cpl_error_code CPL_TYPE_ADD(cpl_image_power)(cpl_image *, double);

static cpl_error_code CPL_TYPE_ADD(cpl_image_exponential)(cpl_image *, double);

static cpl_error_code CPL_TYPE_ADD(cpl_image_logarithm)(cpl_image *, double);

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Type-specific version of the function
  @param    self    Image of the given type to process
  @param    base    The positive base of the logarithm
  @return   CPL_ERROR_NONE or the relevant CPL error code on error
  @see      cpl_image_logarithm()
  
 */
/*----------------------------------------------------------------------------*/
static cpl_error_code CPL_TYPE_ADD(cpl_image_logarithm)(cpl_image * self,
                                                        double      base)
{

    double scale;
    const int myerrno = errno;
    CPL_TYPE * data = cpl_image_get_data(self);
    const size_t n = cpl_image_get_size_x(self) * cpl_image_get_size_y(self);
    const cpl_mask * mask = cpl_image_get_bpm_const(self);
    const cpl_binary * cbpm = mask ? cpl_mask_get_data_const(mask) : NULL;
    cpl_binary * bpm = NULL;
    size_t i;

    cpl_ensure_code(data != NULL, CPL_ERROR_ILLEGAL_INPUT);
    cpl_ensure_code(base >  0.0,  CPL_ERROR_ILLEGAL_INPUT);
    cpl_ensure_code(base != 1.0,  CPL_ERROR_DIVISION_BY_ZERO);


    /* In spite of checks log() may still set errno */
    errno = 0;

    scale = 1.0 / log(base);

    cpl_ensure_code(!errno,  CPL_ERROR_ILLEGAL_INPUT);

    /* Pixel value must be positive */
    for (i = 0; i < n; i++) {
        if (!cbpm || !cbpm[i]) { /* Pixel is not bad (yet) */
            if ((double)data[i] > 0.0) {
                data[i] = (CPL_TYPE)(log((double)data[i]) * scale);
            } else {
                errno = EDOM; /* Use errno to reuse code */
            }
            if (errno) {
                errno = 0;
                if (!bpm) {
                    /* Create the Bad Pixel Map if necessary */
                    /* If cbpm is NULL, then keep it that way to save
                       some checking */
                    bpm = cpl_mask_get_data(cpl_image_get_bpm(self));
                }
                bpm[i] = CPL_BINARY_1;
                data[i] = 0; /* Set bad pixel to zero */
            }
        }
    }

    /* FIXME: Counts also bad pixels */
    cpl_tools_add_flops( 2 * n + 2);

    errno = myerrno;
    return CPL_ERROR_NONE;
}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Type-specific version of the function
  @param    self    Image of the given type to process
  @param    base    The base
  @return   CPL_ERROR_NONE or the relevant CPL error code on error
  @see      cpl_image_exponential()
  
 */
/*----------------------------------------------------------------------------*/
static cpl_error_code CPL_TYPE_ADD(cpl_image_exponential)(cpl_image * self,
                                                          double      base)
{

    const int myerrno = errno;
    CPL_TYPE * data = cpl_image_get_data(self);
    const size_t n = cpl_image_get_size_x(self) * cpl_image_get_size_y(self);
    const cpl_mask * mask = cpl_image_get_bpm_const(self);
    const cpl_binary * cbpm = mask ? cpl_mask_get_data_const(mask) : NULL;
    cpl_binary * bpm = NULL;
    size_t i;

    cpl_ensure_code(data != NULL, CPL_ERROR_ILLEGAL_INPUT);

    /* In spite of checks pow() may still overflow */
    errno = 0;

#ifdef CPL_TYPE_IS_FPOINT
    /* The branch with this check is not needed for integer pixels */
    if (base < 0.0) {
        /* Pixel value must be integer */
        for (i = 0; i < n; i++) {
            if (!cbpm || !cbpm[i]) { /* Pixel is not bad (yet) */
                if ((double)data[i] == ceil((double)data[i])) {
                    data[i] = (CPL_TYPE)pow(base, (double)data[i]);
                } else {
                    errno = EDOM; /* Use errno to reuse code */
                }
                if (errno) {
                    errno = 0;
                    if (!bpm) {
                        /* Create the Bad Pixel Map if necessary */
                        /* If cbpm is NULL, then keep it that way to save
                           some checking */
                        bpm = cpl_mask_get_data(cpl_image_get_bpm(self));
                    }
                    bpm[i] = CPL_BINARY_1;
                    data[i] = 0; /* Set bad pixel to zero */
                }
            }
        }
    } else
#endif
    if (base != 0.0) {
        /* Pixel value can be anything */
        for (i = 0; i < n; i++) {
            if (!cbpm || !cbpm[i]) { /* Pixel is not bad (yet) */
                data[i] = (CPL_TYPE)pow(base, (double)data[i]);
                if (errno) {
                    errno = 0;
                    if (!bpm) {
                        /* Create the Bad Pixel Map if necessary */
                        /* If cbpm is NULL, then keep it that way to save
                           some checking */
                        bpm = cpl_mask_get_data(cpl_image_get_bpm(self));
                    }
                    bpm[i] = CPL_BINARY_1;
                    data[i] = 0; /* Set bad pixel to zero */
                }
            }
        }
    } else {
        /* Base is zero, pixel value must be non-negative */
        for (i = 0; i < n; i++) {
            if (!cbpm || !cbpm[i]) { /* Pixel is not bad (yet) */
                if ((double)data[i] >= 0.0) {
                    data[i] = (CPL_TYPE)pow(base, (double)data[i]);
                } else {
                    errno = EDOM; /* Use errno to reuse code */
                }
                if (errno) {
                    errno = 0;
                    if (!bpm) {
                        /* Create the Bad Pixel Map if necessary */
                        /* If cbpm is NULL, then keep it that way to save
                           some checking */
                        bpm = cpl_mask_get_data(cpl_image_get_bpm(self));
                    }
                    bpm[i] = CPL_BINARY_1;
                    data[i] = 0; /* Set bad pixel to zero */
                }
            }
        }
    }

    /* FIXME: Counts also bad pixels */
    cpl_tools_add_flops( n );

    errno = myerrno;
    return CPL_ERROR_NONE;
}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Type-specific version of the function
  @param    self        Image of the given type to process
  @param    exponent    The exponent
  @return   CPL_ERROR_NONE or the relevant CPL error code on error
  @see      cpl_image_power()
  
 */
/*----------------------------------------------------------------------------*/
static cpl_error_code CPL_TYPE_ADD(cpl_image_power)(cpl_image * self,
                                                    double      exponent)
{

    const int myerrno = errno;
    CPL_TYPE * data = cpl_image_get_data(self);
    const size_t n = cpl_image_get_size_x(self) * cpl_image_get_size_y(self);
    const cpl_mask * mask = cpl_image_get_bpm_const(self);
    const cpl_binary * cbpm = mask ? cpl_mask_get_data_const(mask) : NULL;
    cpl_binary * bpm = NULL;
    size_t i;

    cpl_ensure_code(data != NULL, CPL_ERROR_ILLEGAL_INPUT);

    /* In spite of checks pow() may still overflow */
    errno = 0;

    if (exponent == -0.5) {
        /* Pixel value must be positive */
        for (i = 0; i < n; i++) {
            if (!cbpm || !cbpm[i]) { /* Pixel is not bad (yet) */
                if ((double)data[i] > 0.0) {
                    data[i] = (CPL_TYPE)(1.0/sqrt((double)data[i]));
                } else {
                    errno = EDOM; /* Use errno to reuse code */
                }
                if (errno) {
                    errno = 0;
                    if (!bpm) {
                        /* Create the Bad Pixel Map if necessary */
                        /* If cbpm is NULL, then keep it that way to save
                           some checking */
                        bpm = cpl_mask_get_data(cpl_image_get_bpm(self));
                    }
                    bpm[i] = CPL_BINARY_1;
                    data[i] = 0; /* Set bad pixel to zero */
                }
            }
        }
#ifdef HAVE_CBRT
    } else if (exponent == -1.0 / 3.0) {
        /* Pixel value must be positive */
        for (i = 0; i < n; i++) {
            if (!cbpm || !cbpm[i]) { /* Pixel is not bad (yet) */
                if ((double)data[i] > 0.0) {
                    data[i] = (CPL_TYPE)(1.0/cbrt((double)data[i]));
                } else {
                    errno = EDOM; /* Use errno to reuse code */
                }
                if (errno) {
                    errno = 0;
                    if (!bpm) {
                        /* Create the Bad Pixel Map if necessary */
                        /* If cbpm is NULL, then keep it that way to save
                           some checking */
                        bpm = cpl_mask_get_data(cpl_image_get_bpm(self));
                    }
                    bpm[i] = CPL_BINARY_1;
                    data[i] = 0; /* Set bad pixel to zero */
                }
            }
        }
#endif
    } else if (exponent < 0.0) {
        if (exponent != ceil(exponent)) {
            /* Pixel value must be positive */
            for (i = 0; i < n; i++) {
                if (!cbpm || !cbpm[i]) { /* Pixel is not bad (yet) */
                    if ((double)data[i] > 0.0) {
                        data[i] = (CPL_TYPE)pow((double)data[i], exponent);
                    } else {
                        errno = EDOM; /* Use errno to reuse code */
                    }
                    if (errno) {
                        errno = 0;
                        if (!bpm) {
                            /* Create the Bad Pixel Map if necessary */
                            /* If cbpm is NULL, then keep it that way to save
                               some checking */
                            bpm = cpl_mask_get_data(cpl_image_get_bpm(self));
                        }
                        bpm[i] = CPL_BINARY_1;
                        data[i] = 0; /* Set bad pixel to zero */
                    }
                }
            }
        } else {
            /* Pixel value must be non-zero */
            for (i = 0; i < n; i++) {
                if (!cbpm || !cbpm[i]) { /* Pixel is not bad (yet) */
                    if ((double)data[i] != 0.0) {
                        data[i] = (CPL_TYPE)cpl_tools_ipow((double)data[i],
                                                           exponent);
                    } else {
                        errno = EDOM; /* Use errno to reuse code */
                    }
                    if (errno) {
                        errno = 0;
                        if (!bpm) {
                            /* Create the Bad Pixel Map if necessary */
                            /* If cbpm is NULL, then keep it that way to save
                               some checking */
                            bpm = cpl_mask_get_data(cpl_image_get_bpm(self));
                        }
                        bpm[i] = CPL_BINARY_1;
                        data[i] = 0; /* Set bad pixel to zero */
                    }
                }
            }
        }
    } else if (exponent == 0.5) {
        /* Pixel value must be non-negative */
        for (i = 0; i < n; i++) {
            if (!cbpm || !cbpm[i]) { /* Pixel is not bad (yet) */
                if ((double)data[i] >= 0.0) {
                    data[i] = (CPL_TYPE)sqrt((double)data[i]);
                } else {
                    errno = EDOM; /* Use errno to reuse code */
                }
                if (errno) {
                    errno = 0;
                    if (!bpm) {
                        /* Create the Bad Pixel Map if necessary */
                        /* If cbpm is NULL, then keep it that way to save
                           some checking */
                        bpm = cpl_mask_get_data(cpl_image_get_bpm(self));
                    }
                    bpm[i] = CPL_BINARY_1;
                    data[i] = 0; /* Set bad pixel to zero */
                }
            }
        }
#ifdef HAVE_CBRT
    } else if (exponent == 1.0 / 3.0) {
        if (cbpm == NULL) {
            /* No bad pixels */
            for (i = 0; i < n; i++) {
                data[i] = (CPL_TYPE)cbrt((double)data[i]);
            }
        } else {
            for (i = 0; i < n; i++) {
                if (!cbpm[i]) { /* Pixel is not bad */
                    data[i] = (CPL_TYPE)cbrt((double)data[i]);
                }
            }
        }
#endif
    } else if (exponent != ceil(exponent)) {
        /* Pixel value must be non-negative */
        for (i = 0; i < n; i++) {
            if (!cbpm || !cbpm[i]) { /* Pixel is not bad (yet) */
                if ((double)data[i] >= 0.0) {
                    data[i] = (CPL_TYPE)pow((double)data[i], exponent);
                } else {
                    errno = EDOM; /* Use errno to reuse code */
                }
                if (errno) {
                    errno = 0;
                    if (!bpm) {
                        /* Create the Bad Pixel Map if necessary */
                        /* If cbpm is NULL, then keep it that way to save
                           some checking */
                        bpm = cpl_mask_get_data(cpl_image_get_bpm(self));
                    }
                    bpm[i] = CPL_BINARY_1;
                    data[i] = 0; /* Set bad pixel to zero */
                }
            }
        }
    } else if (exponent != 1.0) {
        /* Pixel value can be anything */
        for (i = 0; i < n; i++) {
            if (!cbpm || !cbpm[i]) { /* Pixel is not bad (yet) */
                data[i] = (CPL_TYPE)cpl_tools_ipow((double)data[i], exponent);
                if (errno) {
                    errno = 0;
                    if (!bpm) {
                        /* Create the Bad Pixel Map if necessary */
                        /* If cbpm is NULL, then keep it that way to save
                           some checking */
                        bpm = cpl_mask_get_data(cpl_image_get_bpm(self));
                    }
                    bpm[i] = CPL_BINARY_1;
                    data[i] = 0; /* Set bad pixel to zero */
                }
            }
        }
    }

    /* FIXME: Counts also bad pixels */
    cpl_tools_add_flops( n );

    errno = myerrno;
    return CPL_ERROR_NONE;
}



/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief    Collapse an image region along its rows or columns.
  @param    self        Image of type CPL_TYPE to collapse.
  @param    llx         lower left x coord.
  @param    lly         lower left y coord
  @param    urx         upper right x coord
  @param    ury         upper right y coord
  @param    direction   Collapsing direction.
  @return   a newly allocated image or NULL in error case
  @note This static function will assert() on invalid image input
  @see      cpl_image_collapse_window_create()
  
  llx, lly, urx, ury are the image region coordinates in FITS convention.
  Those specified bounds are included in the collapsed region.

  The returned image must be deallocated using cpl_image_delete().

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_ILLEGAL_INPUT if the specified window is not valid
 */
/*----------------------------------------------------------------------------*/
static cpl_image *
CPL_TYPE_ADD(cpl_image_collapse_window_create)(const cpl_image * self,
                                               cpl_size          llx,
                                               cpl_size          lly,
                                               cpl_size          urx,
                                               cpl_size          ury,
                                               int               direction)
{

    cpl_image        * other;
    const CPL_TYPE   * pi   = (const CPL_TYPE*)self->pixels;
    const cpl_binary * bpmi = self->bpm ? cpl_mask_get_data(self->bpm) : NULL;
    CPL_TYPE         * po;
    cpl_binary       * bpmo = NULL;
    const size_t       n1x  = 1 + urx - llx;
    const size_t       n1y  = 1 + ury - lly;
    size_t             i, j;


    cpl_ensure(llx >= 1,        CPL_ERROR_ILLEGAL_INPUT, NULL);
    cpl_ensure(lly >= 1,        CPL_ERROR_ILLEGAL_INPUT, NULL);
    cpl_ensure(urx <= self->nx, CPL_ERROR_ILLEGAL_INPUT, NULL);
    cpl_ensure(ury <= self->ny, CPL_ERROR_ILLEGAL_INPUT, NULL);

    assert( self->type == CPL_TYPE_T );

    /* Let pi and bpmi point to first pixel to collapse */
    pi += self->nx * (lly-1) + (llx - 1);
    if (bpmi != NULL) bpmi += self->nx * (lly-1) + (llx - 1);

    if (direction == 0) {
        const double r1y = (double)n1y;
        size_t * nok;

        other = cpl_image_new(n1x, 1, CPL_TYPE_T);
        po = (CPL_TYPE*)other->pixels;

        /* To obtain a stride-1 access of pi[], some temporary storage is needed
           - this may be a disadvantage for very small images */
        nok = (size_t*)cpl_calloc(n1x, sizeof(*nok));

        for (j=0; j < n1y; j++) {
            for (i=0; i < n1x; i++) {
                if (bpmi == NULL || !bpmi[i]) {
                    po[i] += pi[i];
                    nok[i]++; /* Count good pixels */
                }
            }
            pi += self->nx;
            if (bpmi != NULL) bpmi += self->nx;
        }
        if (bpmi != NULL) {
            for (i=0; i < n1x; i++) {
                if (nok[i] == 0) {
                    /* assert(po[i] == 0.0); */
                    if (bpmo == NULL)
                        bpmo = cpl_mask_get_data(cpl_image_get_bpm(other));
                    bpmo[i] = CPL_BINARY_1;
                } else if (nok[i] < n1y) {
                    po[i] *= r1y / (double)nok[i];
                }
            }
        }
        cpl_free(nok);
    } else if (direction == 1) {
        const double r1x = (double)n1x;
        double       sum;
        size_t       nok;

        other = cpl_image_new(1, n1y, CPL_TYPE_T);
        po = (CPL_TYPE*)other->pixels;

        for (j=0; j < n1y; j++) {
            sum = 0.0;
            nok = 0;
            for (i=0; i < n1x; i++) {
                if (bpmi == NULL || !bpmi[i]) {
                    sum += pi[i];
                    nok++;
                }
            }
            if (nok == 0) {
                /* assert(po[j] == 0.0); */
                if (bpmo == NULL)
                    bpmo = cpl_mask_get_data(cpl_image_get_bpm(other));
                bpmo[j] = CPL_BINARY_1;
            } else {
                po[j] = nok == n1x ? sum : sum * r1x / (double)nok;
            }
            pi += self->nx;
            if (bpmi != NULL) bpmi += self->nx;
        }
    } else {
        (void)cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);
        other = NULL;
    }

#ifdef CPL_TYPE_IS_FPOINT
    /* FIXME: Counts also bad pixels */
    cpl_tools_add_flops(n1x * n1y);
#endif

    return other;
}


#elif CPL_OPERATION == CPL_IMAGE_BASIC_ASSIGN

#undef CPL_OPERATION
#define CPL_OPERATION CPL_IMAGE_BASIC_OPERATE

    cpl_image * self;

    if (image1 == NULL) {
        (void)cpl_error_set_message_(CPL_ERROR_NULL_INPUT,
                                     "First image is NULL");
        return NULL;
    } else if (image2 == NULL) {
        (void)cpl_error_set_message_(CPL_ERROR_NULL_INPUT,
                                     "Second image is NULL");
        return NULL;
    } else if (image1->nx != image2->nx) {
        (void)cpl_error_set_message_(CPL_ERROR_INCOMPATIBLE_INPUT,
                                     "Images are %" CPL_SIZE_FORMAT " X %"
                                     CPL_SIZE_FORMAT " (%s) <=> %"
                                     CPL_SIZE_FORMAT " X %" CPL_SIZE_FORMAT
                                     " (%s)", image1->nx, image1->ny,
                                     cpl_type_get_name(image1->type),
                                     image2->nx, image2->ny,
                                     cpl_type_get_name(image2->type));
        return NULL;
    } else if (image1->ny != image2->ny) {
        (void)cpl_error_set_message_(CPL_ERROR_INCOMPATIBLE_INPUT,
                                     "Images are %" CPL_SIZE_FORMAT " X %"
                                     CPL_SIZE_FORMAT " (%s) <=> %"
                                     CPL_SIZE_FORMAT " X %" CPL_SIZE_FORMAT
                                     " (%s)", image1->nx, image1->ny,
                                     cpl_type_get_name(image1->type),
                                     image2->nx, image2->ny,
                                     cpl_type_get_name(image2->type));
        return NULL;
    }

    assert( image1->pixels );
    assert( image2->pixels );

    /* Switch on the first passed image type */
    switch (image1->type) {
#define CPL_MATH_ABS1 abs
    case CPL_TYPE_INT: {
        const int * p1 = (const int *)image1->pixels;
        int       * pout = (int *)cpl_malloc(image1->nx * image1->ny *
                                             sizeof(*pout));

        /* Switch on the second passed image type */
        switch (image2->type) {
#define CPL_TYPE_T CPL_TYPE_INT
#define CPL_TYPE   int
#define CPL_MATH_ABS2 abs
#include "cpl_image_basic_body.h"
#undef CPL_TYPE_T
#undef CPL_TYPE
#undef CPL_MATH_ABS2

#define CPL_TYPE_T CPL_TYPE_FLOAT
#define CPL_TYPE   float
#define CPL_MATH_ABS2 fabsf
#include "cpl_image_basic_body.h"
#undef CPL_TYPE_T
#undef CPL_TYPE
#undef CPL_MATH_ABS2

#define CPL_TYPE_T CPL_TYPE_DOUBLE
#define CPL_TYPE   double
#define CPL_MATH_ABS2 fabs
#include "cpl_image_basic_body.h"
#undef CPL_TYPE_T
#undef CPL_TYPE
#undef CPL_MATH_ABS2

        default:
            cpl_free(pout);
            (void)cpl_error_set_(CPL_ERROR_TYPE_MISMATCH);
            pout = NULL;
        }
        self = pout ? cpl_image_wrap_int(image1->nx, image1->ny, pout) : NULL;
        break;
    }
#undef CPL_MATH_ABS1
#define CPL_MATH_ABS1 fabsf
    case CPL_TYPE_FLOAT: {
        const float * p1   = (const float *)image1->pixels;
        float       * pout = (float *)cpl_malloc(image1->nx * image1->ny *
                                                 sizeof(*pout));

        /* Switch on the second passed image type */
        switch (image2->type) {
#define CPL_TYPE_T CPL_TYPE_INT
#define CPL_TYPE   int
#define CPL_MATH_ABS2 abs
#include "cpl_image_basic_body.h"
#undef CPL_TYPE_T
#undef CPL_TYPE
#undef CPL_MATH_ABS2

#define CPL_TYPE_T CPL_TYPE_FLOAT
#define CPL_TYPE   float
#define CPL_MATH_ABS2 fabsf
#include "cpl_image_basic_body.h"
#undef CPL_TYPE_T
#undef CPL_TYPE
#undef CPL_MATH_ABS2

#define CPL_TYPE_T CPL_TYPE_DOUBLE
#define CPL_TYPE   double
#define CPL_MATH_ABS2 fabs
#include "cpl_image_basic_body.h"
#undef CPL_TYPE_T
#undef CPL_TYPE
#undef CPL_MATH_ABS2

        default:
            cpl_free(pout);
            (void)cpl_error_set_(CPL_ERROR_TYPE_MISMATCH);
            pout = NULL;
        }
        self = pout ? cpl_image_wrap_float(image1->nx, image1->ny, pout) : NULL;
        break;
    }
#undef CPL_MATH_ABS1
#define CPL_MATH_ABS1 fabs
    case CPL_TYPE_DOUBLE: {
        const double * p1   = (const double *)image1->pixels;
        double       * pout = (double *)cpl_malloc(image1->nx * image1->ny *
                                                   sizeof(*pout));

        /* Switch on the second passed image type */
        switch (image2->type) {
#define CPL_TYPE_T CPL_TYPE_INT
#define CPL_TYPE   int
#define CPL_MATH_ABS2 abs
#include "cpl_image_basic_body.h"
#undef CPL_TYPE_T
#undef CPL_TYPE
#undef CPL_MATH_ABS2

#define CPL_TYPE_T CPL_TYPE_FLOAT
#define CPL_TYPE   float
#define CPL_MATH_ABS2 fabsf
#include "cpl_image_basic_body.h"
#undef CPL_TYPE_T
#undef CPL_TYPE
#undef CPL_MATH_ABS2

#define CPL_TYPE_T CPL_TYPE_DOUBLE
#define CPL_TYPE   double
#define CPL_MATH_ABS2 fabs
#include "cpl_image_basic_body.h"
#undef CPL_TYPE_T
#undef CPL_TYPE
#undef CPL_MATH_ABS2

        default:
            cpl_free(pout);
            (void)cpl_error_set_(CPL_ERROR_TYPE_MISMATCH);
            pout = NULL;
        }
        self = pout ? cpl_image_wrap_double(image1->nx, image1->ny, pout)
            : NULL;
        break;
    }

#undef CPL_MATH_ABS1
#define CPL_MATH_ABS1 cabsf
    case CPL_TYPE_FLOAT_COMPLEX: {
        const float complex * p1   = (const float complex *)image1->pixels;
        float complex       * pout =
            (float complex *)cpl_malloc(image1->nx * image1->ny * sizeof(*pout));

        /* Switch on the second passed image type */
        switch (image2->type) {
#define CPL_TYPE_T CPL_TYPE_INT
#define CPL_TYPE   int
#define CPL_MATH_ABS2 abs
#include "cpl_image_basic_body.h"
#undef CPL_TYPE_T
#undef CPL_TYPE
#undef CPL_MATH_ABS2

#define CPL_TYPE_T CPL_TYPE_FLOAT
#define CPL_TYPE   float
#define CPL_MATH_ABS2 fabsf
#include "cpl_image_basic_body.h"
#undef CPL_TYPE_T
#undef CPL_TYPE
#undef CPL_MATH_ABS2

#define CPL_TYPE_T CPL_TYPE_DOUBLE
#define CPL_TYPE   double
#define CPL_MATH_ABS2 fabs
#include "cpl_image_basic_body.h"
#undef CPL_TYPE_T
#undef CPL_TYPE
#undef CPL_MATH_ABS2

#define CPL_TYPE_T CPL_TYPE_FLOAT_COMPLEX
#define CPL_TYPE   float complex
#define CPL_MATH_ABS2 cabsf
#include "cpl_image_basic_body.h"
#undef CPL_TYPE_T
#undef CPL_TYPE
#undef CPL_MATH_ABS2

#define CPL_TYPE_T CPL_TYPE_DOUBLE_COMPLEX
#define CPL_TYPE   double complex
#define CPL_MATH_ABS2 cabs
#include "cpl_image_basic_body.h"
#undef CPL_TYPE_T
#undef CPL_TYPE
#undef CPL_MATH_ABS2

        default:
            cpl_free(pout);
            (void)cpl_error_set_(CPL_ERROR_TYPE_MISMATCH);
            pout = NULL;
        }
        self = pout ? cpl_image_wrap_float_complex(image1->nx, image1->ny, pout)
            : NULL;
        break;
    }
#undef CPL_MATH_ABS1
#define CPL_MATH_ABS1 cabs
    case CPL_TYPE_DOUBLE_COMPLEX: {
        const double complex * p1   = (const double complex *)image1->pixels;
        double complex       * pout =
            (double complex *)cpl_malloc(image1->nx * image1->ny * sizeof(*pout));

        /* Switch on the second passed image type */
        switch (image2->type) {
#define CPL_TYPE_T CPL_TYPE_INT
#define CPL_TYPE   int
#define CPL_MATH_ABS2 abs
#include "cpl_image_basic_body.h"
#undef CPL_TYPE_T
#undef CPL_TYPE
#undef CPL_MATH_ABS2

#define CPL_TYPE_T CPL_TYPE_FLOAT
#define CPL_TYPE   float
#define CPL_MATH_ABS2 fabsf
#include "cpl_image_basic_body.h"
#undef CPL_TYPE_T
#undef CPL_TYPE
#undef CPL_MATH_ABS2

#define CPL_TYPE_T CPL_TYPE_DOUBLE
#define CPL_TYPE   double
#define CPL_MATH_ABS2 fabs
#include "cpl_image_basic_body.h"
#undef CPL_TYPE_T
#undef CPL_TYPE
#undef CPL_MATH_ABS2

#define CPL_TYPE_T CPL_TYPE_FLOAT_COMPLEX
#define CPL_TYPE   float complex
#define CPL_MATH_ABS2 cabsf
#include "cpl_image_basic_body.h"
#undef CPL_TYPE_T
#undef CPL_TYPE
#undef CPL_MATH_ABS2

#define CPL_TYPE_T CPL_TYPE_DOUBLE_COMPLEX
#define CPL_TYPE   double complex
#define CPL_MATH_ABS2 cabs
#include "cpl_image_basic_body.h"
#undef CPL_TYPE_T
#undef CPL_TYPE
#undef CPL_MATH_ABS2

        default:
            cpl_free(pout);
            (void)cpl_error_set_(CPL_ERROR_TYPE_MISMATCH);
            pout = NULL;
        }
        self = pout ? cpl_image_wrap_double_complex(image1->nx, image1->ny,
                                                    pout) : NULL;
        break;
    }
#undef CPL_MATH_ABS1
    default:
        (void)cpl_error_set_(CPL_ERROR_INVALID_TYPE);
        self = NULL;
    }

    if (self != NULL) {

        if (image1->type != CPL_TYPE_INT && image2->type != CPL_TYPE_INT) {
            cpl_tools_add_flops(image1->nx * image1->ny);
        }

        /* Handle bad pixels map */
        if (image1->bpm == NULL && image2->bpm == NULL) {
            self->bpm = NULL;
        } else if (image1->bpm == NULL) {
            self->bpm = cpl_mask_duplicate(image2->bpm);
        } else if (image2->bpm == NULL) {
            self->bpm = cpl_mask_duplicate(image1->bpm);
        } else {
            self->bpm = cpl_mask_duplicate(image1->bpm);
            cpl_mask_or(self->bpm, image2->bpm);
        }
    }

    return self;

#undef CPL_OPERATION
#define CPL_OPERATION CPL_IMAGE_BASIC_ASSIGN

#elif  CPL_OPERATION == CPL_IMAGE_BASIC_ASSIGN_LOCAL

#undef CPL_OPERATION
#define CPL_OPERATION CPL_IMAGE_BASIC_OPERATE_LOCAL

    if (im1 == NULL) {
        return cpl_error_set_message_(CPL_ERROR_NULL_INPUT,
                                      "First image is NULL");
    } else if (im2 == NULL) {
        return cpl_error_set_message_(CPL_ERROR_NULL_INPUT,
                                      "Second image is NULL");
    } else if (im1->nx != im2->nx) {
        return cpl_error_set_message_(CPL_ERROR_INCOMPATIBLE_INPUT,
                                      "Images are %" CPL_SIZE_FORMAT " X %"
                                      CPL_SIZE_FORMAT " (%s) <=> %"
                                      CPL_SIZE_FORMAT " X %" CPL_SIZE_FORMAT
                                      " (%s)", im1->nx, im1->ny,
                                      cpl_type_get_name(im1->type), im2->nx,
                                      im2->ny, cpl_type_get_name(im2->type));
    } else if (im1->ny != im2->ny) {
        return cpl_error_set_message_(CPL_ERROR_INCOMPATIBLE_INPUT,
                                      "Images are %" CPL_SIZE_FORMAT " X %"
                                      CPL_SIZE_FORMAT " (%s) <=> %"
                                      CPL_SIZE_FORMAT " X %" CPL_SIZE_FORMAT
                                      " (%s)", im1->nx, im1->ny,
                                      cpl_type_get_name(im1->type), im2->nx,
                                      im2->ny, cpl_type_get_name(im2->type));
    }

    assert( im1->pixels );
    assert( im2->pixels );

    /* Switch on the first passed image type */
    switch (im1->type) {
    case CPL_TYPE_INT: {
        int * p1 = (int *)im1->pixels;

        /* Switch on the second passed image type */
        switch (im2->type) {
#define CPL_TYPE_T CPL_TYPE_INT
#define CPL_TYPE   int
#include "cpl_image_basic_body.h"
#undef CPL_TYPE_T
#undef CPL_TYPE

#define CPL_TYPE_T CPL_TYPE_FLOAT
#define CPL_TYPE   float
#include "cpl_image_basic_body.h"
#undef CPL_TYPE_T
#undef CPL_TYPE

#define CPL_TYPE_T CPL_TYPE_DOUBLE
#define CPL_TYPE   double
#include "cpl_image_basic_body.h"
#undef CPL_TYPE_T
#undef CPL_TYPE
        default:
            return cpl_error_set_(CPL_ERROR_TYPE_MISMATCH);
        }
        break;
    }
    case CPL_TYPE_FLOAT: {
        float * p1 = (float *)im1->pixels;

        /* Switch on the second passed image type */
        switch (im2->type) {
#define CPL_TYPE_T CPL_TYPE_INT
#define CPL_TYPE   int
#include "cpl_image_basic_body.h"
#undef CPL_TYPE_T
#undef CPL_TYPE

#define CPL_TYPE_T CPL_TYPE_FLOAT
#define CPL_TYPE   float
#include "cpl_image_basic_body.h"
#undef CPL_TYPE_T
#undef CPL_TYPE

#define CPL_TYPE_T CPL_TYPE_DOUBLE
#define CPL_TYPE   double
#include "cpl_image_basic_body.h"
#undef CPL_TYPE_T
#undef CPL_TYPE
        default:
            return cpl_error_set_(CPL_ERROR_TYPE_MISMATCH);
        }
        cpl_tools_add_flops( im1->nx * im1->ny );
        break;
    }
    case CPL_TYPE_DOUBLE: {
        double * p1 = (double *)im1->pixels;

        /* Switch on the second passed image type */
        switch (im2->type) {
#define CPL_TYPE_T CPL_TYPE_INT
#define CPL_TYPE   int
#include "cpl_image_basic_body.h"
#undef CPL_TYPE_T
#undef CPL_TYPE

#define CPL_TYPE_T CPL_TYPE_FLOAT
#define CPL_TYPE   float
#include "cpl_image_basic_body.h"
#undef CPL_TYPE_T
#undef CPL_TYPE

#define CPL_TYPE_T CPL_TYPE_DOUBLE
#define CPL_TYPE   double
#include "cpl_image_basic_body.h"
#undef CPL_TYPE_T
#undef CPL_TYPE

        default:
            return cpl_error_set_(CPL_ERROR_TYPE_MISMATCH);
        }
        cpl_tools_add_flops( im1->nx * im1->ny );
        break;
    }
    case CPL_TYPE_FLOAT_COMPLEX: {
        float complex * p1 = (float complex *)im1->pixels;

        /* Switch on the second passed image type */
        switch (im2->type) {
#define CPL_TYPE_T CPL_TYPE_INT
#define CPL_TYPE   int
#include "cpl_image_basic_body.h"
#undef CPL_TYPE_T
#undef CPL_TYPE

#define CPL_TYPE_T CPL_TYPE_FLOAT
#define CPL_TYPE   float
#include "cpl_image_basic_body.h"
#undef CPL_TYPE_T
#undef CPL_TYPE

#define CPL_TYPE_T CPL_TYPE_DOUBLE
#define CPL_TYPE   double
#include "cpl_image_basic_body.h"
#undef CPL_TYPE_T
#undef CPL_TYPE

#define CPL_TYPE_T CPL_TYPE_FLOAT_COMPLEX
#define CPL_TYPE   float complex
#include "cpl_image_basic_body.h"
#undef CPL_TYPE_T
#undef CPL_TYPE

#define CPL_TYPE_T CPL_TYPE_DOUBLE_COMPLEX
#define CPL_TYPE   double complex
#include "cpl_image_basic_body.h"
#undef CPL_TYPE_T
#undef CPL_TYPE

        default:
            return cpl_error_set_(CPL_ERROR_INVALID_TYPE);
        }
        cpl_tools_add_flops( im1->nx * im1->ny );
        break;
    }
    case CPL_TYPE_DOUBLE_COMPLEX: {
        double complex * p1 = (double complex *)im1->pixels;

        /* Switch on the second passed image type */
        switch (im2->type) {
#define CPL_TYPE_T CPL_TYPE_INT
#define CPL_TYPE   int
#include "cpl_image_basic_body.h"
#undef CPL_TYPE_T
#undef CPL_TYPE

#define CPL_TYPE_T CPL_TYPE_FLOAT
#define CPL_TYPE   float
#include "cpl_image_basic_body.h"
#undef CPL_TYPE_T
#undef CPL_TYPE

#define CPL_TYPE_T CPL_TYPE_DOUBLE
#define CPL_TYPE   double
#include "cpl_image_basic_body.h"
#undef CPL_TYPE_T
#undef CPL_TYPE

#define CPL_TYPE_T CPL_TYPE_FLOAT_COMPLEX
#define CPL_TYPE   float complex
#include "cpl_image_basic_body.h"
#undef CPL_TYPE_T
#undef CPL_TYPE

#define CPL_TYPE_T CPL_TYPE_DOUBLE_COMPLEX
#define CPL_TYPE   double complex
#include "cpl_image_basic_body.h"
#undef CPL_TYPE_T
#undef CPL_TYPE

        default:
            return cpl_error_set_(CPL_ERROR_INVALID_TYPE);
        }
        cpl_tools_add_flops( im1->nx * im1->ny );
        break;
    }
    default:
        return cpl_error_set_(CPL_ERROR_INVALID_TYPE);
    }

    /* Handle bad pixels map */
    if (im2->bpm != NULL) {
        if (im1->bpm == NULL) {
            im1->bpm = cpl_mask_duplicate(im2->bpm);
        } else {
            cpl_mask_or(im1->bpm, im2->bpm);
        }
    }

    return CPL_ERROR_NONE;

#undef CPL_OPERATION
#define CPL_OPERATION CPL_IMAGE_BASIC_ASSIGN_LOCAL

#elif CPL_OPERATION == CPL_IMAGE_BASIC_OP_SCALAR
    
    case CPL_TYPE_T:
    {
        CPL_TYPE     * pio     = (CPL_TYPE*)self->pixels;
        const CPL_TYPE cscalar = (const CPL_TYPE)scalar;
        const size_t   nxy     = (size_t)(self->nx * self->ny);
        size_t         i;

        assert( self->pixels );    
   
        for (i = 0; i < nxy; i++) {
            CPL_OPERATOR(pio[i], cscalar);
        }

#ifdef CPL_TYPE_IS_FPOINT
        cpl_tools_add_flops( nxy );
#endif
        break;
    }

#elif CPL_OPERATION == CPL_IMAGE_BASIC_SQRT
    
    case CPL_TYPE_T:
    {
        CPL_TYPE   * pio = (CPL_TYPE*)image->pixels;
        const size_t nxy = (size_t)(image->nx * image->ny);
        size_t       i;

        for (i=0; i < nxy; i++)
            pio[i] = sqrt(pio[i]);
#ifdef CPL_TYPE_IS_FPOINT
        cpl_tools_add_flops( image->nx * image->ny );
#endif
        break;
    }

#elif CPL_OPERATION == CPL_IMAGE_BASIC_THRESHOLD

    case CPL_TYPE_T:
    {
        CPL_TYPE   * pi = (CPL_TYPE*)image_in->pixels;
        const size_t nxy = (size_t)(image_in->nx * image_in->ny);
        size_t       i;

        for (i=0; i < nxy; i++) {
            if (pi[i] > hi_cut) pi[i] = (CPL_TYPE)assign_hi_cut;
            else if (pi[i] < lo_cut) pi[i] = (CPL_TYPE)assign_lo_cut;
        }
        break;
    } 

#elif CPL_OPERATION == CPL_IMAGE_BASIC_ABS

      case CPL_TYPE_T:
      {
          CPL_TYPE   * pio = (CPL_TYPE*)image->pixels;
          const size_t nxy = (size_t)(image->nx * image->ny);
          size_t       i;

          for (i = 0; i < nxy; i++)
              pio[i] = (CPL_TYPE)CPL_MATH_ABS(pio[i]);
#ifdef CPL_TYPE_IS_FPOINT
          cpl_tools_add_flops( image->nx * image->ny );
#endif
          break;
      }

#elif CPL_OPERATION == CPL_IMAGE_BASIC_AVERAGE

    
    case CPL_TYPE_T:
    {
        const size_t     nxy = (size_t)(image_1->nx * image_1->ny);
        const CPL_TYPE * pi1 = (const CPL_TYPE*)image_1->pixels;
        CPL_TYPE       * po = (CPL_TYPE *)cpl_malloc(nxy * sizeof(*po));
        size_t           i;

        /* Switch on second passed image type */
        switch (image_2->type) {
            case CPL_TYPE_INT:
                pii2 = (int*)image_2->pixels;
                for (i = 0; i < nxy; i++)
                    po[i] = (CPL_TYPE)(0.5 * (pi1[i] + pii2[i]));
                break;
            case CPL_TYPE_FLOAT:
                pfi2 = (float*)image_2->pixels;
                for (i = 0; i < nxy; i++)
                    po[i] = (CPL_TYPE)(0.5 * (pi1[i] + pfi2[i]));
                break;
            case CPL_TYPE_DOUBLE:
                pdi2 = (double*)image_2->pixels;
                for (i = 0; i < nxy; i++)
                    po[i] = (CPL_TYPE)(0.5 * (pi1[i] + pdi2[i]));
                break;
            default:
                cpl_free(po);
                (void)cpl_error_set_(CPL_ERROR_INVALID_TYPE);
                return NULL;
        }
        image_out = cpl_image_wrap_(image_1->nx, image_1->ny, CPL_TYPE_T, po);

#ifdef CPL_TYPE_IS_FPOINT
        cpl_tools_add_flops( 2 * image_out->nx * image_out->ny );
#endif
        break;
    }

#elif CPL_OPERATION == CPL_IMAGE_BASIC_EXTRACT

    case CPL_TYPE_T:
    {
        const size_t outlx = urx - llx + 1;
        const size_t outly = ury - lly + 1;

        /* Output pixel buffer */
        void         * po  = cpl_malloc(outlx * outly * sizeof(CPL_TYPE));

        if (cpl_tools_copy_window(po, in->pixels, sizeof(CPL_TYPE),
                                  in->nx, in->ny, llx, lly, urx, ury)) {
            cpl_free(po);
            (void)cpl_error_set_where_();
        } else {
            self = CPL_TYPE_ADD(cpl_image_wrap)(outlx, outly, (CPL_TYPE*)po);
        }

        break;
    }

#elif CPL_OPERATION == CPL_IMAGE_BASIC_EXTRACTROW

    case CPL_TYPE_T:
    {
        CPL_TYPE * pi = (CPL_TYPE*)image_in->pixels; 
        size_t i;

        for (i = 0; i < (size_t)image_in->nx; i++) {
            out_data[i] = (double)pi[i+(pos-1)*image_in->nx];
        }
        break;
    }

#elif CPL_OPERATION == CPL_IMAGE_BASIC_EXTRACTCOL

    case CPL_TYPE_T:
    {
        CPL_TYPE * pi = (CPL_TYPE*)image_in->pixels; 
        size_t i;

        for (i = 0; i < (size_t)image_in->ny; i++) {
            out_data[i] = (double)pi[pos-1+i*image_in->nx];
        }
        break;
    }

#elif CPL_OPERATION == CPL_IMAGE_BASIC_COLLAPSE_MEDIAN

    case CPL_TYPE_T:
    {
        /* Number of output pixels */
        const size_t     npix = direction ? self->ny : self->nx;

        /* Max number of input pixels in one median computation */
        const size_t     nmed = (direction ? self->nx : self->ny) - ndrop;

        const CPL_TYPE * pi  = (const CPL_TYPE*)self->pixels;
        CPL_TYPE       * po  = (CPL_TYPE*)cpl_malloc(npix * sizeof(CPL_TYPE));
        CPL_TYPE       * med = (CPL_TYPE*)cpl_malloc(nmed * sizeof(CPL_TYPE));
        cpl_binary     * bpm = NULL;
        const cpl_binary * bin = self->bpm
            ? cpl_mask_get_data_const(self->bpm) : NULL;
        cpl_boolean      isok = bin == NULL ? CPL_TRUE : CPL_FALSE;
        size_t           i, j;

        if (direction == 1) {
            /* Collapsing the image in the x direction */
            pi += drop_ll;
            if (bin == NULL) {
                for (j = 0; j < (size_t)(self->ny); j++, pi += self->nx) {
                    size_t k = 0;
                    for (i = 0; i < nmed; i++) {
                        med[k++] = pi[i];
                    }
                    po[j] = CPL_TYPE_ADD(cpl_tools_get_median)(med, k);
                }
            } else {
                bin += drop_ll;
                for (j = 0; j < (size_t)(self->ny); j++,
                         pi += self->nx, bin += self->nx) {
                    size_t k = 0;
                    for (i = 0; i < nmed; i++) {
                        if (!bin[i]) med[k++] = pi[i];
                    }
                    if (k == 0) {
                        if (bpm == NULL)
                            bpm = cpl_calloc((size_t)npix, sizeof(cpl_binary));
                        bpm[j] = CPL_BINARY_1;
                        po[j] = (CPL_TYPE)0;
                    } else {
                        po[j] = CPL_TYPE_ADD(cpl_tools_get_median)(med, k);
                        isok = CPL_TRUE;
                    }
                }
            }
        } else {
            /* Collapsing the image in the y direction */
            pi += drop_ll * self->nx;
            if (bin == NULL) {
                for (i = 0; i < (size_t)(self->nx); i++, pi++) {
                    size_t k = 0;
                    for (j = 0; j < nmed; j++) {
                        med[k++] = pi[j * self->nx];
                    }
                    po[i] = CPL_TYPE_ADD(cpl_tools_get_median)(med, k);
                }
            } else {
                bin += drop_ll * self->nx;
                for (i = 0; i < (size_t)(self->nx); i++, pi++, bin++) {
                    size_t k = 0;
                    for (j = 0; j < nmed; j++) {
                        if (!bin[j * self->nx]) med[k++] = pi[j * self->nx];
                    }
                    if (k == 0) {
                        if (bpm == NULL)
                            bpm = cpl_calloc((size_t)npix, sizeof(cpl_binary));
                        bpm[i] = CPL_BINARY_1;
                        po[i] = (CPL_TYPE)0;
                    } else {
                        po[i] = CPL_TYPE_ADD(cpl_tools_get_median)(med, k);
                        isok = CPL_TRUE;
                    }
                }
            }
        }
        cpl_free(med);

        if (isok) {
            other = direction ? CPL_TYPE_ADD(cpl_image_wrap)(1, self->ny, po)
                : CPL_TYPE_ADD(cpl_image_wrap)(self->nx, 1, po);
            if (bpm != NULL) {
                other->bpm = direction ? cpl_mask_wrap(1, self->ny, bpm)
                    : cpl_mask_wrap(self->nx, 1, bpm);
            }
        } else {
            cpl_free(po);
            cpl_free(bpm);
            (void)cpl_error_set(cpl_func, CPL_ERROR_DATA_NOT_FOUND);
        }

        break;
    }

#elif CPL_OPERATION == CPL_IMAGE_BASIC_ROTATE_INT_LOCAL

    case CPL_TYPE_T:
    {
        /* rot is 0, 1, 2 or 3. */
        switch(rot) {
            case 1: {
                CPL_TYPE * pi = (CPL_TYPE *)self->pixels;
                size_t   i,j;

                if (self->nx == self->ny) {
                    /* If self->nx is even,
                       then there is a multiple of 4 pixels to move.
                       If self->nx is odd,
                       then there is also a multiple of 4 pixels to move,
                       since the center pixel does not move. */
                    /* The first four pixels to move are the corner ones,
                       followed by their neighbors
                       - the last four pixels to move are the center ones. */
                    for (j = 0; j < (size_t)(self->ny/2); j++) {
                        for (i = j; i < (size_t)(self->nx-1-j); i++) {
                            const CPL_TYPE tmp = pi[i + j * self->nx];

                            pi[i + j * self->nx]
                                = pi[(self->ny-1-j) + i * self->nx];

                            pi[(self->ny-1-j) + i * self->nx]
                                = pi[(self->nx-1-i) + (self->ny-1-j) * self->nx];

                            pi[(self->nx-1-i) + (self->ny-1-j) * self->nx]
                                = pi[j + (self->nx-1-i) * self->nx];

                            pi[j + (self->nx-1-i) * self->nx] = tmp;
                        }
                    }
                } else if (self->nx == 1) {
                    /* No pixels need to move - just swap nx and ny */
                    self->nx = self->ny;
                    self->ny = 1;
                } else if (self->ny == 1) {
                    /* The image is a single horizontal row,
                       the order of the pixels needs to be reversed */
  
                    /* Flip the 1D-image and swap nx and ny */
                    CPL_TYPE_ADD(cpl_tools_flip)(pi, (size_t)self->nx);
                    self->ny = self->nx;
                    self->nx = 1;
                } else {
                    /* Duplicate the input image :-( */
                    cpl_image      * tmp = cpl_image_duplicate(self);
                    const CPL_TYPE * pt = (const CPL_TYPE *)tmp->pixels;

                    self->nx = tmp->ny;
                    self->ny = tmp->nx;
                    pi += ((self->ny)-1)* (self->nx);
                    for (j=0; j < (size_t)(self->nx); j++) {
                        for (i = 0; i < (size_t)(self->ny); i++) {
                            *pi = *pt++;
                            pi -= (self->nx);
                        }
                        pi += (self->nx)*(self->ny)+1;
                    }
                    cpl_image_delete(tmp);
                }
                break;
            }
            case 2: {
                /* A 180 degree rotation preserves the shape of the image,
                   and reverts the order of the pixels */

                CPL_TYPE_ADD(cpl_tools_flip)((CPL_TYPE *)self->pixels,
                                             (size_t)self->nx *
                                             (size_t)self->ny);
                break;
            }
            case 3: {
                CPL_TYPE * pi = (CPL_TYPE *)self->pixels;
                size_t   i,j;
                if (self->nx == self->ny) {
                    /* See case 1. */
                    for (j = 0; j < (size_t)(self->ny/2); j++) {
                        for (i = j; i < (size_t)(self->nx-1-j); i++) {
                            const CPL_TYPE tmp = pi[i + j * self->nx];

                            pi[i + j * self->nx]
                                = pi[j + (self->nx-1-i) * self->nx];

                            pi[j + (self->nx-1-i) * self->nx]
                                = pi[(self->nx-1-i) + (self->ny-1-j) * self->nx];

                            pi[(self->nx-1-i) + (self->ny-1-j) * self->nx]
                                = pi[(self->ny-1-j) + i * self->nx];

                            pi[(self->ny-1-j) + i * self->nx] = tmp;
                        }
                    }
                } else if (self->ny == 1) {
                    /* No pixels need to move - just swap nx and ny */
                    self->ny = self->nx;
                    self->nx = 1;
                } else if (self->nx == 1) {
                    /* The image is a single vertical solumn,
                       the order of the pixels needs to be reversed */

                    /* Flip the 1D-image and swap nx and ny */
                    CPL_TYPE_ADD(cpl_tools_flip)(pi, (size_t)self->ny);
                    self->nx = self->ny;
                    self->ny = 1;
                } else {
                    /* Duplicate the input image :-( */
                    cpl_image      * tmp = cpl_image_duplicate(self);
                    const CPL_TYPE * pt = (const CPL_TYPE *)tmp->pixels;

                    self->nx = tmp->ny;
                    self->ny = tmp->nx;
                    pi += (self->nx)-1;
                    for (j=0; j < (size_t)(self->nx); j++) {
                        for (i = 0; i < (size_t)(self->ny); i++) {
                            *pi = *pt++;
                            pi += (self->nx);
                        }
                        pi -= (self->nx)*(self->ny)+1;
                    }
                    cpl_image_delete(tmp);
                }
                break;
            }
            default:
                break;
        }
        break;
    }

#elif CPL_OPERATION == CPL_IMAGE_BASIC_FLIP_LOCAL

    case CPL_TYPE_T:
    {
        const size_t nx = im->nx;
        const size_t ny = im->ny;
        CPL_TYPE   * pi = (CPL_TYPE *)im->pixels;
        size_t       i, j;

        switch(angle) {
        case 0: {
            const size_t rowsize = (size_t)nx * sizeof(CPL_TYPE);
            CPL_TYPE     row[nx];
            CPL_TYPE *   pfirst = pi;
            CPL_TYPE *   plast  = pi + (ny-1) * nx;

            for (j = 0; j < ny/2; j++, pfirst += nx, plast -= nx) {
                (void)memcpy(row,    pfirst, rowsize);
                (void)memcpy(pfirst, plast,  rowsize);
                (void)memcpy(plast,  row,    rowsize);
            }
            break;
        }
        case 2: {

            for (j = 0; j < ny; j++, pi += nx) {
                CPL_TYPE_ADD(cpl_tools_flip)(pi, nx);
            }
            break;
        }
        case 1: {
            if (nx == ny) {
                CPL_TYPE * pt = pi;

                for (j = 0; j < nx; j++, pt += nx) {
                    for (i = 0; i < j; i++) {
                        const CPL_TYPE tmp = pt[i];
                        pt[i] = pi[j + i * nx];
                        pi[j + i * nx] = tmp;
                    }
                }
            } else {
                /* Duplicate the input image */
                cpl_image * tmp_im = cpl_image_duplicate(im);
                const CPL_TYPE * pt = (const CPL_TYPE *)tmp_im->pixels;

                im->nx = ny;
                im->ny = nx;
                for (j=0; j<nx; j++) {
                    for (i = 0; i<ny; i++) {
                        *pi++ = *pt;
                        pt += nx;
                    }
                    pt -= (nx*ny-1);
                }
                cpl_image_delete(tmp_im);
            }
            break;
        }
        case 3: {
            if (nx == ny) {
                CPL_TYPE * pt = pi;

                for (j = 0; j < nx; j++, pt += nx) {
                    for (i = 0; i < nx - j; i++) {
                        const CPL_TYPE tmp = pt[i];
                        pt[i] = pi[(nx - 1 - j) + (nx - 1 - i) * nx];
                        pi[(nx - 1 - j) + (nx - 1 - i) * nx] = tmp;
                    }
                }
            } else {
                /* Duplicate the input image */
                cpl_image * tmp_im = cpl_image_duplicate(im);
                const CPL_TYPE * pt = (const CPL_TYPE *)tmp_im->pixels;

                im->nx = ny;
                im->ny = nx;
                pt += (nx*ny-1);
                for (j=0; j<nx; j++) {
                    for (i = 0; i<ny; i++) {
                        *pi++ = *pt;
                        pt -= nx;
                    }
                    pt += (nx*ny-1);
                }
                cpl_image_delete(tmp_im);
            }
            break;
        }
        default:
            return cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);
        }
        break;
    }

#elif CPL_OPERATION == CPL_IMAGE_BASIC_MOVE_PIXELS

    case CPL_TYPE_T:
    {
        /* Duplicate the input image */
        cpl_image * tmp_im = cpl_image_duplicate(im);
        CPL_TYPE  * po;
        const CPL_TYPE  * pi;

        cpl_ensure_code(tmp_im, cpl_error_get_code());

        /* Get pointer to the data */
        pi = (const CPL_TYPE *)tmp_im->pixels;
        po = (CPL_TYPE *)im->pixels;
                        
        /* Move the pixels */
        for (j=0; j<nb_cut; j++) {
            for (i = 0; i<nb_cut; i++) {
                tile_x = (new_pos[i+j*nb_cut]-1) % nb_cut;
                tile_y = (new_pos[i+j*nb_cut]-1) / nb_cut;
                for (l=0; l<tile_sz_y; l++) {
                    for (k=0; k<tile_sz_x; k++) {
                        opos=(k+i*tile_sz_x) + im->nx*(l+j*tile_sz_y);
                        npos=(k+tile_x*tile_sz_x) + 
                            im->nx*(l+tile_y*tile_sz_y);
                        po[npos] = pi[opos];
                    }
                }
            }
        }
        cpl_image_delete(tmp_im);
        break;
    }

#else
#error "Undefined CPL Operation"
#endif

#undef CPL_TYPE
#undef CPL_TYPE_CONCAT
#undef CPL_TYPE_T
#undef CPL_MATH_ABS

#undef CPL_TYPE_ADD
#undef CPL_TYPE_IS_FPOINT

#endif
