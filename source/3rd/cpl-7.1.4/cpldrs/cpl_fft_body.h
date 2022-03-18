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
 * MERCHANTABILITY or FFTNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#define CPL_FFTW_ADD(a) CPL_CONCAT2X(CPL_FFTW, a)
#define CPL_TYPE_ADD(a) CPL_CONCAT2X(a, CPL_TYPE)
#define CPL_TYPE_ADD_CONST(a) CPL_CONCAT3X(a, CPL_TYPE, const)
#define CPL_TYPE_ADD_COMPLEX(a) CPL_CONCAT2X(a, CPL_TYPE_C)
#define CPL_TYPE_ADD_COMPLEX_CONST(a) CPL_CONCAT3X(a, CPL_TYPE_C, const)

static cpl_error_code CPL_TYPE_ADD(cpl_fft_image)(cpl_image          *,
                                                  const cpl_image    *,
                                                  cpl_fft_mode        ,
                                                  unsigned            ,
                                                  CPL_FFTW_ADD(plan) *,
                                                  CPL_FFTW_TYPE     **,
                                                  CPL_FFTW_TYPE     **,
                                                  cpl_boolean)

#ifdef CPL_HAVE_ATTR_NONNULL
    __attribute__((nonnull(6,7)))
#endif
    ;

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief Perform a FFT operation on an image of a specific type
  @param self    Pre-allocated output image of the given type
  @param other   Input image
  @param mode    CPL_FFT_FORWARD or CPL_FFT_BACKWARD, optionally CPL_FFT_NOSCALE
  @param rigor   FFTW_ESTIMATE, FFTW_MEASURE etc. Transform only w. _ESTIMATE
  @param pplan   NULL, or a pointer to keep the plan
  @param pbufin  A pointer to keep the input buffer
  @param pbufout A pointer to keep the output buffer
  @param is_last CPL_TRUE for the last call with the given pplan
  @return CPL_ERROR_NONE or the corresponding #_cpl_error_code_
  @see cpl_fft_image_()
  @note The precision for both images must be either double or float.
        When pplan is non-NULL, then the plan is destroyed when is_last is TRUE
 */
