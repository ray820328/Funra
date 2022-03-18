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

/*
  The functions below are documented with Doxygen as usual,
  but the preprocessor generated function names mean that
  doxygen cannot actually parse it. So it is all ignored.
  @cond
 */

static cpl_error_code ADDTYPE(cpl_image_reject_value)(cpl_image *, cpl_value);


/*----------------------------------------------------------------------------*/
/**
  @internal
  @ingroup cpl_image
  @param    self   Input image to modify
  @param    mode   Bit field specifying which special value(s) to reject
  @see cpl_image_reject_value()
   
*/
/*----------------------------------------------------------------------------*/
static cpl_error_code ADDTYPE(cpl_image_reject_value)(cpl_image * self,
                                                      cpl_value mode)
{

    CPL_TYPE  *  data = (CPL_TYPE*)cpl_image_get_data(self);
    cpl_binary*  bpm  = NULL;
    const size_t npix = (size_t)cpl_image_get_size_x(self)
                      * (size_t)cpl_image_get_size_y(self);
    size_t i;
    cpl_value check = mode;

    cpl_ensure_code(mode != 1,    CPL_ERROR_INVALID_TYPE);
    cpl_ensure_code(mode != 0,    CPL_ERROR_UNSUPPORTED_MODE);

    check &= ~ ( CPL_VALUE_NOTFINITE | CPL_VALUE_ZERO);
    cpl_ensure_code(check == 0,   CPL_ERROR_UNSUPPORTED_MODE);

#ifdef CPL_TYPE_IS_INT
    if (mode & CPL_VALUE_ZERO) {
        for (i = 0; i < npix; i++) {
            if (data[i] == 0) {
                if (bpm == NULL) {
                    bpm = cpl_mask_get_data(cpl_image_get_bpm(self));
                }
                bpm[i] = CPL_BINARY_1;
            }
        }
    }
#else

    for (i = 0; i < npix; i++) {
        const int fptype = fpclassify(data[i]);
        if ((fptype == FP_ZERO && (mode & CPL_VALUE_ZERO)) ||
            (fptype == FP_NAN  && (mode & CPL_VALUE_NAN))  ||
            (fptype == FP_INFINITE  && (((mode & CPL_VALUE_PLUSINF) &&
                                         ((mode & CPL_VALUE_MINUSINF)
                                          || data[i] > 0.0)) ||
                                        ((mode & CPL_VALUE_MINUSINF) &&
                                         data[i] < 0.0)))) {

            if (bpm == NULL) {
                bpm = cpl_mask_get_data(cpl_image_get_bpm(self));
            }
            bpm[i] = CPL_BINARY_1;
        }
    }
#endif

    return CPL_ERROR_NONE;
}

/* @endcond
 */
