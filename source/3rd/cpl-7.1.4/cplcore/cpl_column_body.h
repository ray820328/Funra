/* This file is part of the ESO Common Pipeline Library
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
 * The functions below are documented with Doxygen as usual,
 * but the preprocessor generated function names mean that
 * doxygen cannot actually parse it. So it is all ignored.
 * @cond
 */

static void ADDTYPE(cpl_column_pow)(cpl_column *, double) CPL_ATTR_NONNULL;


/*
 * @brief
 *   Private function to make the power of a column of the specific type.
 *
 * @param column   Target column.
 * @param exponent Constant exponent.
 *
 * @return Nothing
 * @see cpl_image_power()
 *
 * This private function is just a way to compact the code of other
 * higher level functions. No check is performed on input, assuming
 * that this was done elsewhere.
 */

static void ADDTYPE(cpl_column_pow)(cpl_column *column, double exponent)
{

    const int      myerrno   = errno;
    const cpl_size nullcount = cpl_column_count_invalid(column);
    const cpl_size length    = cpl_column_get_size(column);
    ELEM_TYPE      *fp       = ADDTYPE(cpl_column_get_data)(column);
    const int      *np       = nullcount == 0 ? NULL
        : cpl_column_get_data_invalid_const(column);


    if (nullcount == length)
        return;

    if ((cpl_size)((size_t)length) != length) {
        (void)cpl_error_set_message_(CPL_ERROR_UNSUPPORTED_MODE,
                                     "%" CPL_SIZE_FORMAT, length);
        return;
    }

    /* In spite of checks pow() may still overflow */
    errno = 0;

    if (exponent == -0.5) {
        /* Column value must be positive */
        for (size_t i = 0; i < (size_t)length; i++) {
            if (np == NULL || np[i] == 0) {
                ELEM_TYPE value = 0; /* Init avoids (false pos.) warning */
                if (fp[i] > 0.0) {
                    value = ELEM_CAST(1.0 / ELEM_SQRT(fp[i]));
                } else {
                    errno = EDOM; /* Use errno to reuse code */
                }
                if (errno) {
                    errno = 0;
                    cpl_column_set_invalid(column, i);
                } else {
                    fp[i] = value;
                }
            }
        }
#ifdef ELEM_CBRT
    } else if (exponent == -1.0 / 3.0) {
        /* Column value must be positive */
        for (size_t i = 0; i < (size_t)length; i++) {
            if (np == NULL || np[i] == 0) {
                ELEM_TYPE value = 0; /* Init avoids (false pos.) warning */
                if (fp[i] > 0.0) {
                    value = ELEM_CAST(1.0 / ELEM_CBRT(fp[i]));
                } else {
                    errno = EDOM; /* Use errno to reuse code */
                }
                if (errno) {
                    errno = 0;
                    cpl_column_set_invalid(column, i);
                } else {
                    fp[i] = value;
                }
            }
        }
#endif
    } else if (exponent < 0.0) {
        if (exponent != ceil(exponent)) {
            /* Column value must be positive */
            for (size_t i = 0; i < (size_t)length; i++) {
                if (np == NULL || np[i] == 0) {
                    ELEM_TYPE value = 0; /* Init avoids (false pos.) warning */
                    if (fp[i] > 0.0) {
                        value = ELEM_CAST(ELEM_POW(fp[i], exponent));
                    } else {
                        errno = EDOM; /* Use errno to reuse code */
                    }
                    if (errno) {
                        errno = 0;
                        cpl_column_set_invalid(column, i);
                    } else {
                        fp[i] = value;
                    }
                }
            }
        } else {
            /* Column value must be non-zero */
            for (size_t i = 0; i < (size_t)length; i++) {
                if (np == NULL || np[i] == 0) {
                    ELEM_TYPE value = 0; /* Init avoids (false pos.) warning */
                    if (fp[i] != 0.0) {
                        value = ELEM_CAST(cpl_tools_ipow((double)fp[i],
                                                         exponent));
                    } else {
                        errno = EDOM; /* Use errno to reuse code */
                    }
                    if (errno) {
                        errno = 0;
                        cpl_column_set_invalid(column, i);
                    } else {
                        fp[i] = value;
                    }
                }
            }
        }
    } else if (exponent == 0.5) {
        /* Column value must be non-negative */
        for (size_t i = 0; i < (size_t)length; i++) {
            if (np == NULL || np[i] == 0) {
                ELEM_TYPE value = 0; /* Init avoids (false pos.) warning */
                if (fp[i] >= 0.0) {
                    value = ELEM_CAST(ELEM_SQRT(fp[i]));
                } else {
                    errno = EDOM; /* Use errno to reuse code */
                }
                if (errno) {
                    errno = 0;
                    cpl_column_set_invalid(column, i);
                } else {
                    fp[i] = value;
                }
            }
        }
#ifdef ELEM_CBRT
    } else if (exponent == 1.0 / 3.0) {
        /* Column value may be anything */
        for (size_t i = 0; i < (size_t)length; i++) {
            if (np == NULL || np[i] == 0) {
                fp[i] = ELEM_CAST(ELEM_CBRT(fp[i]));
            }
        }
#endif
    } else if (exponent != ceil(exponent)) {
        /* Column value must be non-negative */
        for (size_t i = 0; i < (size_t)length; i++) {
            if (np == NULL || np[i] == 0) {
                ELEM_TYPE value = 0; /* Init avoids (false pos.) warning */
                if (fp[i] >= 0.0) {
                    value = ELEM_CAST(ELEM_POW(fp[i], exponent));
                } else {
                    errno = EDOM; /* Use errno to reuse code */
                }
                if (errno) {
                    errno = 0;
                    cpl_column_set_invalid(column, i);
                } else {
                    fp[i] = value;
                }
            }
        }
    } else if (exponent != 1.0) {
        /* Column value may be anything */

        for (size_t i = 0; i < (size_t)length; i++) {
            if (np == NULL || np[i] == 0) {
                fp[i] = ELEM_CAST(cpl_tools_ipow((double)fp[i],
                                                 exponent));
            }
        }

    }


    if (exponent != 1.0) {
        /* FIXME: Counts also bad pixels */
        cpl_tools_add_flops( length );
    }

    errno = myerrno;
}

/*
 *  @endcond
 */