/*----------------------------------------------------------------------------*/
static cpl_error_code CPL_TYPE_ADD(cpl_fft_image)(cpl_image          * self,
                                                  const cpl_image    * other,
                                                  cpl_fft_mode         mode,
                                                  unsigned             rigor,
                                                  CPL_FFTW_ADD(plan) * pplan,
                                                  CPL_FFTW_TYPE     ** pbufin,
                                                  CPL_FFTW_TYPE     ** pbufout,
                                                  cpl_boolean          is_last)
{
    const cpl_type typin  = cpl_image_get_type(other);
    const cpl_type typout = cpl_image_get_type(self);
    const int nxin  = (int)cpl_image_get_size_x(other);
    const int nyin  = (int)cpl_image_get_size_y(other);
    const int nxout = (int)cpl_image_get_size_x(self);
    const int nxh   = ((mode & CPL_FFT_FORWARD) ? nxin : nxout) / 2 + 1;
    cpl_error_code error = CPL_ERROR_NONE;

    /* FIXME: This should be verified during configure and replaced by
       an assert() */
    cpl_ensure_code(sizeof(CPL_TYPE complex) == sizeof(CPL_FFTW_TYPE),
                    CPL_ERROR_UNSUPPORTED_MODE);

    if (mode & CPL_FFT_FORWARD) {
        CPL_FFTW_ADD(plan) pforw;
        CPL_FFTW_TYPE    * out_b = (CPL_FFTW_TYPE*)
            CPL_TYPE_ADD_COMPLEX(cpl_image_get_data)(self);
        CPL_FFTW_TYPE * out_bt;

        cpl_ensure_code(out_b != NULL, CPL_ERROR_TYPE_MISMATCH);
        /* Make sure mode contains only the supported flags */
        cpl_ensure_code(!(mode & ~(CPL_FFT_FORWARD | CPL_FFT_NOSCALE)),
                        CPL_ERROR_ILLEGAL_INPUT);

        if (typin & CPL_TYPE_COMPLEX) {
            CPL_FFTW_TYPE * in_b;
            CPL_FFTW_TYPE * in_bt;
            size_t alignmask;

            CPL_DIAG_PRAGMA_PUSH_IGN(-Wcast-qual);
            in_b = (CPL_FFTW_TYPE *)
                CPL_TYPE_ADD_COMPLEX_CONST(cpl_image_get_data)(other);
            CPL_DIAG_PRAGMA_POP;

            if (pplan != NULL && *pplan != NULL) {
                pforw = *pplan;
            } else {
#ifdef _OPENMP
#pragma omp critical(cpl_fft_fftw)
#endif
                {
                    /* Allocate transformation buffers.
                       Each is used iff the pixel buffer is not aligned.
                       When the caller is cpl_fft_imagelist() a subsequent call
                       may need the transformation buffer, so we always create
                       it although it may never be written to.
                       For the same reason we use  FFTW_PRESERVE_INPUT.
                    */

                    /* FIXME: In-place faster (or at least not slower) ? */
                    *pbufin  = CPL_FFTW_ADD(malloc)(nxin * sizeof(CPL_FFTW_TYPE)
                                                    * nyin);
                    *pbufout = CPL_FFTW_ADD(malloc)(nxin * sizeof(CPL_FFTW_TYPE)
                                                    * nyin);

                    /* FIXME: If unaligned then drop FFTW_PRESERVE_INPUT */
                    pforw = CPL_FFTW_ADD(plan_dft_2d)(nyin, nxin, *pbufin,
                                                      *pbufout, FFTW_FORWARD,
                                                      rigor
                                                      | FFTW_PRESERVE_INPUT);
                }

                if (pplan != NULL) *pplan = pforw;
            }
            alignmask = (size_t)(*pbufin) | (size_t)(*pbufout);

            in_bt = cpl_fft_aligned((void*)in_b, *pbufin, alignmask);

            if (in_bt == *pbufin) {
                memcpy(in_bt, in_b, nxin * sizeof(CPL_FFTW_TYPE) * nyin);
            }

            out_bt = in_b == out_b ?
                *pbufout : cpl_fft_aligned((void*)out_b, *pbufout, alignmask);

            CPL_FFTW_ADD(execute_dft)(pforw, in_bt, out_bt);

            if (out_bt == *pbufout) {
                memcpy(out_b, *pbufout, nxin * sizeof(CPL_FFTW_TYPE) * nyin);
            }

        } else {
            const CPL_TYPE * in_b =
                CPL_TYPE_ADD_CONST(cpl_image_get_data)(other);
            CPL_TYPE      * in_bt;
            size_t alignmask;

            /* For the real-to-complex transform, only the left half of
               the result is computed. The size of the output image may
               either match that, or the input buffer */

            if (pplan != NULL && *pplan != NULL) {

                pforw = *pplan;

            } else {

#ifdef _OPENMP
#pragma omp critical(cpl_fft_fftw)
#endif
                {
                    /* Allocate transformation buffers.
                       Each is used iff the pixel buffer is not aligned
                       - or in case of the output of the right size */
                    
                    *pbufin  = CPL_FFTW_ADD(malloc)(nxin * sizeof(CPL_TYPE)
                                                    * nyin);
                    *pbufout = CPL_FFTW_ADD(malloc)(nxh * sizeof(CPL_FFTW_TYPE)
                                                    * nyin);

                    /* FIXME: If unaligned then drop FFTW_PRESERVE_INPUT */
                    pforw = CPL_FFTW_ADD(plan_dft_r2c_2d)(nyin, nxin,
                                                          (CPL_TYPE*)*pbufin,
                                                          *pbufout, rigor
                                                          | FFTW_PRESERVE_INPUT);
                }
                if (pplan != NULL) *pplan = pforw;
            }

            alignmask = (size_t)(*pbufin) | (size_t)(*pbufout);

            CPL_DIAG_PRAGMA_PUSH_IGN(-Wcast-qual);
            in_bt = cpl_fft_aligned((void*)in_b, *pbufin, alignmask);
            CPL_DIAG_PRAGMA_POP;

            if (in_bt == (CPL_TYPE*)*pbufin) {
                memcpy(in_bt, in_b, nxin * sizeof(CPL_TYPE) * nyin);
            }

            out_bt = nxout == nxh ? cpl_fft_aligned((void*)out_b, *pbufout,
                                                    alignmask)
                : *pbufout;

            CPL_FFTW_ADD(execute_dft_r2c)(pforw, in_bt, out_bt);

            if (nxout != nxh) {
                /* Need to repack the transformed half */
                const CPL_FFTW_TYPE * out_bhj = *pbufout;
                CPL_FFTW_TYPE       * out_bj  = out_b;
                int                   j;

                for (j = 0; j < nyin; j++, out_bhj += nxh, out_bj += nxin) {
                    (void)memcpy(out_bj, out_bhj, nxh * sizeof(*out_bj));
                }
            } else if (out_bt == *pbufout) {
                /* For the real-to-complex transform, only the left half of
                   the transform is done. The output matches that,
                   but is not aligned. */
                (void)memcpy(out_b, out_bt, nxh * sizeof(CPL_FFTW_TYPE) * nyin);
            }
        }

        if (pplan == NULL || is_last) {
            double fl_add = 0.0, fl_mul = 0.0, fl_fma = 0.0;

#ifdef _OPENMP
#pragma omp critical(cpl_fft_fftw)
#endif
            {
                CPL_FFTW_ADD(flops)(pforw, &fl_add, &fl_mul, &fl_fma);
                CPL_FFTW_ADD(destroy_plan)(pforw);
                CPL_FFTW_ADD(free)(*pbufin);
                CPL_FFTW_ADD(free)(*pbufout);
            }
            cpl_tools_add_flops((cpl_flops)(fl_add + fl_mul + 2.0 * fl_fma));
        }
    } else if (mode & CPL_FFT_BACKWARD) {
        CPL_FFTW_ADD(plan) pback;
        const CPL_FFTW_TYPE * in_b = (const CPL_FFTW_TYPE *)
            CPL_TYPE_ADD_COMPLEX_CONST(cpl_image_get_data)(other);
        size_t alignmask;


        /* Make sure mode contains only the supported flags */
        cpl_ensure_code(!(mode & ~(CPL_FFT_BACKWARD | CPL_FFT_NOSCALE)),
                        CPL_ERROR_ILLEGAL_INPUT);
        cpl_ensure_code(typin & CPL_TYPE_COMPLEX,  CPL_ERROR_TYPE_MISMATCH);

        if (typout & CPL_TYPE_COMPLEX) {
            CPL_FFTW_TYPE * out_b =
                CPL_TYPE_ADD_COMPLEX(cpl_image_get_data)(self);
            CPL_FFTW_TYPE * out_bt;
            CPL_FFTW_TYPE * in_bt;


            if (pplan != NULL && *pplan != NULL) {
                pback = *pplan;
            } else {

#ifdef _OPENMP
#pragma omp critical(cpl_fft_fftw)
#endif
                {
                    /* Allocate transformation buffers.
                       Each is used iff the pixel buffer is not aligned */

                    /* FIXME: In-place faster (or at least not slower) ? */
                    *pbufin  = CPL_FFTW_ADD(malloc)(nxin * sizeof(CPL_FFTW_TYPE)
                                                    * nyin);
                    *pbufout = CPL_FFTW_ADD(malloc)(nxin * sizeof(CPL_FFTW_TYPE)
                                                    * nyin);

                    /* FIXME: If unaligned then drop FFTW_PRESERVE_INPUT */
                    pback = CPL_FFTW_ADD(plan_dft_2d)(nyin, nxin, *pbufin,
                                                      *pbufout, FFTW_BACKWARD,
                                                      FFTW_PRESERVE_INPUT
                                                      | rigor);
                }

                if (pplan != NULL) *pplan = pback;
            }

            alignmask = (size_t)(*pbufin) | (size_t)(*pbufout);

            CPL_DIAG_PRAGMA_PUSH_IGN(-Wcast-qual);
            in_bt = cpl_fft_aligned((void*)in_b, *pbufin, alignmask);
            CPL_DIAG_PRAGMA_POP;

            if (in_bt == *pbufin) {
                memcpy(in_bt, in_b, nxin * sizeof(CPL_FFTW_TYPE) * nyin);
            }

            out_bt = in_b == out_b ?
                *pbufout : cpl_fft_aligned((void*)out_b, *pbufout, alignmask);

            CPL_FFTW_ADD(execute_dft)(pback, in_bt, out_bt);

            if (out_bt == *pbufout) {
                memcpy(out_b, *pbufout, nxin * sizeof(CPL_FFTW_TYPE) * nyin);
            }
        } else {
            CPL_TYPE * out_b = CPL_TYPE_ADD(cpl_image_get_data)(self);
            CPL_TYPE * out_bt;

            /* FFTW always modifies the input array in the C2R transform,
               so pbufin is always required here */
            if (pplan != NULL && *pplan != NULL) {
                pback = *pplan;
            } else {

#ifdef _OPENMP
#pragma omp critical(cpl_fft_fftw)
#endif
                {

                    /* Allocate transformation buffers. The output buffer is
                       used iff the pixel buffer is not aligned */

                    *pbufin = CPL_FFTW_ADD(malloc)(nxh * sizeof(CPL_FFTW_TYPE)
                                                   * nyin);
                    *pbufout = CPL_FFTW_ADD(malloc)(nxout * sizeof(CPL_TYPE)
                                                    * nyin);
                    /* From http://www.fftw.org/doc/Planner-Flags.html (3.3.4)
                       (2014-12-11): for multi-dimensional c2r transforms,
                       however, no input-preserving algorithms are implemented
                       and the planner will return NULL if one is requested.
                    */

                    pback = CPL_FFTW_ADD(plan_dft_c2r_2d)(nyin, nxout, *pbufin,
                                                          (CPL_TYPE*)*pbufout,
                                                          FFTW_DESTROY_INPUT
                                                          | rigor);
                }

                if (pplan != NULL) *pplan = pback;
            }

            if (nxin != nxh) {
                /* For the complex-to-real transform, only the left half of
                   the input is transformed. It needs to be repacked first */
                const CPL_FFTW_TYPE * in_bj  = in_b;
                CPL_FFTW_TYPE       * in_bhj = *pbufin;
                int                   j;

                for (j = 0; j < nyin; j++, in_bhj += nxh, in_bj += nxin) {
                    (void)memcpy(in_bhj, in_bj, nxh * sizeof(*in_bhj));
                }
            } else {
                /* For the complex-to-real transform, only the left half of
                   the input is transformed. The input matches that. */

                (void)memcpy(*pbufin, in_b, nxh * sizeof(CPL_FFTW_TYPE) * nyin);
            }

            alignmask = (size_t)(*pbufin) | (size_t)(*pbufout);
            out_bt = cpl_fft_aligned((void*)out_b, *pbufout, alignmask);

            CPL_FFTW_ADD(execute_dft_c2r)(pback, *pbufin, out_bt);
            if (out_bt == (CPL_TYPE*)*pbufout) {
                memcpy(out_b, *pbufout, nxout * sizeof(CPL_TYPE) * nyin);
            }
        }

        if (pplan == NULL || is_last) {
            double fl_add = 0.0, fl_mul = 0.0, fl_fma = 0.0;
#ifdef _OPENMP
#pragma omp critical(cpl_fft_fftw)
#endif
            {
                CPL_FFTW_ADD(flops)(pback, &fl_add, &fl_mul, &fl_fma);
                CPL_FFTW_ADD(destroy_plan)(pback);
                CPL_FFTW_ADD(free)(*pbufin);
                CPL_FFTW_ADD(free)(*pbufout);
            }
            cpl_tools_add_flops((cpl_flops)(fl_add + fl_mul + 2.0 * fl_fma));
        }
        if (!(mode & CPL_FFT_NOSCALE)) {
            error = cpl_image_divide_scalar(self, (double)(nxout * nyin));
        }
    } else {
        error = CPL_ERROR_ILLEGAL_INPUT;
    }

   return cpl_error_set_(error); /* Set or propagate error, if any */
}

#undef CPL_TYPE_ADD
#undef CPL_TYPE_ADD_CONST
#undef CPL_TYPE_ADD_COMPLEX
#undef CPL_TYPE_ADD_COMPLEX_CONST
