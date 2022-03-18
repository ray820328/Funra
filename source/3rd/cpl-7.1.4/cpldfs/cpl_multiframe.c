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
#  include <config.h>
#endif


#include <sys/types.h>
#include <regex.h>

#include <cxstring.h>

#include <cpl_error_impl.h>
#include "cpl_multiframe.h"


/**
 *  @defgroup cpl_regex Regular Expression Filter
 *  @ingroup cpl_multiframe
 *
 *  The module implements a regular expression filter type. The type
 *  @c cpl_regex is a compiled regular expression created with
 *  a given set of regular expression syntax options, and an optional
 *  negation of the result when it is applied to an input string.
 */

/**@{*/

typedef regex_t regex;
typedef enum _cpl_regex_syntax_option_ flag_type;


struct _cpl_regex_
{
        regex     _m_re;
        flag_type _m_flags;
        cxbool    _m_negated;
};



/**
 * @brief
 *   Create a new regular expression filter.
 *
 * @param expression  Regular expression.
 * @param negated     Negate the result when applying the filter.
 * @param flags       Regular expression syntax options.
 *
 * @return
 *   The function returns a newly allocated regular expression filter object,
 *   or @c NULL in case an error occurred.
 *
 * The function allocates a regular expression filter object and initializes
 * it with the compiled regular expression @em expression. If the flag
 * @em negated is set the result when applying the filter to an input string
 * is negated. The argument @em flags allows to specify regular expression
 * syntax options for the compilation of the regular expression.
 *
 * The returned regular expression filter object must be destroyed using the
 * destructor cpl_regex_delete().
 *
 * Note that the syntax option @c CPL_REGEX_NOSUBS is always set implicitly,
 * since the interface does not allow to retrieve this information.
 */

cpl_regex *
cpl_regex_new(const char *expression, int negated, flag_type flags)
{

    cpl_regex *self = cx_malloc(sizeof *self);

    if (self) {

        cxint _flags = REG_NOSUB;


        /*
         * Set flags. Note sub-expression matching is not supported
         */

        if (flags & CPL_REGEX_ICASE) {
            _flags = _flags | REG_ICASE;
        }

        if ((flags & CPL_REGEX_BASIC) && (flags & CPL_REGEX_EXTENDED)) {

            cx_free(self);
            cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);

            return NULL;

        }
        else {

            if (flags & CPL_REGEX_BASIC) {
                _flags = _flags & ~REG_EXTENDED;
            }

            if (flags & CPL_REGEX_EXTENDED) {
                _flags = _flags | REG_EXTENDED;
            }

        }


        cxint status = regcomp(&self->_m_re, expression, _flags);

        if (status) {
            cx_free(self);
            self = NULL;
        }
        else {
            self->_m_flags = flags | CPL_REGEX_NOSUBS;
            self->_m_negated = negated ? TRUE : FALSE;
        }

    }

    return self;

}


/**
 * @brief
 *   Destroys a regular expression filter object.
 *
 * @param self  The regular expression filter object.
 *
 * @return
 *   Nothing.
 *
 * The function destroys the given regular expression filter object @em self,
 * and deallocates the memory used. If the filter object @em self is @c NULL,
 * nothing is done and no error is set.
 */

void
cpl_regex_delete(cpl_regex *self)
{

    if (self) {
        regfree(&self->_m_re);
        cx_free(self);
    }

    return;

}


/**
 * @brief
 *   Compare a regular expression with a given character string.
 *
 * @param self    The regular expression filter object to apply.
 * @param string  The string to be tested.
 *
 * @return
 *   The function returns 0 if the filter object does not match the input
 *   string, and a non-zero value otherwise.
 *
 * The function compares the input string @em string with the regular
 * expression of the filter object @em self. The function returns a non-zero
 * value for positive matches, i.e. the regular expression matches the input
 * string @em string and the filter is not negated, or the regular expression
 * does not match the input string but the filter is negated. Otherwise the
 * function reports a negative match, i.e. returns @c 0.
 */

int
cpl_regex_apply(const cpl_regex *self, const char *string)
{

    cxint status = regexec(&self->_m_re, string, 0, 0, 0);

    cxbool match = (status == REG_NOMATCH) ? FALSE : TRUE;


    return self->_m_negated ? !match : match;

}


/**
 * @brief
 *   Test whether a regular expression filter is negated.
 *
 * @param self  The regular expression filter object to test.
 *
 * @return
 *   The function returns @c 0 if the filter @em self is not negated, and
 *   @c 1 otherwise.
 *
 * The function reports whether the filter @em self is negated or not.
 */

int
cpl_regex_is_negated(const cpl_regex *self)
{
    return self->_m_negated;
}


/**
 * @brief
 *   Toggle the negation state of a regular expression filter.
 *
 * @param self  The regular expression filter object to update.
 *
 * @return
 *   Nothing.
 *
 * The function toggles the negation state of the given regular expression
 * filter object @em self. If @em self is negated, it is not negated after
 * this function has been called, and vice versa.
 */

void
cpl_regex_negate(cpl_regex *self)
{
    self->_m_negated = !self->_m_negated;
}
/**@}*/



// DICB FITS keyword ranking

typedef struct _cpl_dicb_header_order_ cpl_dicb_header_order;

struct _cpl_dicb_header_order_
{
    const cxchar *key;
    cxsize length;
    cxuint rank;
};

static cpl_dicb_header_order _cpl_dicb_primary_ranking[] =
{
 {"SIMPLE ",  7, 0},
 {"XTENSION", 8, 0},
 {"BITPIX ",  7, 10},
 {"NAXIS ",   6, 11},
 {"NAXIS",    5, 12},
 {"GROUP ",   6, 20},
 {"PCOUNT ",  7, 30},
 {"GCOUNT ",  7, 31},
 {"EXTEND ",  7, 32},
 {"BZERO ",   6, 33},
 {"BSCALE ",  7, 34},
 {"BUNIT ",   6, 35},
 {"BLANK ",   6, 36},
 {"TFIELDS ", 8, 37},
 {"TBCOL",    5, 38},
 {"TFORM",    5, 39},
 {"TUNIT",    5, 40},
 {"INSTRUME", 8, 100},
 {"TELESCOP", 8, 101},
 {"OBJECT ",  7, 102},
 {"ORIGIN ",  7, 103},
 {"PI-COI ",  7, 104},
 {"EXPTIME ", 8, 105},
 {"RA ",      3, 106},
 {"DEC ",     4, 107},
 {"EQUINOX ", 8, 108},
 {"RADECSYS", 8, 109},
 {"MJD-OBS ", 8, 110},
 {"DATE-OBS", 8, 111},
 {"TIMESYS ", 8, 112},
 {"LST ",     4, 113},
 {"UTC ",     4, 114},
 {"WCSAXES ", 8, 200},
 {"CTYPE",    5, 201},
 {"CRVAL",    5, 202},
 {"CRPIX",    5, 203},
 {"CD",       2, 204},
 {"CUNIT",    5, 205},
 {"CUNIT",    5, 205},
 {"HDUCLASS", 8, 500},
 {"HDUCLAS",  7, 501},
 {"HDUDOC ",  7, 502},
 {"HDUVERS ", 7, 503},
 {"SCIDATA ", 7, 504},
 {"ERRDATA ", 7, 505},
 {"QUALDATA", 8, 506},
 {"QUALMASK", 8, 506},
 {"ORIGFILE", 8, 600},
 {"PIPEFILE", 8, 601},
 {"ARCFILE ", 8, 602},
 {"ZHECKSUM", 8, 700},
 {"DATASUM ", 8, 701},
 {"ZDATASUM", 8, 701},
 {"DATAMD5 ", 8, 702},
 {"INHERIT ", 8, 800},
 {"HISTORY ", 8, 1001},
 {"COMMENT ", 8, 1002},
 {"        ", 8, 1003}
};

static cpl_dicb_header_order _cpl_dicb_hierarch_ranking[] =
{
 {"ESO DPR ",  8, 900},
 {"ESO OBS ",  8, 901},
 {"ESO TPL ",  8, 902},
 {"ESO GEN ",  8, 903},
 {"ESO TEL ",  8, 904},
 {"ESO ADA ",  8, 905},
 {"ESO INS ",  8, 906},
 {"ESO DET ",  8, 907},
 {"ESO DET1 ", 9, 907},
 {"ESO DET2 ", 9, 908},
 {"ESO PRO ",  8, 909},
 {"ESO LOG ",  8, 910},
 {"ESO QC ",   7, 911},
 {"ESO DRS ",  8, 912}
};


inline static cxuint
_cpl_dicb_header_get_keyword_rank(const cxchar *cstr)
{

    cxuint rank = 0;

    cxsize ngroups;

    cpl_dicb_header_order *ranks;


    if (strncmp(cstr, "HIERARCH ", 9) != 0) {

        rank = 899;
        ranks = _cpl_dicb_primary_ranking;
        ngroups = CX_N_ELEMENTS(_cpl_dicb_primary_ranking);

    }
    else {

        /*
         * add byte offset to skip 'HIERARCH ' prefix in header cards.
         */

        cstr += 9;

        rank = 999;
        ranks = _cpl_dicb_hierarch_ranking;
        ngroups = CX_N_ELEMENTS(_cpl_dicb_hierarch_ranking);

    }


    cxsize ig;

    for (ig = 0; ig < ngroups; ++ig) {

        if (strncmp(cstr, ranks[ig].key, ranks[ig].length) == 0) {
            rank = ranks[ig].rank;
            break;
        }

    }

    return rank;

}


// FITS card implementation

#include <string.h>

#include <fitsio.h>

#include <cxmessages.h>
#include <cxstring.h>



typedef struct _cpl_fitscard_ cpl_fitscard;

struct _cpl_fitscard_
{
    cx_string *_m_record;
};


typedef cxint (*cpl_fitscard_compare_func)(cpl_fitscard *, cpl_fitscard *);


static const cxchar *const _cpl_fits_keyword_traits =
        "._-ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";


inline static cxint
_cpl_fits_parse_keyname(cx_string *keyname, const cxchar *name)
{

    cxsize length = strlen(name);

    cx_string *_keyname;


    if ((length == 0) || (strncmp(name, "        ", 8) == 0)) {
        _keyname = cx_string_create("        ");
    }
    else if (strncmp(name, "COMMENT", 7) == 0) {
        _keyname = cx_string_create("COMMENT ");
    }
    else if (strncmp(name, "HISTORY", 7) == 0) {
        _keyname = cx_string_create("HISTORY ");
    }
    else if (strncmp(name, "HIERARCH ", 9) == 0) {

        if (length <= 9) {
            return -1;
        }

        _keyname = cx_string_create(name);
        cx_string_replace_character(_keyname, 9, cx_string_size(_keyname),
                                    '.', ' ');

        if (cx_string_get(_keyname)[length - 1] != ' ') {
            cx_string_append(_keyname, " ");
        }

    }
    else if ((strchr(name, ' ') != 0) || (strchr(name, '.') != 0)) {

        _keyname = cx_string_create("HIERARCH ");
        cx_string_append(_keyname, name);
        cx_string_replace_character(_keyname, 0, cx_string_size(_keyname),
                                    '.', ' ');

        if (cx_string_get(_keyname)[cx_string_size(_keyname) - 1] != ' ') {
            cx_string_append(_keyname, " ");
        }

    }
    else {

        if (strlen(name) > 8) {
            return -2;
        }

        _keyname = cx_string_create(name);

        cxint padding = 8 - strlen(name);

        if (padding > 0) {
            cx_string_extend(_keyname, padding, ' ');
        }

    }

    cx_string_set(keyname, cx_string_get(_keyname));
    cx_string_delete(_keyname);

    return 0;

}


inline static cxint
_cpl_fits_get_keyname(const cxchar *card, cx_string *keyname, int *status)
{

    if (*status) {
        return *status;
    }

    cxchar name[FLEN_KEYWORD];

    cxint length;

    fits_get_keyname((cxchar *)card, name, &length, status);

    if (*status) {
        return *status;
    }

    if (_cpl_fits_parse_keyname(keyname, name)) {
        *status = -1;
    }

    return *status;

}


inline static cxint
_cpl_fits_format_card(cx_string *record, const cxchar *keyname,
                      const cxchar *value, const cxchar *comment)
{

    if (strlen(keyname) == 0) {
        return -1;
    }

    if (strlen(keyname) >= FLEN_KEYWORD) {
        return -2;
    }

    if (value && (strlen(value) >= FLEN_VALUE)) {
        return -3;
    }

    if (comment && (strlen(comment) >= FLEN_COMMENT)) {
        return -4;
    }


    cxbool has_value   = value != NULL;
    cxbool has_comment = (comment != NULL) && (strlen(comment) > 0);
    cxbool is_comment  = FALSE;


    /*
     *  Fill in the keyword name
     */

     cx_string *_record = cx_string_create(keyname);

     if (strncmp(keyname, "        ", 8) == 0) {
         is_comment = TRUE;
     }
     else if (strncmp(keyname, "COMMENT", 7) == 0) {
         is_comment = TRUE;
     }
     else if (strncmp(keyname, "HISTORY", 7) == 0) {
         is_comment = TRUE;
     }


     /*
      * Add value indicator if needed
      */

     if (!is_comment) {
         cx_string_append(_record, "= ");
     }


     /*
      *  Add the value if it is present
      */

     if (has_value) {

         cxsize length = strlen(value);


         if (value[0] == '\'') {

             if (value[length - 1] != '\'') {
                 cx_string_delete(_record);
                 return -5;
             }


             cxchar *_s = cx_calloc(length - 1, sizeof(cxchar));
             memcpy(_s, &value[1], length - 2);

             cx_string *s = cx_string_create(_s);

             cx_free(_s);

             _s = (cxchar *)cx_string_get(s);


             /*
              *  Expand single quote characters if necessary
              */

             while ((_s = strchr(_s, '\'')) != NULL) {

                 cxsize pos = _s - cx_string_get(s);

                 if (pos == cx_string_size(s) - 1) {

                     cx_string_append(s, "'");
                     _s = (cxchar *)cx_string_get(s) + pos + 1;

                 }
                 else {

                     if (*(_s + 1) != '\'') {

                         cx_string_insert(s, pos, "'");
                         _s = (cxchar *)cx_string_get(s) + pos + 2;

                     }

                 }

             }


             /*
              * Fixed format strings are required to have at least 8
              * characters. Thus they may have to be padded with spaces.
              * This does not actually apply to hierarchical keywords,
              * which uses the free format convention. However cfitsio
              * also applies the padding to hierarchical keywords,
              * which should not be an issue since trailing blanks are not
              * significant.
              * Therefore the padding is applied here to all string values.
              */

             cxint padding = 8 - cx_string_size(s);

             if (padding > 0) {
                 cx_string_extend(s, padding, ' ');
             }

             cx_string_append(_record, "'");
             cx_string_append(_record, cx_string_get(s));
             cx_string_append(_record, "'");

             cx_string_delete(s);

         }
         else if (value[0] == '(') {

             if (value[length - 1] != ')') {
                 return -6;
             }

             cx_string_append(_record, value);

         }
         else if ((value[0] == 'T' || value[0] == 'F') && (length == 1)) {

             /*
              * The value of a logical keyword must appear in byte 30
              * of the card.
              */

             cxint padding = 29 - cx_string_size(_record);

             if (padding > 0) {
                 cx_string_extend(_record, padding, ' ');
             }

             cx_string_append(_record, value);

         }
         else {

             /*
              * Fixed format numerical values occupy the bytes 11 through 30,
              * and they are right justified in that field. Free format
              * numerical values may appear anywhere starting at byte 11.
              * With cfitsio the values are right justified at byte 30 unless
              * either the keyword name or the value is to long to fit in byte
              * 11 through 30.
              * This behavior is replicated here.
              */

             cxint padding = 30 - cx_string_size(_record) - length;

             if (padding > 0) {
                 cx_string_extend(_record, padding, ' ');
             }

             cx_string_append(_record, value);

         }

         if (cx_string_size(_record) > FLEN_CARD - 1) {
             cx_string_delete(_record);
             return -7;
         }

     }


     /*
      *  Add the optional comment
      */

     if (has_comment) {

         if (is_comment) {
             cx_string_append(_record, comment);
         }
         else {

             /*
              * The keyword comment indicator ' / ' should appear in byte 32
              * of the card, or later. If no character of the comment would
              * appear in the final card, due to its size constraint, the whole
              * comment, including the comment indicator, is dropped.
              */

             if ((FLEN_CARD - 1) - cx_string_size(_record) > 3) {

                 cxint padding = 30 - cx_string_size(_record);

                 if (padding > 0) {
                     cx_string_extend(_record, padding, ' ');
                 }

                 cx_string_append(_record, " / ");
                 cx_string_append(_record, comment);

             }

         }

     }

     cx_string_truncate(_record, FLEN_CARD);
     cx_string_set(record, cx_string_get(_record));

     cx_string_delete(_record);

     return 0;

}


inline static cxssize
_cpl_fitsfile_find_extension(fitsfile *file, const cxchar *name, cxsize version)
{

    cxint status = 0;

    cxint chdu = -1;

    fits_movnam_hdu(file, ANY_HDU, (cxchar *)name, version, &status);

    if (status) {
        return -1;
    }

    fits_get_hdu_num(file, &chdu);

    return chdu - 1;

}


inline static cxssize
_cpl_fits_find_extension(const cxchar *filename, const cxchar *name,
                         cxsize version)
{

    cxint status = 0;

    cxssize chdu = -1;

    fitsfile *fp = NULL;

    status = fits_open_diskfile(&fp, filename, READONLY, &status);

    if (status) {
        return -1;
    }

    chdu = _cpl_fitsfile_find_extension(fp, name, version);

    fits_close_file(fp, &status);

    return chdu;

}


inline static cpl_fitscard *
_cpl_fitscard_new(const cxchar *record)
{

    cpl_fitscard *self = NULL;


    if (record && (strlen(record) < FLEN_CARD)) {

        self = cx_malloc(sizeof *self);
        self->_m_record = cx_string_create(record);

    }

    return self;

}


inline static void
_cpl_fitscard_delete(cpl_fitscard *self)
{

    if (self) {
        cx_string_delete(self->_m_record);
        cx_free(self);
    }

    return;

}


inline static const cx_string *
_cpl_fitscard_get_card(const cpl_fitscard *self)
{
    cx_assert(self != NULL);
    return self->_m_record;
}


inline static cpl_fitscard *
_cpl_fitscard_set_card(cpl_fitscard *self, const cxchar *record)
{

    cx_assert(self != NULL);
    cx_assert(record != NULL);

    if (strlen(record) >= FLEN_CARD) {
        return NULL;
    }

    cx_string_set(self->_m_record, record);

    return self;

}


inline static cx_string *
_cpl_fitscard_get_key(const cpl_fitscard *self)
{

    cx_assert(self != NULL);


    cxchar keyname[FLEN_KEYWORD];

    cxint keylen = 0;
    cxint status = 0;


    fits_get_keyname((cxchar *)cx_string_get(self->_m_record), keyname,
                     &keylen, &status);

    if (status) {
        return NULL;
    }

    return cx_string_create(keyname);

}


inline static cx_string *
_cpl_fitscard_get_value(const cpl_fitscard *self)
{

    cx_assert(self != NULL);


    cxchar value[FLEN_VALUE];

    cxint status = 0;


    fits_parse_value((cxchar *)cx_string_get(self->_m_record), value,
                     0, &status);

    if (status)
    {
        return NULL;
    }

    return cx_string_create(value);

}


inline static cx_string *
_cpl_fitscard_get_comment(const cpl_fitscard *self)
{

    cx_assert(self != NULL);


    cxchar value[FLEN_VALUE];
    cxchar comment[FLEN_COMMENT];

    cxint status = 0;


    /*
     *  Even if one is only interested in the comment, the value must
     *  always be retrieved (as the function name suggests).
     */

    fits_parse_value((cxchar *)cx_string_get(self->_m_record), value,
                     comment, &status);

    if (status)
    {
        return NULL;
    }

    return cx_string_create(comment);

}


inline static cxint
_cpl_fitscard_set_key(cpl_fitscard *self, const cxchar *keyname)
{

    cx_assert(self != NULL);
    cx_assert(keyname != NULL);


    cxint status = 0;

    cx_string *_keyname = cx_string_create(keyname);


    if ((cx_string_find_first_not_of(_keyname, _cpl_fits_keyword_traits) !=
            cx_string_size(_keyname)) || (keyname[0] == '.')) {
        return BAD_KEYCHAR;
    }


    status = _cpl_fits_parse_keyname(_keyname, keyname);

    if (status) {
        cx_string_delete(_keyname);
        return status;
    }


    /*
     * Reassemble the record using the new keyword name, the value and
     * the comment. The value and comment are parsed to allow to
     * properly check for errors or FITS standard violations.
     */


    cxchar value[FLEN_VALUE];
    cxchar comment[FLEN_COMMENT];

    fits_parse_value((cxchar *)cx_string_get(self->_m_record), value,
                     comment, &status);

    if (status)
    {
        cx_string_delete(_keyname);
        return status;
    }

    cx_string *record = cx_string_new();

    status = _cpl_fits_format_card(record, cx_string_get(_keyname),
                                   value, comment);

    if (status) {

        cx_string_delete(record);
        cx_string_delete(_keyname);

        return status;

    }

    cx_string_set(self->_m_record, cx_string_get(record));

    cx_string_delete(record);
    cx_string_delete(_keyname);

    return 0;

}


inline static cxint
_cpl_fitscard_set_value(cpl_fitscard *self, const cxchar *value,
                        const cxchar *comment)
{

    cx_assert(self != NULL);
    cx_assert(value != NULL);


    cxint status = 0;

    cx_string *keyname = cx_string_new();


    _cpl_fits_get_keyname(cx_string_get(self->_m_record), keyname, &status);

    if (status) {
        cx_string_delete(keyname);
        return status;
    }


    cx_string *record = cx_string_new();

    status = _cpl_fits_format_card(record, cx_string_get(keyname),
                                   value, comment);

    if (status) {

        cx_string_delete(record);
        cx_string_delete(keyname);

        return status;

    }

    cx_string_set(self->_m_record, cx_string_get(record));

    cx_string_delete(record);
    cx_string_delete(keyname);

    return 0;

}


inline static cxint
_cpl_fitscard_set_comment(cpl_fitscard *self, const cxchar *comment)
{

    cx_assert(self != NULL);


    cxchar _value[FLEN_VALUE];

    cxint status = 0;

    cx_string *keyname = cx_string_new();


    _cpl_fits_get_keyname(cx_string_get(self->_m_record), keyname, &status);

    if (status) {
        cx_string_delete(keyname);
        return status;
    }

    fits_parse_value((cxchar *)cx_string_get(self->_m_record), _value,
                     0, &status);

    if (status)
    {
        cx_string_delete(keyname);
        return status;
    }


    cx_string *record = cx_string_new();

    status = _cpl_fits_format_card(record, cx_string_get(keyname),
                                   _value, comment);

    if (status) {

        cx_string_delete(record);
        cx_string_delete(keyname);

        return status;

    }

    cx_string_set(self->_m_record, cx_string_get(record));

    cx_string_delete(record);
    cx_string_delete(keyname);

    return 0;

}


inline static cxint
_cpl_fitscard_compare_dicb(cpl_fitscard *a, cpl_fitscard *b)
{
    cx_assert((a != NULL) && (b != NULL));

    const cxchar *_a = cx_string_get(_cpl_fitscard_get_card(a));
    const cxchar *_b = cx_string_get(_cpl_fitscard_get_card(b));

    cxuint ra = _cpl_dicb_header_get_keyword_rank(_a);
    cxuint rb = _cpl_dicb_header_get_keyword_rank(_b);

    if (ra == rb) {
        return strcmp(_a, _b);
    }

    return (ra < rb) ? -1 : 1;

}



// FITS header implementation

#include <cxdeque.h>


typedef struct _cpl_fitsheader_ cpl_fitsheader;

struct _cpl_fitsheader_
{
        cx_deque *_m_records;
};


inline static cpl_fitsheader *
_cpl_fitsheader_new(void)
{

    cpl_fitsheader *self = cx_malloc(sizeof *self);

    if (self) {

        self->_m_records = cx_deque_new();

        if (!self->_m_records) {
            cx_free(self);
            self = NULL;
        }

    }

    return self;

}


inline static cpl_fitsheader *
_cpl_fitsheader_duplicate(const cpl_fitsheader *other)
{

    cx_assert(other != NULL);

    cpl_fitsheader *self = _cpl_fitsheader_new();


    if (self) {

        cx_deque *cards = other->_m_records;

        cxsize ic;
        cxsize sz = cx_deque_size(cards);


        for (ic = 0; ic < sz; ++ic) {

            cpl_fitscard *card = cx_deque_get(cards, ic);
            cpl_fitscard *_card = _cpl_fitscard_new(cx_string_get(_cpl_fitscard_get_card(card)));

            cx_deque_push_back(self->_m_records, _card);

        }

    }

    return self;

}


inline static void
_cpl_fitsheader_delete(cpl_fitsheader *self)
{
    if (self) {
        cx_deque_destroy(self->_m_records, (cx_free_func)_cpl_fitscard_delete);
        cx_free(self);
    }

    return;
}


inline static cxsize
_cpl_fitsheader_get_size(const cpl_fitsheader *self)
{

    cx_assert(self != NULL);

    return cx_deque_size(self->_m_records);

}


inline static cpl_fitscard *
_cpl_fitsheader_get(cpl_fitsheader *self, cxsize irecord)
{

    cx_assert(self != NULL);
    cx_assert(irecord < cx_deque_size(self->_m_records));

    cpl_fitscard *card = cx_deque_get(self->_m_records, irecord);
    return card;

}


inline static const cpl_fitscard *
_cpl_fitsheader_get_const(const cpl_fitsheader *self, cxsize irecord)
{

    cx_assert(self != NULL);
    cx_assert(irecord < cx_deque_size(self->_m_records));

    return _cpl_fitsheader_get((cpl_fitsheader *)self, irecord);

}


inline static cpl_fitscard *
_cpl_fitsheader_find(cpl_fitsheader *self, const cxchar *keyname)
{

    cx_assert(self != NULL);
    cx_assert(keyname != NULL);

    cx_deque *cards = self->_m_records;

    cxsize ic;
    cxsize sz = cx_deque_size(cards);

    cpl_fitscard *card = NULL;


    for (ic = 0; ic < sz; ++ic) {

        cpl_fitscard *_card = cx_deque_get(cards, ic);

        cx_string *_keyname = _cpl_fitscard_get_key(_card);

        cxint result = strcmp(cx_string_get(_keyname), keyname);


        cx_string_delete(_keyname);

        if (result == 0) {
            card = _card;
            break;
        }

    }

    return card;

}


inline static const cpl_fitscard *
_cpl_fitsheader_find_const(const cpl_fitsheader *self, const cxchar *keyname)
{

    cx_assert(self != NULL);
    cx_assert(keyname != NULL);

    return _cpl_fitsheader_find((cpl_fitsheader *)self, keyname);

}


inline static cxsize
_cpl_fitsheader_find_card(const cpl_fitsheader *self, const cpl_fitscard *card)
{

    cx_assert(self != NULL);
    cx_assert(card != NULL);


    cx_deque *cards = self->_m_records;

    cxsize ic;
    cxsize sz = cx_deque_size(cards);
    cxsize pos = sz;

    cx_string *keyname = _cpl_fitscard_get_key(card);


    for (ic = 0; ic < sz; ++ic) {

        cpl_fitscard *_card = cx_deque_get(cards, ic);

        cx_string *_keyname = _cpl_fitscard_get_key(_card);

        cxint result = cx_string_compare(_keyname, keyname);


        cx_string_delete(_keyname);

        if (result == 0) {
            pos = ic;
            break;
        }

    }

    cx_string_delete(keyname);

    return pos;

}


inline static cxsize
_cpl_fitsheader_get_position(const cpl_fitsheader *self, const cxchar *keyname)
{

    cx_assert(self != NULL);
    cx_assert(keyname != NULL);


    cx_deque *cards = self->_m_records;

    cxsize ic;
    cxsize sz = cx_deque_size(cards);
    cxsize pos = sz;

    for (ic = 0; ic < sz; ++ic) {

        cpl_fitscard *_card = cx_deque_get(cards, ic);

        cx_string *_keyname = _cpl_fitscard_get_key(_card);

        cxint result = strcmp(cx_string_get(_keyname), keyname);


        cx_string_delete(_keyname);

        if (result == 0) {
            pos = ic;
            break;
        }

    }

    return pos;

}


inline static cpl_fitsheader *
_cpl_fitsheader_create_from_file(fitsfile *file, cxsize position)
{

    cx_assert(file != NULL);


    cxint status   = 0;
    cxint nrecords = 0;


    fits_movabs_hdu(file, position + 1, 0, &status);
    fits_read_record(file, 0, NULL, &status);

    fits_get_hdrspace(file, &nrecords, 0, &status);

    if (status) {
        return NULL;
    }


    cpl_fitsheader *self = _cpl_fitsheader_new();

    if (nrecords > 0) {

        for (cxint i = 0; i < nrecords; ++i) {

            cxchar record[FLEN_CARD];

            fits_read_record(file, i + 1, record, &status);

            if (!status) {
                cx_deque_push_back(self->_m_records, _cpl_fitscard_new(record));
            }

        }
    }

    if (status) {
        _cpl_fitsheader_delete(self);
        return NULL;
    }

    return self;

}


inline static cpl_fitsheader *
_cpl_fitsheader_create(const cxchar *filename, cxsize position)
{

    cx_assert(filename != NULL);


    cxint status = 0;

    fitsfile *ifile = 0;

    fits_open_diskfile(&ifile, filename, READONLY, &status);

    if (status) {
        return NULL;
    }


    cpl_fitsheader *self = _cpl_fitsheader_create_from_file(ifile, position);

    fits_close_file(ifile, &status);

    if (self == NULL) {
        return NULL;
    }

    return self;

}


inline static cpl_fitsheader *
_cpl_fitsheader_create_from_filter(const cpl_fitsheader *other,
                                   const cpl_regex *filter)
{

    cx_assert(other != NULL);
    cx_assert(filter != NULL);


    cx_deque *cards = other->_m_records;

    cxsize ic;
    cxsize sz = cx_deque_size(cards);

    cpl_fitsheader *self = _cpl_fitsheader_new();


    for (ic = 0; ic < sz; ++ic) {

        cpl_fitscard *card = cx_deque_get(cards, ic);

        const cxchar *record = cx_string_get(_cpl_fitscard_get_card(card));


        if (cpl_regex_apply(filter, record)) {

            cpl_fitscard *_card = _cpl_fitscard_new(record);

            cx_deque_push_back(self->_m_records, _card);

        }

    }

    return self;

}


inline static cxint
_cpl_fitsheader_append(cpl_fitsheader *self, cpl_fitscard *card)
{

    cx_assert(self != NULL);
    cx_assert(card != NULL);

    cx_deque_push_back(self->_m_records, card);

    return 0;

}


inline static cxint
_cpl_fitsheader_insert(cpl_fitsheader *self, cpl_fitscard *card,
                       cxsize irecord)
{

    cx_assert(self != NULL);
    cx_assert(card != NULL);
    cx_assert(irecord < cx_deque_size(self->_m_records));

    cx_deque_insert(self->_m_records, irecord, card);

    return 0;

}


inline static cpl_fitscard *
_cpl_fitsheader_remove(cpl_fitsheader *self, cxsize irecord)
{

    cx_assert(self != NULL);
    cx_assert(irecord < cx_deque_size(self->_m_records));

    cpl_fitscard *card = cx_deque_extract(self->_m_records, irecord);

    return card;

}


inline static cpl_fitscard *
_cpl_fitsheader_remove_card(cpl_fitsheader *self, cpl_fitscard *card)
{

    cx_assert(self != NULL);
    cx_assert(card != NULL);

    cxsize irecord = _cpl_fitsheader_find_card(self, card);

    cpl_fitscard *_card = cx_deque_extract(self->_m_records, irecord);

    return _card;

}


inline static cxint
_cpl_fitsheader_join(cpl_fitsheader *self, const cpl_fitsheader *other)
{

    cx_assert(self != NULL);
    cx_assert(other != NULL);


    cx_deque *cards = other->_m_records;

    cxsize ic;
    cxsize sz = cx_deque_size(cards);


    for (ic = 0; ic < sz; ++ic) {

        cpl_fitscard *card = cx_deque_get(cards, ic);

        const cxchar *record = cx_string_get(_cpl_fitscard_get_card(card));

        cpl_fitscard *_card = _cpl_fitscard_new(record);


        cx_deque_push_back(self->_m_records, _card);

    }

    return 0;

}


inline static cxint
_cpl_fitsheader_sort(cpl_fitsheader *self, cpl_fitscard_compare_func cmp)
{

    cx_assert(self != 0);
    cx_assert(cmp != NULL);

    cx_deque_sort(self->_m_records, (cx_compare_func)cmp);

    return 0;

}


#if 0

static void
_cpl_fitsheader_dump(const cpl_fitsheader *self, FILE *os)
{

    if (self) {

        cxsize sz = cx_deque_size(self->_m_records);

        for (cxsize i = 0; i < sz; ++i) {

            const cx_string *record =
                    _cpl_fitscard_get_card(cx_deque_get(self->_m_records, i));

            fprintf(os, "%s\n", cx_string_get(record));
        }

    }

    return;

}

#endif


// FITS data unit base class implementation

#include <fitsio.h>

#include <cxstring.h>


typedef enum _cpl_fits_du_type_ cpl_fits_dataunit_type;

enum _cpl_fits_du_type_
{
    CPL_FITS_DU_TYPE_IMAGE = IMAGE_HDU,
    CPL_FITS_DU_TYPE_ATBL  = ASCII_TBL,
    CPL_FITS_DU_TYPE_BTBL  = BINARY_TBL,
    CPL_FITS_DU_TYPE_EMPTY
};


typedef struct _cpl_fitsdataunit_ cpl_fitsdataunit;

struct _cpl_fitsdataunit_
{
    fitsfile   *_m_file;

    cxulong    _m_position;
    cxint      _m_type;
    cx_string *_m_name;
    cxulong    _m_version;
    cxulong    _m_level;

    cxint      _m_bitpix;
    cxlong     _m_pcount;
    cxlong     _m_gcount;

    cxulong    _m_unitsize;
    cxulong    _m_datasize;

    cxint      _m_naxes;
    LONGLONG  *_m_naxis;


    /*
     * Virtual function for writing data unit structure to the target file
     */

    cxint (*_m_write_layout)(cpl_fitsdataunit *self, fitsfile *file);
};



inline static cxint
_cpl_fits_read_axes(fitsfile *fptr, cxint naxis, LONGLONG *naxes,
                    cxint *status)
{

    if (*status == 0) {

        cxchar keyn[FLEN_KEYWORD] = {'N', 'A', 'X', 'I', 'S', '\0'};

        cxint i;


        for (i = 0; i < naxis; ++i) {
            snprintf(&keyn[5], FLEN_KEYWORD - 5, "%d", i + 1);
            fits_read_key(fptr, TLONGLONG, keyn, &naxes[i], 0, status);
        }

    }

    return *status;

}


inline static int
_cpl_fits_read_extinfo(fitsfile *fptr, cxint *hdutype,
                       cxint *bitpix, cxint *naxis, LONGLONG *naxes,
                       cxlong *pcount, cxlong *gcount, cxint *status)
{

    if (*status == 0) {

        cxint _status = 0;

        if (hdutype) {
            fits_get_hdu_type(fptr, hdutype, &_status);
        }

        fits_read_key(fptr, TINT,  "BITPIX", bitpix, 0, &_status);
        fits_read_key(fptr, TINT,  "NAXIS",  naxis,  0, &_status);

        if (naxes) {
            _cpl_fits_read_axes(fptr, *naxis, naxes, &_status);
        }

        if (_status) {
            *status = _status;
            return *status;
        }

        fits_read_key(fptr, TLONG, "PCOUNT", pcount, 0, &_status);
        fits_read_key(fptr, TLONG, "GCOUNT", gcount, 0, &_status);

        if (_status == KEY_NO_EXIST) {

            cxint hdunum;

            fits_get_hdu_num(fptr, &hdunum);

            /*
             * Reset the error status if this is a primary HDU, since
             * these keyword may not be present in a primary HDU.
             */

            if (hdunum == 1)
            {
                _status = 0;
                *pcount = 0;
                *gcount = 1;
            }

        }

        *status = _status;
    }

    return *status;

}


inline static int
_cpl_fits_read_extid(fitsfile *fptr, cxchar *extname, cxulong *extvers,
                     cxulong *extlevel, cxint *status)
{

    if (*status == 0) {

        cxint _status = 0;

        fits_read_key(fptr, TSTRING, "EXTNAME", extname, 0, &_status);

        if (_status == KEY_NO_EXIST) {
            _status = 0;
        }

        fits_read_key(fptr, TULONG, "EXTVER", extvers, 0, &_status);

        if (_status == KEY_NO_EXIST) {
            _status = 0;
        }

        fits_read_key(fptr, TULONG, "EXTLEVEL", extlevel, 0, &_status);

        if (_status == KEY_NO_EXIST) {
            _status = 0;
        }

        *status = _status;

    }

    return *status;

}


inline static cxint
_cpl_fitsdataunit_init_empty(cpl_fitsdataunit *self)
{

    self->_m_file     = NULL;
    self->_m_position = 0;
    self->_m_type     = ANY_HDU;
    self->_m_name     = NULL;
    self->_m_version  = 0;
    self->_m_level    = 0;
    self->_m_bitpix   = 0;
    self->_m_pcount   = 0;
    self->_m_gcount   = 0;
    self->_m_unitsize = 0;
    self->_m_datasize = 0;
    self->_m_naxes    = 0;
    self->_m_naxis    = NULL;

    self->_m_write_layout = NULL;

    return 0;

}


inline static cxint
_cpl_fitsdataunit_init(cpl_fitsdataunit *self, fitsfile *file,
                       cxulong position)
{

    _cpl_fitsdataunit_init_empty(self);

    self->_m_file     = file;
    self->_m_position = position + 1;
    self->_m_name     = cx_string_new();

    return 0;

}


inline static void
_cpl_fitsdataunit_clear(cpl_fitsdataunit *self)
{

    cx_string_delete(self->_m_name);
    cx_free(self->_m_naxis);

    self->_m_file     = NULL;
    self->_m_position = 0;
    self->_m_type     = ANY_HDU;
    self->_m_name     = NULL;
    self->_m_version  = 0;
    self->_m_level    = 0;
    self->_m_bitpix   = 0;
    self->_m_pcount   = 0;
    self->_m_gcount   = 0;
    self->_m_unitsize = 0;
    self->_m_datasize = 0;
    self->_m_naxes    = 0;
    self->_m_naxis    = NULL;

    self->_m_write_layout = NULL;

    return;

}


inline static cxint
_cpl_fitsdataunit_copy(cpl_fitsdataunit *self, const cpl_fitsdataunit *other)
{

    self->_m_file     = other->_m_file;
    self->_m_position = other->_m_position;
    self->_m_type     = other->_m_type;
    self->_m_name     = cx_string_copy(other->_m_name);
    self->_m_version  = other->_m_version;
    self->_m_level    = other->_m_level;
    self->_m_bitpix   = other->_m_bitpix;
    self->_m_pcount   = other->_m_pcount;
    self->_m_gcount   = other->_m_gcount;
    self->_m_unitsize = other->_m_unitsize;
    self->_m_datasize = other->_m_datasize;
    self->_m_naxes    = other->_m_naxes;

    if (other->_m_naxis) {

        register size_t sz = other->_m_naxes * sizeof *other->_m_naxis;

        self->_m_naxis = cx_malloc(sz);
        memcpy(self->_m_naxis, other->_m_naxis, sz);

    }
    else {
        self->_m_naxis = NULL;
    }

    self->_m_write_layout = other->_m_write_layout;

    return 0;

}


inline static cxint
_cpl_fitsdataunit_make_current(cpl_fitsdataunit *self)
{

    cxint status = 0;

    cxint hdutype = ANY_HDU;
    fits_movabs_hdu(self->_m_file, self->_m_position, &hdutype, &status);

    return status;

}


inline static cxint
_cpl_fitsdataunit_initialize(cpl_fitsdataunit *self, fitsfile *file,
                             cxulong position)
{
    cxint status  = 0;
    cxint idummy  = 0;

    LONGLONG start = 0;
    LONGLONG end   = 0;


    _cpl_fitsdataunit_init(self, file, position);


    status = _cpl_fitsdataunit_make_current(self);

    if (status) {
        _cpl_fitsdataunit_clear(self);
        return status;
    }

    _cpl_fits_read_extinfo(self->_m_file, &idummy, &self->_m_bitpix,
                           &self->_m_naxes, 0, &self->_m_pcount,
                           &self->_m_gcount, &status);

    fits_get_hduaddrll(self->_m_file, 0, &start, &end, &status);

    if (status) {
        _cpl_fitsdataunit_clear(self);
        return status;
    }

    self->_m_unitsize = end - start;


    if (self->_m_naxes > 0) {

        self->_m_naxis = cx_malloc(self->_m_naxes * sizeof *self->_m_naxis);

        _cpl_fits_read_axes(self->_m_file, self->_m_naxes, &self->_m_naxis[0],
                            &status);

    }

    if (status) {
        _cpl_fitsdataunit_clear(self);
        return status;
    }


    if (self->_m_position == 0) {

        cxint hdunum;
        self->_m_position = fits_get_hdu_num(self->_m_file, &hdunum);

    }

    if (self->_m_type == ANY_HDU) {
        fits_get_hdu_type(self->_m_file, &self->_m_type, &status);
    }

    if (status) {
        _cpl_fitsdataunit_clear(self);
        return status;
    }


    if (cx_string_size(self->_m_name)) {

        cxchar extname[FLEN_VALUE] = {'\0'};

        _cpl_fits_read_extid(self->_m_file, extname, &self->_m_version,
                             &self->_m_level, &status);
        cx_string_set(self->_m_name, extname);
    }

    if (status) {
        _cpl_fitsdataunit_clear(self);
        return status;
    }


    /*
     *  Calculate the size of the payload in bytes
     */

     if (self->_m_naxis != NULL) {

         cxint i;

         self->_m_datasize = self->_m_bitpix < 0 ?
                 -self->_m_bitpix / 8 : self->_m_bitpix / 8;

         for (i = 0; i < self->_m_naxes; ++i) {
             self->_m_datasize *= self->_m_naxis[i];
         }

     }

     return status;

}


inline static fitsfile *
_cpl_fitsdataunit_get_file(const cpl_fitsdataunit *self)
{
    cx_assert(self != NULL);
    return self->_m_file;
}


inline static const cx_string *
_cpl_fitsdataunit_get_name(const cpl_fitsdataunit *self)
{
    cx_assert(self != NULL);
    return self->_m_name;
}


inline static cxulong
_cpl_fitsdataunit_get_version(const cpl_fitsdataunit *self)
{
    cx_assert(self != NULL);
    return self->_m_version;
}


inline static cxulong
_cpl_fitsdataunit_get_level(const cpl_fitsdataunit *self)
{
    cx_assert(self != NULL);
    return self->_m_level;
}


inline static cxulong
_cpl_fitsdataunit_get_position(const cpl_fitsdataunit *self)
{
    cx_assert(self != NULL);
    return self->_m_position - 1;
}


inline static cpl_fits_dataunit_type
_cpl_fitsdataunit_get_type(const cpl_fitsdataunit *self)
{
    cx_assert(self != NULL);
    return self->_m_type;
}


inline static cxulong
_cpl_fitsdataunit_get_size(const cpl_fitsdataunit *self)
{
    cx_assert(self != NULL);
    return self->_m_unitsize;
}


inline static cxulong
_cpl_fitsdataunit_get_datasize(const cpl_fitsdataunit *self)
{
    cx_assert(self != NULL);
    return self->_m_datasize;
}


inline static cx_string *
_cpl_fitsdataunit_get_filename(const cpl_fitsdataunit *self)
{

    cxchar name[FLEN_FILENAME] = {'\0'};

    cxint status = 0;

    fits_file_name(self->_m_file, name, &status);

    if (status) {
        return NULL;
    }

    return cx_string_create(name);

}


inline static cxint
_cpl_fitsdataunit_write_data(cpl_fitsdataunit *self, fitsfile *file)
{

    cxint status = 0;

    status = _cpl_fitsdataunit_make_current(self);

    if (status) {
        return status;
    }

    fits_copy_data(self->_m_file, file, &status);

    return status;

}


inline static cxint
_cpl_fitsdataunit_write_layout(cpl_fitsdataunit *self, fitsfile *file)
{

    if (self->_m_write_layout == NULL) {
        return -1;
    }

    return self->_m_write_layout(self, file);

}



// FITS image data unit implementation

#include <fitsio.h>


typedef struct _cpl_fitsemptyunit_ cpl_fitsemptyunit;

struct _cpl_fitsemptyunit_
{
        cpl_fitsdataunit base;   /* Must be the first member */
};


inline static void
_cpl_fitsemptyunit_delete(cpl_fitsemptyunit *self)
{

    if (self) {
        _cpl_fitsdataunit_clear((cpl_fitsdataunit *)self);
    }

    cx_free(self);

    return;

}


static cxint
_cpl_fitsemptyunit_write_layout(cpl_fitsdataunit *self, fitsfile *file)
{

    cx_assert(self != NULL);
    cx_assert(file != NULL);


    cxint status = 0;

    fits_write_imghdrll(file, self->_m_bitpix, self->_m_naxes,
                        &self->_m_naxis[0], &status);

    return 0;

}


inline static cpl_fitsemptyunit *
_cpl_fitsemptyunit_new(void)
{

    cpl_fitsemptyunit *self = cx_malloc(sizeof *self);

    cpl_fitsdataunit *_self = (cpl_fitsdataunit *)self;


    _cpl_fitsdataunit_init_empty(_self);

    _self->_m_bitpix = 8;
    _self->_m_type   = CPL_FITS_DU_TYPE_EMPTY;
    _self->_m_write_layout = _cpl_fitsemptyunit_write_layout;

    return self;

}


inline static cpl_fitsemptyunit *
_cpl_fitsemptyunit_clone(const cpl_fitsemptyunit *other)
{


    cpl_fitsdataunit *_other = (cpl_fitsdataunit *)other;


    if (((_other->_m_file != NULL) && (_other->_m_position != 0)) ||
            (_other->_m_type != CPL_FITS_DU_TYPE_EMPTY)) {
        return NULL;
    }


    cpl_fitsemptyunit *self = cx_malloc(sizeof *self);

    cxint status = _cpl_fitsdataunit_copy((cpl_fitsdataunit *)self, _other);

    if (status) {
        _cpl_fitsemptyunit_delete(self);
        return NULL;
    }

    return self;

}



// FITS image data unit implementation

#include <fitsio.h>


typedef struct _cpl_fitsimageunit_ cpl_fitsimageunit;

struct _cpl_fitsimageunit_
{
        cpl_fitsdataunit base;   /* Must be the first member */
};


inline static void
_cpl_fitsimageunit_delete(cpl_fitsimageunit *self)
{

    if (self) {
        _cpl_fitsdataunit_clear((cpl_fitsdataunit *)self);
    }

    cx_free(self);

    return;

}


static cxint
_cpl_fitsimageunit_write_layout(cpl_fitsdataunit *self, fitsfile *file)
{

    cx_assert(self != NULL);
    cx_assert(file != NULL);


    cxint status = 0;

    fits_write_imghdrll(file, self->_m_bitpix, self->_m_naxes,
                        &self->_m_naxis[0], &status);

    return 0;

}


inline static cpl_fitsimageunit *
_cpl_fitsimageunit_new(fitsfile *file, cxulong position)
{

    cpl_fitsimageunit *self = cx_malloc(sizeof *self);

    cpl_fitsdataunit *_self = (cpl_fitsdataunit *)self;


    cxint status = _cpl_fitsdataunit_initialize(_self, file, position);

    if (status) {
        cx_free(self);
        return NULL;
    }

    if (_self->_m_type != CPL_FITS_DU_TYPE_IMAGE) {
        status = NOT_IMAGE;
    }

    if (_self->_m_pcount != 0) {
        status = BAD_PCOUNT;
    }

    if (_self->_m_gcount != 1) {
        status = BAD_GCOUNT;
    }

    if (status) {
        _cpl_fitsimageunit_delete(self);
        return NULL;
    }

    _self->_m_write_layout = _cpl_fitsimageunit_write_layout;

    return self;

}


inline static cpl_fitsimageunit *
_cpl_fitsimageunit_clone(const cpl_fitsimageunit *other)
{


    cpl_fitsdataunit *_other = (cpl_fitsdataunit *)other;


    if ((_other->_m_file == NULL) || (_other->_m_position == 0) ||
            (_other->_m_type != CPL_FITS_DU_TYPE_IMAGE)) {
        return NULL;
    }


    cpl_fitsimageunit *self = cx_malloc(sizeof *self);

    cxint status = _cpl_fitsdataunit_copy((cpl_fitsdataunit *)self, _other);

    if (status) {
        _cpl_fitsimageunit_delete(self);
        return NULL;
    }

    return self;

}



// FITS binary table data unit implementation

#include <fitsio.h>

#include <cxstrutils.h>


typedef struct _cpl_fitsbtableunit_ cpl_fitsbtableunit;

struct _cpl_fitsbtableunit_
{
        cpl_fitsdataunit base;   /* Must be the first member */

        cxint    _m_columns;
        LONGLONG _m_rows;

        cxchar **_m_ttype;
        cxchar **_m_tform;
        cxchar **_m_tunit;
        cxchar **_m_tdisp;
        cxchar **_m_tdim;
        cxchar **_m_tscale;
        cxchar **_m_tzero;
        cxchar **_m_tnull;

        cxchar *_m_tfields;
        cxchar *_m_theap;
};


inline static void
_cpl_fitsbtableunit_delete(cpl_fitsbtableunit *self)
{

    if (self) {

        cxint i;

        cx_free(self->_m_theap);
        cx_free(self->_m_tfields);

        for (i = 0; i < self->_m_columns; ++i) {

            cx_free(self->_m_tnull[i]);
            cx_free(self->_m_tzero[i]);
            cx_free(self->_m_tscale[i]);
            cx_free(self->_m_tdim[i]);
            cx_free(self->_m_tdisp[i]);
            cx_free(self->_m_tunit[i]);
            cx_free(self->_m_tform[i]);
            cx_free(self->_m_ttype[i]);

        }

        cx_free(self->_m_tnull);
        cx_free(self->_m_tzero);
        cx_free(self->_m_tscale);
        cx_free(self->_m_tdim);
        cx_free(self->_m_tdisp);
        cx_free(self->_m_tunit);
        cx_free(self->_m_tform);
        cx_free(self->_m_ttype);

        _cpl_fitsdataunit_clear((cpl_fitsdataunit *)self);

    }

    cx_free(self);

    return;

}


static cxint
_cpl_fitsbtableunit_write_layout(cpl_fitsdataunit *self, fitsfile *file)
{

    cxint status = 0;
    cxint hdunum = 0;

    cpl_fitsbtableunit *_self = (cpl_fitsbtableunit *)self;


    fits_get_hdu_num(file, &hdunum);

    if (hdunum == 1) {
        return NOT_BTABLE;
    }

    long axis[] = {self->_m_naxis[0], self->_m_naxis[1]};

    fits_write_exthdr(file, "BINTABLE", self->_m_bitpix, self->_m_naxes, axis,
                      self->_m_pcount, self->_m_gcount, &status);
    fits_write_record(file, _self->_m_tfields, &status);


    // FIXME: Instead of fixing the comments written by the function writing
    //        a generic extension header, one could use the function writing
    //        a complete binary table header. However this requires getting
    //        the fully parsed value of the TTYPE, TFORM and TUNIT keywords!

    /*
     * Fix keyword comments. Drop the generic extension keyword comments
     * in favor of binary table specific ones.
     */

    fits_modify_comment(file, "XTENSION", "binary table extension",
                        &status);
    fits_modify_comment(file, "BITPIX", "8-bit bytes",
                        &status);
    fits_modify_comment(file, "NAXIS", "2-dimensional binary table",
                        &status);
    fits_modify_comment(file, "NAXIS1", "width of table in bytes",
                        &status);
    fits_modify_comment(file, "NAXIS2", "number of rows in table",
                        &status);

    if (status) {
        return status;
    }


    /*
     *  Write table meta data
     */

    typedef cxchar ** _cpl_stringarray;

    _cpl_stringarray *tdata[] = {&_self->_m_ttype, &_self->_m_tform,
                                 &_self->_m_tunit, &_self->_m_tdisp,
                                 &_self->_m_tdim, &_self->_m_tscale,
                                 &_self->_m_tzero, &_self->_m_tnull, 0};

    cxint i;

    for (i = 0; i < _self->_m_columns; ++i) {

        cxsize k = 0;

        status = 0;

        while (tdata[k]) {

            cxchar *const *td = *tdata[k];

            if ((td[i] != NULL) && (strlen(td[i]) > 0)) {
                fits_write_record(file, td[i], &status);
            }

            ++k;
        }

        if (status) {
            return status;
        }

    }

    if ((_self->_m_theap != NULL) && (strlen(_self->_m_theap) > 0)) {
        fits_write_record(file, _self->_m_theap, &status);
    }

    return status;

}


inline static cpl_fitsbtableunit *
_cpl_fitsbtableunit_new(fitsfile *file, cxulong position)
{

    cpl_fitsbtableunit *self = cx_malloc(sizeof *self);

    cpl_fitsdataunit *_self = (cpl_fitsdataunit *)self;


    cxint status = _cpl_fitsdataunit_initialize(_self, file, position);

    if (status) {
        cx_free(self);
        return NULL;
    }


    if (_self->_m_type != CPL_FITS_DU_TYPE_BTBL) {
        status = NOT_BTABLE;
    }

    if (_self->_m_bitpix != 8) {
        status = BAD_BITPIX;
    }

    if (_self->_m_naxes != 2) {
        status = BAD_NAXIS;
    }

    if (_self->_m_gcount != 1) {
        status = BAD_GCOUNT;
    }

    if (status) {
        _cpl_fitsbtableunit_delete(self);
        return NULL;
    }

    _self->_m_write_layout = _cpl_fitsbtableunit_write_layout;

    self->_m_ttype  = NULL;
    self->_m_tform  = NULL;
    self->_m_tunit  = NULL;
    self->_m_tdisp  = NULL;
    self->_m_tdim   = NULL;
    self->_m_tscale = NULL;
    self->_m_tzero  = NULL;
    self->_m_tnull  = NULL;

    self->_m_tfields = NULL;
    self->_m_theap   = NULL;


    /*
     * Get the number of columns and rows in the table.
     * Note that the number of columns is given by the
     * TFIELDS keyword.
     */

    fits_get_num_cols(_self->_m_file, &self->_m_columns, &status);
    fits_get_num_rowsll(_self->_m_file, &self->_m_rows, &status);

    if (status) {
        _cpl_fitsbtableunit_delete(self);
        return NULL;
    }

    if (self->_m_columns) {

        self->_m_ttype  = cx_malloc(self->_m_columns * sizeof(cxchar *));
        self->_m_tform  = cx_malloc(self->_m_columns * sizeof(cxchar *));
        self->_m_tunit  = cx_malloc(self->_m_columns * sizeof(cxchar *));
        self->_m_tdisp  = cx_malloc(self->_m_columns * sizeof(cxchar *));
        self->_m_tdim   = cx_malloc(self->_m_columns * sizeof(cxchar *));
        self->_m_tscale = cx_malloc(self->_m_columns * sizeof(cxchar *));
        self->_m_tzero  = cx_malloc(self->_m_columns * sizeof(cxchar *));
        self->_m_tnull  = cx_malloc(self->_m_columns * sizeof(cxchar *));


        cxchar card[FLEN_CARD];

        /*
         * Read the TFIELDS record from the file. TFIELDS is actually
         * the number of columns in the table, and is already available
         * as a number, but to avoid any changes in the textual
         * representation we keep a copy of the card too.
         */

        card[0] = '\0';
        fits_read_card(_self->_m_file, "TFIELDS", card, &status);

        if (status) {
            _cpl_fitsbtableunit_delete(self);
            return NULL;
        }

        self->_m_tfields = (cxchar *)cx_strdup(card);


        /*
         * Read the table meta data
         */

        typedef cxchar ** _cpl_stringarray;

        struct _cpl_btable_metadata
        {
            const cxchar *name;
            cxbool        required;

            _cpl_stringarray *data;

            cxuint error;
        };

        struct _cpl_btable_metadata tdata[] = {
            {"TTYPE",  FALSE, &self->_m_ttype,  KEY_NO_EXIST},
            {"TFORM",  TRUE,  &self->_m_tform,  NO_TFORM},
            {"TUNIT",  FALSE, &self->_m_tunit,  KEY_NO_EXIST},
            {"TDISP",  FALSE, &self->_m_tdisp,  KEY_NO_EXIST},
            {"TDIM",   FALSE, &self->_m_tdim,   KEY_NO_EXIST},
            {"TSCALE", FALSE, &self->_m_tscale, KEY_NO_EXIST},
            {"TZERO",  FALSE, &self->_m_tzero,  KEY_NO_EXIST},
            {"TNULL",  FALSE, &self->_m_tnull,  KEY_NO_EXIST},
            {0, FALSE, 0, 0}
        };

        cxint i;

        for (i = 0; i < self->_m_columns; ++i) {

            cxsize k = 0;

            while (tdata[k].name) {

                cxchar keyname[FLEN_KEYWORD] = {'\0'};

                snprintf(keyname, FLEN_KEYWORD - 1 , "%s%d",
                         tdata[k].name, i + 1);

                card[0] = '\0';
                fits_read_card(_self->_m_file, keyname, card, &status);

                if (status) {

                    if (status == KEY_NO_EXIST) {

                        /*
                         * Ignore keyword not found errors unless the keyword
                         * is a required keyword.
                         */

                        if (tdata[k].required) {
                            _cpl_fitsbtableunit_delete(self);
                            return NULL;
                        }

                    }
                    else {
                        _cpl_fitsbtableunit_delete(self);
                        return NULL;
                    }

                }


                /*
                 * If the status flag is not set, the keyword was found and
                 * the FITS card is stored. A non-zero status at this point
                 * indicates that an optional keyword was not found. In this
                 * latter case the status flag is reset and the data pointer
                 * is set to NULL.
                 */

                if (status == 0) {
                    (*tdata[k].data)[i] = (cxchar *)cx_strdup(card);
                }
                else {
                    status = 0;
                    (*tdata[k].data)[i] = NULL;
                }

                ++k;

            }

        }

        card[0] = '\0';
        fits_read_card(_self->_m_file, "THEAP", card, &status);

        if (status && (status != KEY_NO_EXIST)) {
            _cpl_fitsbtableunit_delete(self);
            return NULL;
        }

        if (status == 0) {
            self->_m_theap = (cxchar *)cx_strdup(card);
        }

    }

    return self;

}


inline static cpl_fitsbtableunit *
_cpl_fitsbtableunit_clone(const cpl_fitsbtableunit *other)
{


    cpl_fitsdataunit *_other = (cpl_fitsdataunit *)other;


    if ((_other->_m_file == NULL) || (_other->_m_position == 0) ||
            (_other->_m_type != CPL_FITS_DU_TYPE_BTBL)) {
        return NULL;
    }


    cpl_fitsbtableunit *self = cx_malloc(sizeof *self);

    cxint status = _cpl_fitsdataunit_copy((cpl_fitsdataunit *)self, _other);

    if (status) {
        _cpl_fitsbtableunit_delete(self);
        return NULL;
    }


    /*
     * Copy data binary table unit data members
     */

    self->_m_columns = other->_m_columns;
    self->_m_rows    = other->_m_rows;

    if (self->_m_columns) {

        self->_m_ttype  = cx_malloc(other->_m_columns * sizeof(cxchar *));
        self->_m_tform  = cx_malloc(other->_m_columns * sizeof(cxchar *));
        self->_m_tunit  = cx_malloc(other->_m_columns * sizeof(cxchar *));
        self->_m_tdisp  = cx_malloc(other->_m_columns * sizeof(cxchar *));
        self->_m_tdim   = cx_malloc(other->_m_columns * sizeof(cxchar *));
        self->_m_tscale = cx_malloc(other->_m_columns * sizeof(cxchar *));
        self->_m_tzero  = cx_malloc(other->_m_columns * sizeof(cxchar *));
        self->_m_tnull  = cx_malloc(other->_m_columns * sizeof(cxchar *));

        cxint i;

        for (i = 0; i < self->_m_columns; ++i) {

            self->_m_tnull[i] = cx_strdup(other->_m_tnull[i]);
            self->_m_tzero[i] = cx_strdup(other->_m_tzero[i]);
            self->_m_tscale[i] = cx_strdup(other->_m_tscale[i]);
            self->_m_tdim[i] = cx_strdup(other->_m_tdim[i]);
            self->_m_tdisp[i] = cx_strdup(other->_m_tdisp[i]);
            self->_m_tunit[i] = cx_strdup(other->_m_tunit[i]);
            self->_m_tform[i] = cx_strdup(other->_m_tform[i]);
            self->_m_ttype[i] = cx_strdup(other->_m_ttype[i]);

        }

    }
    else {

        self->_m_ttype  = NULL;
        self->_m_tform  = NULL;
        self->_m_tunit  = NULL;
        self->_m_tdisp  = NULL;
        self->_m_tdim   = NULL;
        self->_m_tscale = NULL;
        self->_m_tzero  = NULL;
        self->_m_tnull  = NULL;

    }

    self->_m_tfields = cx_strdup(other->_m_tfields);
    self->_m_theap = cx_strdup(other->_m_theap);

    return self;

}



// FITS dataset implementation

#include <fitsio.h>


static const cxchar *_CPL_FITSDATASET_IGNORE_KEYPATTERN =
        "(^SIMPLE  =|^XTENSION=|^BITPIX  =|^NAXIS   =|^NAXIS[1-9][0-9]*"
        "|^PCOUNT  =|^GCOUNT  ="
        "|^EXTEND  =|^EXTNAME =|^EXTVERS =|^EXTLEVEL="
        "|^TFIELDS ="
        "|^TTYPE[1-9][0-9]* *=|^TFORM[1-9][0-9]* *=|^TUNIT[1-9][0-9]* *="
        "|^TDISP[1-9][0-9]* *=|^TDIM[1-9][0-9]* *="
        "|^TSCALE[1-9][0-9]* *=|^TZERO[1-9][0-9]* *=|^TNULL[1-9][0-9]* *="
        "|^THEAP   ="
        "|^CHECKSUM=|^DATASUM =|^DATAMD5 =|^DATE    =|^END *$"
        "|^ZHECKSUM=|^ZDATASUM="
        "|^ORIGFILE=|^ARCHFILE="
        "|^COMMENT *FITS \\(Flexible Image Transport System\\) format"
        "|^COMMENT *and Astrophysics', volume 376, page 359;)";


typedef struct _cpl_fitsdataset_ cpl_fitsdataset;

struct _cpl_fitsdataset_
{
        const cxchar *_m_name;

        cxulong _m_version;
        cxulong _m_level;

        cpl_fitsheader   *_m_header;
        cpl_fitsdataunit *_m_data;
};


inline static cpl_fitsdataunit *
_cpl_fitsdataset_make_dataunit(fitsfile *file, cxulong position)
{

    cxint status  = 0;
    cxint hdutype = ANY_HDU;

    fits_movabs_hdu(file, position + 1, &hdutype, &status);

    if (status) {
        return NULL;
    }


    cpl_fitsdataunit *dataunit = NULL;

    switch (hdutype) {
        case IMAGE_HDU:
            dataunit = (cpl_fitsdataunit *)_cpl_fitsimageunit_new(file,
                                                                  position);
            break;

        case BINARY_TBL:
            dataunit = (cpl_fitsdataunit *)_cpl_fitsbtableunit_new(file,
                                                                   position);
            break;

        default:
            break;

    }

    return dataunit;

}


inline static cpl_fitsdataunit *
_cpl_fitsdataset_clone_dataunit(const cpl_fitsdataunit *other)
{

    cpl_fitsdataunit *dataunit = NULL;

    switch (_cpl_fitsdataunit_get_type(other)) {

        case CPL_FITS_DU_TYPE_IMAGE:
        {
            const cpl_fitsimageunit *_other = (const cpl_fitsimageunit *)other;

            dataunit = (cpl_fitsdataunit *)_cpl_fitsimageunit_clone(_other);
            break;
        }

        case CPL_FITS_DU_TYPE_BTBL:
        {
            const cpl_fitsbtableunit *_other = (const cpl_fitsbtableunit *)other;

            dataunit = (cpl_fitsdataunit *)_cpl_fitsbtableunit_clone(_other);
            break;
        }

        case CPL_FITS_DU_TYPE_EMPTY:
        {
            const cpl_fitsemptyunit *_other = (const cpl_fitsemptyunit *)other;

            dataunit = (cpl_fitsdataunit *)_cpl_fitsemptyunit_clone(_other);
            break;
        }

        default:
            break;

    }

    return dataunit;

}


inline static void
_cpl_fitsdataset_delete(cpl_fitsdataset *self)
{

    if (self->_m_name) {
        cx_free((cxchar *)self->_m_name);
    }

    if (self->_m_header) {
        _cpl_fitsheader_delete(self->_m_header);
    }

    if (self->_m_data) {

        switch (_cpl_fitsdataunit_get_type(self->_m_data)) {

            case CPL_FITS_DU_TYPE_IMAGE:
                _cpl_fitsimageunit_delete((cpl_fitsimageunit *)self->_m_data);
                break;

            case CPL_FITS_DU_TYPE_BTBL:
                _cpl_fitsbtableunit_delete((cpl_fitsbtableunit *)self->_m_data);
                break;

            case CPL_FITS_DU_TYPE_EMPTY:
                _cpl_fitsemptyunit_delete((cpl_fitsemptyunit *)self->_m_data);
                break;

            default:
                break;

        }
    }

    cx_free(self);

    return;

}


inline static cpl_fitsdataset *
_cpl_fitsdataset_new(fitsfile *file, cxulong position)
{

    cpl_fitsdataset *self = cx_malloc(sizeof *self);

    self->_m_name    = NULL;
    self->_m_version = 0;
    self->_m_level   = 0;
    self->_m_data    = NULL;

    cpl_regex *ignore_keys =
            cpl_regex_new(_CPL_FITSDATASET_IGNORE_KEYPATTERN, TRUE,
                          CPL_REGEX_EXTENDED | CPL_REGEX_NOSUBS);

    cpl_fitsheader *hdr = _cpl_fitsheader_create_from_file(file, position);


    self->_m_header = _cpl_fitsheader_create_from_filter(hdr, ignore_keys);

    cpl_regex_delete(ignore_keys);
    ignore_keys = NULL;

    cpl_fitscard *name    = _cpl_fitsheader_find(hdr, "EXTNAME");
    cpl_fitscard *version = _cpl_fitsheader_find(hdr, "EXTVER");
    cpl_fitscard *level   = _cpl_fitsheader_find(hdr, "EXTLEVEL");

    if (name) {

        cx_string *_name = _cpl_fitscard_get_value(name);

        cxsize start = cx_string_find_first_not_of(_name, " '");
        cxsize end   = cx_string_find_last_not_of(_name, " '");

        cx_string *s = cx_string_substr(_name, start, end);
        self->_m_name = cx_strdup(cx_string_get(s));

        cx_string_delete(s);
        cx_string_delete(_name);

    }

    if (version) {

        cx_string *value = _cpl_fitscard_get_value(version);

        cxchar *end;
        cxulong _version = strtoul(cx_string_get(value), &end, 10);


        cx_string_delete(value);

        if (*end != '\0') {

            _cpl_fitsheader_delete(hdr);
            _cpl_fitsdataset_delete(self);

            return NULL;

        }

        self->_m_version = _version;

    }

    if (level) {

        cx_string *value = _cpl_fitscard_get_value(level);

        cxchar *end;
        cxulong _level = strtoul(cx_string_get(value), &end, 10);


        cx_string_delete(value);

        if (*end != '\0') {

            _cpl_fitsheader_delete(hdr);
            _cpl_fitsdataset_delete(self);

            return NULL;

        }

        self->_m_level = _level;
    }

    _cpl_fitsheader_delete(hdr);

    self->_m_data = _cpl_fitsdataset_make_dataunit(file, position);

    return self;

}


inline static cpl_fitsdataset *
_cpl_fitsdataset_create(const cpl_fitsheader *hdr, const cpl_fitsdataunit *data)
{

    cpl_fitsdataset *self = cx_malloc(sizeof *self);

    self->_m_name    = NULL;
    self->_m_version = 0;
    self->_m_level   = 0;
    self->_m_data    = NULL;

    cpl_regex *ignore_keys =
            cpl_regex_new(_CPL_FITSDATASET_IGNORE_KEYPATTERN, TRUE,
                          CPL_REGEX_EXTENDED | CPL_REGEX_NOSUBS);

    self->_m_header = _cpl_fitsheader_create_from_filter(hdr, ignore_keys);

    cpl_regex_delete(ignore_keys);
    ignore_keys = NULL;

    self->_m_data = _cpl_fitsdataset_clone_dataunit(data);

    return self;

}


inline static cpl_fitsheader *
_cpl_fitsdataset_get_header(cpl_fitsdataset *self)
{
    return self->_m_header;
}


inline static const cpl_fitsheader *
_cpl_fitsdataset_get_header_const(const cpl_fitsdataset *self)
{
    return self->_m_header;
}


inline static cpl_fitsdataunit *
_cpl_fitsdataset_get_data(cpl_fitsdataset *self)
{
    return self->_m_data;
}


inline static const cpl_fitsdataunit *
_cpl_fitsdataset_get_data_const(const cpl_fitsdataset *self)
{
    return self->_m_data;
}


inline static const cxchar *
_cpl_fitsdataset_get_name(const cpl_fitsdataset *self)
{
    return self->_m_name;
}


inline static cxsize
_cpl_fitsdataset_get_version(const cpl_fitsdataset *self)
{
    return self->_m_version;
}


inline static cxsize
_cpl_fitsdataset_get_level(const cpl_fitsdataset *self)
{
    return self->_m_level;
}



inline static cxint
_cpl_fitsdataset_set_header(cpl_fitsdataset *self, const cpl_fitsheader *hdr)
{

    cpl_regex *ignore_keys =
            cpl_regex_new(_CPL_FITSDATASET_IGNORE_KEYPATTERN, TRUE,
                          CPL_REGEX_EXTENDED | CPL_REGEX_NOSUBS);

    if (ignore_keys == NULL) {
        return 1;
    }


    cpl_fitsheader *_hdr = _cpl_fitsheader_create_from_filter(hdr,
                                                              ignore_keys);

    cpl_regex_delete(ignore_keys);
    ignore_keys = NULL;

    if (_hdr == NULL) {
        return 1;
    }

    if (self->_m_header) {
        _cpl_fitsheader_delete(self->_m_header);
    }

    self->_m_header = _hdr;

    return 0;

}


inline static cxint
_cpl_fitsdataset_set_id(cpl_fitsdataset *self, const cxchar *name,
                        cxulong version, cxulong level)
{

    cx_assert(name != NULL);


    if (self->_m_name) {
        cx_free((cxchar *)self->_m_name);
    }

    self->_m_name = cx_strdup(name);


    if (version > 0) {
        self->_m_version = version;
    }

    if (level > 0) {
        self->_m_level = level;
    }

    return 0;

}


inline static cxint
_cpl_fitsdataset_write(const cpl_fitsdataset *self, fitsfile *file,
                       cxbool is_primary, cxbool checksums)
{

    cxint status = 0;

    /*
     * Append new, empty FITS HDU
     */

    fits_create_hdu(file, &status);

    if (status) {
        return status;
    }


    /*
     * Write FITS data unit structure
     */

    self->_m_data->_m_write_layout(self->_m_data, file);


    /*
     * If defined, write FITS extension identifier. These keywords were
     * filtered out of the header when reading it from a file, so that
     * appending the keywords rather than updating it is ok.
     *
     * Note that the data types used in the fits_write_key() calls of the
     * version and the level must match the actual type definition of the
     * data members, since it is passed by pointer. To be independent of
     * the actual member type an assignment to a local variable could
     * be used.
     */

    if ((self->_m_name != NULL) && (strlen(self->_m_name) > 0)) {
        fits_write_key(file, TSTRING, "EXTNAME", (cxchar *)self->_m_name,
                       "FITS Extension name", &status);
    }

    if (self->_m_version > 0) {

        cxulong version = self->_m_version;

        fits_write_key(file, TULONG, "EXTVER", &version,
                       "FITS Extension identification", &status);

    }

    if (self->_m_level > 0) {

        cxulong level = self->_m_level;

        fits_write_key(file, TULONG, "EXTLEVEL", &level,
                       "FITS Extension level", &status);

    }

    if (status) {
        return status;
    }


    /*
     *  Write templates for file creation time stamp and the checksums.
     */

    if (is_primary) {
        fits_write_key(file, TSTRING, (cxchar *)"DATE",
                       (cxchar *)"YYYY-MM-DDThh:mm:ss",
                       NULL, &status);
    }


    if (checksums) {
        fits_write_key(file, TSTRING, (cxchar *)"CHECKSUM",
                       (cxchar *)"0000000000000000",
                       (cxchar *)"ASCII 1's complement checksum", &status);
        fits_write_key(file, TSTRING, (cxchar *)"DATASUM",
                       (cxchar *)"       0",
                       (cxchar *)"", &status);
    }

    if (status)
    {
        return status;
    }


    /*
     * Reserve space for the header keyword records and write them to the
     * output stream.
     */

    fits_set_hdrsize(file, _cpl_fitsheader_get_size(self->_m_header), &status);

    cxsize i;

    for (i = 0; i < _cpl_fitsheader_get_size(self->_m_header); ++i) {

        const cpl_fitscard *card = _cpl_fitsheader_get_const(self->_m_header,
                                                             i);
        const cx_string *record = _cpl_fitscard_get_card(card);

        fits_write_record(file, cx_string_get(record), &status);

    }

    if (status)  {
        return status;
    }


    /*
     * Dump the data to the output stream
     */

    status = _cpl_fitsdataunit_write_data(self->_m_data, file);

    if (status) {
        return -1;
    }


    /*
     * Write checksums and time stamp
     */

    if (is_primary) {

        fits_write_date(file, &status);
        fits_modify_comment(file, "DATE", "Date this file was written",
                            &status);

    }

    if (checksums) {
        fits_write_chksum(file, &status);
    }

    return status;

}


// CPL multi-frame implementation

/**
 * @defgroup cpl_multiframe Multi Frames
 *
 * This module implements the @c cpl_multiframe container type. A multi frame
 * contains references to datasets (FITS extensions) which may be distributed
 * across several physical files. These references can then be merged into
 * a new product file.
 *
 * @par Synopsis:
 * @code
 *   #include <cpl_multiframe.h>
 * @endcode
 *
 * @note This feature should be considered as experimental!
 */

/**@{*/

#include <cxmap.h>
#include <cxdeque.h>

#include <cpl_error_impl.h>
#include <cpl_fits.h>



struct _cpl_multiframe_
{
        cx_map   *_m_files;
        cx_deque *_m_datasets;
};


static cxbool
_cpl_multiframe_key_compare(cxcptr a, cxcptr b)
{

    const cxchar *_a = a;
    const cxchar *_b = b;

    return (strcmp(_a, _b) < 0) ? TRUE : FALSE;

}


inline static cpl_error_code
_cpl_multiframe_append_dataset(cpl_multiframe *self, const cxchar *id,
                               const cxchar *filename, cxsize position,
                               const cpl_regex *filter1,
                               const cpl_regex *filter2,
                               cxuint flags)
{

    cx_assert(self != NULL);
    cx_assert(id != NULL);
    cx_assert(filename != NULL);

    // FIXME: Should it be allowed to append a primary dataset?

    //cx_assert(position > 0);

    cx_assert((flags == CPL_MULTIFRAME_ID_SET) ||
              (flags == CPL_MULTIFRAME_ID_PREFIX) ||
              (flags == CPL_MULTIFRAME_ID_JOIN));
    cx_assert((flags != CPL_MULTIFRAME_ID_PREFIX) || (*id != '\0'));


    cxint status = 0;

    cx_map_iterator it = cx_map_find(self->_m_files, filename);

    fitsfile *fp = NULL;


    if (it != cx_map_end(self->_m_files)) {
        fp = cx_map_get_value(self->_m_files, it);
    }
    else {

        fits_open_diskfile(&fp, filename, READONLY, &status);

        if (status) {

            return cpl_error_set_(CPL_ERROR_ASSIGNING_STREAM);

        }

    }


    cpl_fitsheader *hdr0 = NULL;

    cpl_fitsdataset *_dataset = _cpl_fitsdataset_new(fp, 0);


    if (_dataset == NULL) {

        fits_close_file(fp, &status);
        return cpl_error_set_(CPL_ERROR_DATA_NOT_FOUND);

    }

    if (filter1) {
        hdr0 = _cpl_fitsheader_create_from_filter(_cpl_fitsdataset_get_header_const(_dataset),
                                                  filter1);
    }
    else {
        hdr0 = _cpl_fitsheader_duplicate(_cpl_fitsdataset_get_header_const(_dataset));
    }

    if (hdr0 == NULL) {

        _cpl_fitsdataset_delete(_dataset);
        fits_close_file(fp,  &status);

        return cpl_error_set_(CPL_ERROR_ILLEGAL_OUTPUT);

    }


    cpl_fitsdataset *dataset = NULL;

    if (position == 0) {

        cpl_fitsheader *_hdr0 = _cpl_fitsheader_new();

        dataset = _cpl_fitsdataset_create(_hdr0,
                                          _cpl_fitsdataset_get_data_const(_dataset));

        _cpl_fitsheader_delete(_hdr0);

    }
    else {

        dataset = _cpl_fitsdataset_new(fp, position);

    }

    if (dataset == NULL) {

        _cpl_fitsheader_delete(hdr0);
        _cpl_fitsdataset_delete(_dataset);

        fits_close_file(fp, &status);

        return cpl_error_set_(CPL_ERROR_DATA_NOT_FOUND);

    }


    /*
     * If a filter for the secondary header is given apply it and create a
     * new, filtered header.
     */

    if (filter2) {

        const cpl_fitsheader *hdr = _cpl_fitsdataset_get_header_const(dataset);

        cpl_fitsheader *_hdr = _cpl_fitsheader_create_from_filter(hdr, filter2);


        if (_hdr == NULL) {

            _cpl_fitsdataset_delete(dataset);
            _cpl_fitsheader_delete(hdr0);
            _cpl_fitsdataset_delete(_dataset);

            fits_close_file(fp, &status);

            return cpl_error_set_(CPL_ERROR_ILLEGAL_OUTPUT);

        }

        status = _cpl_fitsdataset_set_header(dataset, _hdr);

        _cpl_fitsheader_delete(_hdr);
        _hdr = NULL;

        if (status) {

            _cpl_fitsdataset_delete(dataset);
            _cpl_fitsheader_delete(hdr0);
            _cpl_fitsdataset_delete(_dataset);

            fits_close_file(fp, &status);

            return cpl_error_set_(CPL_ERROR_ILLEGAL_OUTPUT);

        }

    }


    /*
     * If the primary header is not empty, create the new dataset header by
     * joining the primary and the auxiliary headers.
     */

    if ((hdr0 != NULL) && (_cpl_fitsheader_get_size(hdr0) > 0)) {

        cpl_fitsheader *_hdr = _cpl_fitsdataset_get_header(dataset);

        _cpl_fitsheader_join(_hdr, hdr0);
        _cpl_fitsheader_sort(_hdr, _cpl_fitscard_compare_dicb);

    }

    _cpl_fitsheader_delete(hdr0);
    hdr0 = NULL;


    /*
     * Construct and assign the new dataset id
     */

    const cxchar *_name = _cpl_fitsdataset_get_name(dataset);

    cx_string *_id = 0;


    switch (flags) {

        case CPL_MULTIFRAME_ID_SET:
        {
            _id = cx_string_create(id);
            break;
        }

        case CPL_MULTIFRAME_ID_PREFIX:
        {
            _id = cx_string_create(id);
            cx_string_append(_id, _name);

            break;
        }

        case CPL_MULTIFRAME_ID_JOIN:
        {

            const cxchar *_prefix = _cpl_fitsdataset_get_name(_dataset);

            if ((_prefix == NULL) || (strlen(_prefix) == 0)) {

                _cpl_fitsdataset_delete(dataset);
                _cpl_fitsdataset_delete(_dataset);

                fits_close_file(fp, &status);

                return cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);

            }

            _id = cx_string_create(_prefix);

            if (_name && (*_name != '\0')) {
                cx_string_append(_id, id);
                cx_string_append(_id, _name);
            }

            break;

        }

        default:
        {

            /* This point should never be reached */

            _cpl_fitsdataset_delete(dataset);
            _cpl_fitsdataset_delete(_dataset);

            fits_close_file(fp, &status);

            return cpl_error_set_(CPL_ERROR_UNSUPPORTED_MODE);
            break;

        }

    }

    _cpl_fitsdataset_delete(_dataset);
    _dataset = NULL;


    // FIXME: The following call does not handle the dataset version and level.
    //        Instead the dataset should be searched for the id, and if
    //        it is already present, the dataset version should be set
    //        to one more than the maximum version which is already present.

    status = _cpl_fitsdataset_set_id(dataset, cx_string_get(_id), 0, 0);

    cx_string_delete(_id);
    _id = NULL;

    if (status) {

        _cpl_fitsdataset_delete(dataset);
        fits_close_file(fp, &status);

        return cpl_error_set_(CPL_ERROR_ILLEGAL_OUTPUT);

    }


    /*
     * Register the dataset, and the associated fitsfile handle if it has not
     * been registered before.
     */

    cx_deque_push_back(self->_m_datasets, dataset);

    if (it == cx_map_end(self->_m_files)) {

        cx_map_insert(self->_m_files, cx_strdup(filename), fp);

    }

    return CPL_ERROR_NONE;

}


inline static cpl_fitsdataset *
_cpl_multiframe_merge(fitsfile *fp, cxsize position,
                      const cpl_fitsheader *hdr,
                      const cpl_regex *filter1,
                      const cpl_regex *filter2)
{

    cx_assert(fp != NULL);
    cx_assert(hdr != NULL);

    // FIXME: Should it be allowed to append a primary dataset?

    cx_assert(position > 0);


    cxint status = 0;

    cpl_fitsheader *hdr0 = NULL;


    if (filter1) {
        hdr0 = _cpl_fitsheader_create_from_filter(hdr, filter1);
    }
    else {
        hdr0 = _cpl_fitsheader_duplicate(hdr);
    }

    if (hdr0 == NULL) {

        cpl_error_set_(CPL_ERROR_ILLEGAL_OUTPUT);
        return NULL;

    }


    cpl_fitsdataset *dataset = _cpl_fitsdataset_new(fp, position);

    if (dataset == NULL) {

        _cpl_fitsheader_delete(hdr0);
        cpl_error_set_(CPL_ERROR_DATA_NOT_FOUND);

        return NULL;

    }


    /*
     * If a filter for the secondary header is given apply it and create a
     * new, filtered header.
     */

    if (filter2) {

        const cpl_fitsheader *hdr1 = _cpl_fitsdataset_get_header_const(dataset);

        cpl_fitsheader *_hdr = _cpl_fitsheader_create_from_filter(hdr1, filter2);


        if (_hdr == NULL) {

            _cpl_fitsdataset_delete(dataset);
            _cpl_fitsheader_delete(hdr0);

            cpl_error_set_(CPL_ERROR_ILLEGAL_OUTPUT);

            return NULL;

        }

        status = _cpl_fitsdataset_set_header(dataset, _hdr);

        _cpl_fitsheader_delete(_hdr);
        _hdr = NULL;

        if (status) {

            _cpl_fitsdataset_delete(dataset);
            _cpl_fitsheader_delete(hdr0);

            cpl_error_set_(CPL_ERROR_ILLEGAL_OUTPUT);

            return NULL;

        }

    }


    /*
     * If the primary header is not empty, create the new dataset header by
     * joining the primary and the auxiliary headers.
     */

    if ((hdr0 != NULL) && (_cpl_fitsheader_get_size(hdr0) > 0)) {

        cpl_fitsheader *_hdr = _cpl_fitsdataset_get_header(dataset);

        _cpl_fitsheader_join(_hdr, hdr0);
        _cpl_fitsheader_sort(_hdr, _cpl_fitscard_compare_dicb);

    }

    _cpl_fitsheader_delete(hdr0);

    return dataset;

}


inline static cpl_error_code
_cpl_multiframe_append_datagroup(cpl_multiframe *self, const cxchar *id,
                                 const cxchar *filename,
                                 cxsize nsets, cpl_size *positions,
                                 const cpl_regex **filter1,
                                 const cpl_regex **filter2,
                                 const cxchar **links,
                                 cxint flags)
{

    cx_assert(self != NULL);
    cx_assert(id != NULL);
    cx_assert(filename != NULL);
    cx_assert(positions != NULL);

    cx_assert(nsets > 1);
    cx_assert((flags == CPL_MULTIFRAME_ID_PREFIX) || (flags == CPL_MULTIFRAME_ID_JOIN));
    cx_assert((flags != CPL_MULTIFRAME_ID_PREFIX) || (*id != '\0'));


    cxsize is;

    for (is = 0; is < nsets; ++is) {

        if (positions[is] < 1) {

            return cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);

        }

    }


    cxint status = 0;

    cx_map_iterator it = cx_map_find(self->_m_files, filename);

    fitsfile *fp = NULL;


    if (it != cx_map_end(self->_m_files)) {
        fp = cx_map_get_value(self->_m_files, it);
    }
    else {

        fits_open_diskfile(&fp, filename, READONLY, &status);

        if (status) {

            return cpl_error_set_(CPL_ERROR_ASSIGNING_STREAM);

        }

    }


    cpl_fitsdataset *_dataset = _cpl_fitsdataset_new(fp, 0);

    if (_dataset == NULL) {

        fits_close_file(fp, &status);
        return cpl_error_set_(CPL_ERROR_DATA_NOT_FOUND);

    }


    const cxchar *name0 = _cpl_fitsdataset_get_name(_dataset);

    if ((flags == CPL_MULTIFRAME_ID_JOIN) && ((name0 == NULL) || (*name0 == '\0'))) {

        _cpl_fitsdataset_delete(_dataset);
        fits_close_file(fp, &status);

        return cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);

    }


    const cpl_fitsheader *hdr0 = _cpl_fitsdataset_get_header_const(_dataset);


    for (is = 0; is < nsets; ++is) {

        const cpl_regex *_filter1 = (filter1 != NULL) ? filter1[is] : NULL;
        const cpl_regex *_filter2 = (filter2 != NULL) ? filter2[is] : NULL;

        cpl_fitsdataset *dataset = _cpl_multiframe_merge(fp, positions[is], hdr0,
                                                         _filter1, _filter2);

        if (dataset == NULL) {

            cpl_error_code _status = cpl_error_get_code();

            _cpl_fitsdataset_delete(_dataset);

            fits_close_file(fp, &status);
            cpl_error_set_where(cpl_func);

            return _status;

        }

        /*
         * Create and set the new dataset id
         */

        cxchar *name  = cx_strdup(_cpl_fitsdataset_get_name(dataset));

        cx_string *_id;

        if (flags == CPL_MULTIFRAME_ID_JOIN) {

            _id = cx_string_create(name0);

            if (name && (*name != '\0')) {
                cx_string_append(_id, id);
                cx_string_append(_id, name);
            }

        }
        else {

            _id = cx_string_create(id);
            cx_string_append(_id, name);

        }

        // FIXME: The following call does not handle the dataset version and level.
        //        Instead the dataset should be searched for the id, and if
        //        it is already present, the dataset version should be set
        //        to one more than the maximum version which is already present.

        status = _cpl_fitsdataset_set_id(dataset, cx_string_get(_id), 0, 0);

        cx_string_delete(_id);
        _id = NULL;

        if (status) {

            cx_free(name);

            _cpl_fitsdataset_delete(dataset);
            _cpl_fitsdataset_delete(_dataset);

            fits_close_file(fp, &status);

            return cpl_error_set_(CPL_ERROR_ILLEGAL_OUTPUT);

        }


        /*
         * Update dataset links
         */

        if (links) {

            cpl_fitsheader *hdr = _cpl_fitsdataset_get_header(dataset);

            const cxchar **_links = links;
            const cxchar *lnkname;

            while ((lnkname = *_links)) {

                cpl_fitscard *card = _cpl_fitsheader_find(hdr, lnkname);

                if (card) {

                    cx_string *value   = _cpl_fitscard_get_value(card);
                    cx_string *comment = _cpl_fitscard_get_comment(card);

                    cxsize first = cx_string_find_first_not_of(value, "'");
                    cxsize last  = cx_string_find_last_not_of(value, "' ");

                    cx_string *_value = cx_string_substr(value, first, last - first + 1);

                    cx_string_delete(value);

                    value = cx_string_create("'");

                    if (flags == CPL_MULTIFRAME_ID_JOIN) {

                        cx_string_append(value, name0);

                        if (name && (*name != '\0')) {
                            cx_string_append(value, id);
                            cx_string_append(value, cx_string_get(_value));
                        }

                    }
                    else {

                        cx_string_append(value, id);
                        cx_string_append(value, cx_string_get(_value));

                    }

                    cx_string_append(value, "'");

                    _cpl_fitscard_set_value(card, cx_string_get(value), cx_string_get(comment));

                    cx_string_delete(_value);
                    cx_string_delete(comment);
                    cx_string_delete(value);

                }

                ++_links;

            }

        }

        cx_free(name);


        /*
         * Register the dataset.
         */

        cx_deque_push_back(self->_m_datasets, dataset);

    }

    _cpl_fitsdataset_delete(_dataset);


    /*
     * Register the datagroup's fitsfile handle if it has not been registered before.
     */

    if (it == cx_map_end(self->_m_files)) {
        cx_map_insert(self->_m_files, cx_strdup(filename), fp);
    }

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Create a new multi-frame container object.
 *
 * @param head    The first dataset of the multi-frame object
 * @param id      Unique dataset identifier.
 * @param filter  Filter to be applied to the dataset properties on merge.
 *
 * @return
 *   The function returns a newly allocated multi-frame object on success, or
 *   @c NULL otherwise.
 *
 * The function allocates the memory for a multi-frame container and adds the
 * frame @em master as the head, i.e. the first and emtpy dataset of the
 * multi-frame object. A unique dataset identifier @em id may be given. The
 * identifier @em id may be the empty string, in which case it is ignored when
 * writing the multi-frame object to a file. Furthermore a regular expression
 * filter object may be given which will be applied to each of the properties
 * of the dataset @em head. Only those properties of em head, which pass the
 * filter @em filter will be propagated to the created multi-frame container.
 */

cpl_multiframe *
cpl_multiframe_new(const cpl_frame *head, const char *id,
                   cpl_regex *filter)
{

    if ((head == NULL) || (id == NULL)) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return NULL;
    }


    cpl_multiframe *self = cx_malloc(sizeof *self);

    self->_m_files = cx_map_new(_cpl_multiframe_key_compare, cx_free, NULL);
    self->_m_datasets = cx_deque_new();



    const cxchar *filename = cpl_frame_get_filename(head);

    if (filename == NULL) {

        cpl_multiframe_delete(self);
        return NULL;

    }


    cxint status = 0;

    fitsfile *fp;

    fits_open_diskfile(&fp, filename, READONLY, &status);

    if (status) {

        cpl_multiframe_delete(self);
        return NULL;

    }


    cpl_fitsdataset *dataset = _cpl_fitsdataset_new(fp, 0);

    if (dataset == NULL) {

        fits_close_file(fp, &status);
        cpl_multiframe_delete(self);

        return NULL;

    }


    /*
     * Remove keywords which should not appear in an empty HDU
     */

    const cpl_fitsheader *hdr = _cpl_fitsdataset_get_header_const(dataset);

    const cxchar *ignore_key_pattern = "(^BSCALE  =|^BZERO   =|^BUNIT   ="
                                       "|^BLANK   =|^DATAMIN =|^DATAMAX =)";

    cpl_regex *ignore_keys =
            cpl_regex_new(ignore_key_pattern, TRUE,
                          CPL_REGEX_EXTENDED | CPL_REGEX_NOSUBS);

    cpl_fitsheader *_hdr = _cpl_fitsheader_create_from_filter(hdr, ignore_keys);

    cpl_regex_delete(ignore_keys);

    if (_hdr == NULL) {

        _cpl_fitsdataset_delete(dataset);
        fits_close_file(fp, &status);
        cpl_multiframe_delete(self);

        return NULL;

    }


    /*
     * Apply given filter to the pre-processed header
     */

    if (filter) {

        cpl_fitsheader *hdr0 = _cpl_fitsheader_create_from_filter(_hdr, filter);

        if (hdr0 == NULL) {

            _cpl_fitsdataset_delete(dataset);
            fits_close_file(fp, &status);
            cpl_multiframe_delete(self);

            return NULL;

        }

        _cpl_fitsheader_delete(_hdr);
        _hdr = hdr0;

    }


    _cpl_fitsdataset_delete(dataset);


    /*
     * Create a new empty dataset
     */

    cpl_fitsemptyunit *empty = _cpl_fitsemptyunit_new();

    dataset = _cpl_fitsdataset_create(_hdr, (cpl_fitsdataunit *)empty);

    //status = _cpl_fitsdataset_set_header(dataset, _hdr);

    _cpl_fitsemptyunit_delete(empty);
    empty = NULL;

    _cpl_fitsheader_delete(_hdr);
    _hdr = NULL;

    //if (status) {
    if (!dataset) {

        fits_close_file(fp, &status);
        cpl_multiframe_delete(self);

        return NULL;

    }


    status = _cpl_fitsdataset_set_id(dataset, id, 0, 0);

    if (status) {

        _cpl_fitsdataset_delete(dataset);
        fits_close_file(fp, &status);
        cpl_multiframe_delete(self);

        return NULL;

    }


    /*
     * Register dataset
     */

    cx_deque_push_back(self->_m_datasets, dataset);
    cx_map_insert(self->_m_files, cx_strdup(cpl_frame_get_filename(head)), fp);

    return self;

}


/**
 * @brief
 *   Destroys a multi-frame container object
 *
 * @param self  The multi-frame container.
 *
 * @return
 *   Nothing.
 *
 * The function deallocates the multi-frame container @em self.
 */

void
cpl_multiframe_delete(cpl_multiframe *self)
{
    if (self != NULL) {

        if (self->_m_datasets != NULL) {
            cx_deque_destroy(self->_m_datasets,
                             (cx_free_func)_cpl_fitsdataset_delete);
        }

        if (self->_m_files) {

            cxint status = 0;

            cx_map_iterator it = cx_map_begin(self->_m_files);


            while (it != cx_map_end(self->_m_files)) {

                fits_close_file(cx_map_get_value(self->_m_files, it), &status);
                status = 0;

                it = cx_map_next(self->_m_files, it);

            }

            cx_map_delete(self->_m_files);

        }

        cx_free(self);

    }

    return;
}


/**
 * @brief
 *   Get the size of a multi-frame container object.
 *
 * @param self  The multi-frame object.
 *
 * @return
 *   The function returns the current number of datasets referenced by
 *   the multi-frame container.
 *
 * The function returns the number of dataset entries stored in the
 * multi-frame @em self.
 */

cpl_size
cpl_multiframe_get_size(const cpl_multiframe *self)
{

    if (self == NULL) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return 0L;
    }

    return (cpl_size)cx_deque_size(self->_m_datasets);

}


/**
 * @brief
 *   Adds a dataset reference given by position to a multi-frame container
 *   object.
 *
 * @param self     The multi-frame object.
 * @param id       Unique dataset identifier.
 * @param frame    The source data frame from which the dataset is taken.
 * @param position Position of the source dataset in the source data frame.
 * @param filter1  Property filter to apply to the primary header of the
 *                 source dataset.
 * @param filter2  Property filter to apply to the extension header of the
 *                 source dataset.
 * @param flags    Flag controlling the creation of the dataset's target id.
 *
 * @return
 *   The function returns @c CPL_ERROR_NONE on success, and an appropriate
 *   error code otherwise.
 *
 * The function adds a new dataset entry to the multi-frame @em self. The
 * dataset to add is the taken from position @em position of the source data
 * frame @em frame. Before the selected dataset is added to the multi-frame
 * @em self, the dataset's primary and supplementary properties are merged.
 * If the filter arguments are given, i.e. @em filter1 and/or @em filter2 are
 * non @c NULL, they are applied to the primary and the supplementary
 * properties before both are merged.
 *
 * The creation of the dataset's target id is controlled by the argument
 * @em flags. It can be set to one of the values defined by the enumeration
 * cpl_multiframe_id_mode. If @em flags is set to @c CPL_MULTIFRAME_ID_SET,
 * the argument @em id is used as dataset identifier. If @em flags is set to
 * @c CPL_MULTIFRAME_ID_PREFIX then @em id is used as prefix for the dataset's
 * original name (extension name). If the dataset to be appended does not have
 * a name, @em id is used as the full dataset identifier. If @em flags is set
 * to @c CPL_MULTIFRAME_ID_JOIN, the dataset's identifier is created by
 * concatenating the dataset name found in the primary properties, and the
 * dataset name taken from the supplementary properties, using @em id as
 * separator string. If no dataset name is found in the supplementary
 * properties, only the dataset name found in the primary properties is used
 * as identifier and the given separator is not appended. Note that for this
 * last method it is an error if there is no dataset name present in the
 * primary properties of the source dataset.
 *
 * The argument @em id may be the empty string for the methods
 * @c CPL_MULTIFRAME_ID_SET and @c CPL_MULTIFRAME_ID_JOIN. For the method
 * @c CPL_MULTIFRAME_ID_PREFIX this is an error.
 */

cpl_error_code
cpl_multiframe_append_dataset_from_position(cpl_multiframe *self, const char *id,
                                            const cpl_frame *frame, cpl_size position,
                                            const cpl_regex *filter1,
                                            const cpl_regex *filter2,
                                            unsigned int flags)
{

    if ((self == NULL) || (id == NULL) || (frame == NULL)) {

        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    }

    if (position < 0) {

        return cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);

    }

    if ((flags != CPL_MULTIFRAME_ID_SET) &&
            (flags != CPL_MULTIFRAME_ID_PREFIX) &&
            (flags != CPL_MULTIFRAME_ID_JOIN))
    {

        return cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);

    }

    if ((flags == CPL_MULTIFRAME_ID_PREFIX) && (*id == '\0')) {

        return cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);

    }


    const cxchar *filename = cpl_frame_get_filename(frame);

    if (filename == NULL) {

        return cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);

    }


    cxint status = _cpl_multiframe_append_dataset(self, id, filename, position,
                                                  filter1, filter2, flags);

    if (status) {
        cpl_error_set_where(cpl_func);
        return status;
    }

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Adds a dataset reference given by name to a multi-frame container object.
 *
 * @param self     The multi-frame object.
 * @param id       Unique dataset identifier.
 * @param frame    The source data frame from which the dataset is taken.
 * @param name     Name of the source dataset in the source data frame.
 * @param filter1  Property filter to apply to the primary header of the
 *                 source dataset.
 * @param filter2  Property filter to apply to the extension header of the
 *                 source dataset.
 * @param flags    Flag controlling the creation of the dataset's target id.
 *
 * @return
 *   The function returns @c CPL_ERROR_NONE on success, and an appropriate
 *   error code otherwise.
 *
 * The function adds a new dataset entry to the multi-frame @em self. The
 * dataset to add is looked up in the source data frame @em frame using its
 * name @em name. It is an error if no dataset with the given name
 * @em name is found. Before the selected dataset is added to the multi-frame
 * @em self, the dataset's primary and supplementary properties are merged.
 * If the filter arguments are given, i.e. @em filter1 and/or @em filter2 are
 * non @c NULL, they are applied to the primary and the supplementary
 * properties before both are merged.
 *
 * The creation of the dataset's target id is controlled by the argument
 * @em flags. It can be set to one of the values defined by the enumeration
 * cpl_multiframe_id_mode. If @em flags is set to @c CPL_MULTIFRAME_ID_SET,
 * the argument @em id is used as dataset identifier. If @em flags is set to
 * @c CPL_MULTIFRAME_ID_PREFIX then @em id is used as prefix for the dataset's
 * original name (extension name). If the dataset to be appended does not have
 * a name, @em id is used as the full dataset identifier. If @em flags is set
 * to @c CPL_MULTIFRAME_ID_JOIN, the dataset's identifier is created by
 * concatenating the dataset name found in the primary properties, and the
 * dataset name taken from the supplementary properties, using @em id as
 * separator string. If no dataset name is found in the supplementary
 * properties, only the dataset name found in the primary properties is used
 * as identifier and the given separator is not appended. Note that for this
 * last method it is an error if there is no dataset name present in the
 * primary properties of the source dataset.
 *
 * The argument @em id may be the empty string for the methods
 * @c CPL_MULTIFRAME_ID_SET and @c CPL_MULTIFRAME_ID_JOIN. For the method
 * @c CPL_MULTIFRAME_ID_PREFIX this is an error.
 */

cpl_error_code
cpl_multiframe_append_dataset(cpl_multiframe *self, const char *id,
                              const cpl_frame *frame, const char *name,
                              const cpl_regex *filter1,
                              const cpl_regex *filter2,
                              unsigned int flags)
{

    if ((self == NULL) || (id == NULL) || (frame == NULL) || (name == NULL)) {

        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    }


    if ((flags != CPL_MULTIFRAME_ID_SET) &&
            (flags != CPL_MULTIFRAME_ID_PREFIX) &&
            (flags != CPL_MULTIFRAME_ID_JOIN))
    {

        return cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);

    }

    if ((flags == CPL_MULTIFRAME_ID_PREFIX) && (*id == '\0')) {

        return cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);

    }


    const cxchar *filename = cpl_frame_get_filename(frame);

    if (filename == NULL) {

        return cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);

    }


    cxsize position = 0;

    if (strlen(name) > 0) {

        cxssize _position = _cpl_fits_find_extension(filename, name, 0);

        if (_position < 0) {

            return cpl_error_set_(CPL_ERROR_DATA_NOT_FOUND);

        }

        position = _position;

    }


    cxint status = _cpl_multiframe_append_dataset(self, id, filename, position,
                                                  filter1, filter2, flags);

    if (status) {
        cpl_error_set_where(cpl_func);
        return status;
    }

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Adds a group of dataset references given by position to a multi-frame
 *   container object.
 *
 * @param self       The multi-frame object.
 * @param id         Unique dataset identifier.
 * @param frame      The source data frame from which the datasets are taken.
 * @param nsets      The number of datasets to be merged.
 * @param positions  Positions of the source datasets in the source data frame.
 * @param filter1    Property filters to apply to the primary header of each
 *                   source dataset.
 * @param filter2    Property filters to apply to the extension header of each
 *                   source dataset.
 * @param properties Property names to be updated.
 * @param flags      Flag controlling the creation of the dataset's target id.
 *
 * @return
 *   The function returns @c CPL_ERROR_NONE on success, and an appropriate
 *   error code otherwise.
 *
 * The function adds @em nsets new dataset entry to the multi-frame @em self.
 * The datasets to add are taken from the source data frame @em frame and are
 * specified by the first @em nsets positions passed through the array
 * @em positions. Before each selected dataset is added to the multi-frame
 * @em self, the dataset's primary and supplementary properties are merged.
 * If the filter arguments are given, i.e. the respective entries in
 * @em filter1 and/or @em filter2 are non @c NULL, they are applied to the
 * primary and the supplementary properties before both are merged. The arrays
 * @em filter1 and @em filter2 must be given, and they must have @em nsets
 * elements. The array elements, i.e. an individual filter may be set to
 * @c NULL if no filter should be applied.
 *
 * The creation of the dataset's target id is controlled by the argument
 * @em flags. It can be only set to @c CPL_MULTIFRAME_ID_PREFIX or
 * @c CPL_MULTIFRAME_ID_JOIN. If @em flags is set to @c CPL_MULTIFRAME_ID_PREFIX
 * then @em id is used as prefix for the current dataset's original name
 * (extension name). If the dataset to be appended does not have a name,
 * @em id is used as the dataset identifier. If @em flags is set to
 * @c CPL_MULTIFRAME_ID_JOIN, the dataset's identifier is created by
 * concatenating the dataset name found in the primary properties, and the
 * dataset name taken from the supplementary properties, using @em id as
 * separator string. If no dataset name is found in the supplementary
 * properties, only the dataset name found in the primary properties is used
 * as identifier and the given separator is not appended. Note that for this
 * last method it is an error if there is no dataset name present in the
 * primary properties of the source dataset.
 *
 * The argument @em id may be the empty string for the method
 * @c CPL_MULTIFRAME_ID_JOIN. For the method @c CPL_MULTIFRAME_ID_PREFIX this
 * is an error.
 *
 * If @em properties is given it has to be a @c NULL terminated array of
 * property names. For each specified property name their value is changed
 * according to the naming scheme selected by @em flags, i.e. the value is
 * either prefixed by @em id, or it is set to the concatenation of the source
 * dataset name found in its primary properties, @em id, and its original value.
 * This can be used to correctly change properties used to reference one of
 * the other datasets in the given group through their value. If a given
 * property is not found, it is ignored.
 */

cpl_error_code
cpl_multiframe_append_datagroup_from_position(cpl_multiframe *self, const char *id,
                                              const cpl_frame *frame,
                                              cpl_size nsets, cpl_size *positions,
                                              const cpl_regex **filter1,
                                              const cpl_regex **filter2,
                                              const char **properties,
                                              unsigned int flags)
{

    if ((self == NULL) || (id == NULL) || (frame == NULL)) {

        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    }

    if (positions == NULL) {

        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    }

    if (nsets < 2) {

        return cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);

    }

    if ((flags != CPL_MULTIFRAME_ID_PREFIX) && (flags != CPL_MULTIFRAME_ID_JOIN))
    {

        return cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);

    }

    if ((flags == CPL_MULTIFRAME_ID_PREFIX) && (*id == '\0')) {

        return cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);

    }


    const cxchar *filename = cpl_frame_get_filename(frame);

    if (filename == NULL) {

        return cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);

    }

    cxint status = _cpl_multiframe_append_datagroup(self, id, filename, nsets, positions,
                                                    filter1, filter2, properties, flags);

    if (status) {
        cpl_error_set_where(cpl_func);
        return status;
    }

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Adds a group of dataset references given by name to a multi-frame
 *   container object.
 *
 * @param self       The multi-frame object.
 * @param id         Unique dataset identifier.
 * @param frame      The source data frame from which the datasets are taken.
 * @param nsets      The number of datasets to be merged.
 * @param names      The names of the source datasets in the source data frame.
 * @param filter1    Property filters to apply to the primary header of each
 *                   source dataset.
 * @param filter2    Property filters to apply to the extension header of each
 *                   source dataset.
 * @param properties Property names to be updated.
 * @param flags      Flag controlling the creation of the dataset's target id.
 *
 * @return
 *   The function returns @c CPL_ERROR_NONE on success, and an appropriate
 *   error code otherwise.
 *
 * The function is equivalent to cpl_multiframe_append_datagroup_from_position(),
 * but the source datasets to be added are looked up in the source data frame
 * @em frame using their names given in the array @em names.
 */

cpl_error_code
cpl_multiframe_append_datagroup(cpl_multiframe *self, const char *id,
                                const cpl_frame *frame,
                                cpl_size nsets, const char **names,
                                const cpl_regex **filter1,
                                const cpl_regex **filter2,
                                const char **properties,
                                unsigned int flags)
{

    if ((self == NULL) || (id == NULL) || (frame == NULL)) {

        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    }

    if (names == NULL) {

        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    }

    if (nsets < 2) {

        return cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);

    }

    if ((flags != CPL_MULTIFRAME_ID_PREFIX) && (flags != CPL_MULTIFRAME_ID_JOIN))
    {

        return cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);

    }

    if ((flags == CPL_MULTIFRAME_ID_PREFIX) && (*id == '\0')) {

        return cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);

    }


    const cxchar *filename = cpl_frame_get_filename(frame);

    if (filename == NULL) {

        return cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);

    }

    cxssize is;

    cpl_size *positions = cx_calloc(nsets, sizeof(cpl_size));

    for (is = 0; is < nsets; ++is) {

        cxint _position = cpl_fits_find_extension(filename, names[is]);

        if (_position > 0) {
            positions[is] = _position;
        }
        else {

            cx_free(positions);

            return cpl_error_set_(CPL_ERROR_DATA_NOT_FOUND);

        }

    }

    cxint status = _cpl_multiframe_append_datagroup(self, id, filename, nsets, positions,
                                                    filter1, filter2, properties, flags);

    cx_free(positions);
    positions = NULL;

    if (status) {
        cpl_error_set_where(cpl_func);
        return status;
    }

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Add a placeholder to a multi-frame container.
 *
 * @param self The multi-frame object.
 * @param id   Unique dataset identifier.
 *
 * @return
 *   The function returns @c CPL_ERROR_NONE on success, and an appropriate
 *   error code otherwise.
 *
 * The function adds an empty dataset, a placeholder to a multi-frame
 * container. An empty dataset is special since it is not attached to an
 * underlying data file. When the multi-frame object is written to a file,
 * an empty dataset appears as a named, but otherwise empty unit.
 */

cpl_error_code
cpl_multiframe_add_empty(cpl_multiframe *self, const char *id)
{

    if ((self == NULL) || (id == NULL) || (*id == '\0')) {

        return cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);

    }

    cpl_fitsheader   *hdr   = _cpl_fitsheader_new();
    cpl_fitsemptyunit *data = _cpl_fitsemptyunit_new();

    if ((hdr == NULL) || (data == NULL)) {

        return cpl_error_set_(CPL_ERROR_ILLEGAL_OUTPUT);

    }


    cpl_fitsdataset *dataset = _cpl_fitsdataset_create(hdr, (cpl_fitsdataunit *)data);

    _cpl_fitsheader_delete(hdr);
    _cpl_fitsemptyunit_delete(data);

    if (dataset == NULL) {

        return cpl_error_set_(CPL_ERROR_ILLEGAL_OUTPUT);

    }


    // FIXME: The following call does not handle the dataset version and level.
    //        Instead the dataset should be searched for the id, and if
    //        it is already present, the dataset version should be set
    //        to one more than the maximum version which is already present.

    cxint status = _cpl_fitsdataset_set_id(dataset, id, 0, 0);

    if (status) {

        return cpl_error_set_(CPL_ERROR_ILLEGAL_OUTPUT);

    }


    /*
     * Register the empty dataset.
     */

    cx_deque_push_back(self->_m_datasets, dataset);


    return CPL_ERROR_NONE;

}


/**
 * @brief
 *  Write a multi-frame container to a file.
 *
 * @param self     The multi-frame container object.
 * @param filename Name of the file to which the multi-frame object is written.
 *
 * @return
 *   The function returns @c CPL_ERROR_NONE on success, and an appropriate
 *   error code otherwise.
 *
 * The function writes the multi-frame object @em self to the file @em filename.
 * A multi-frame object contains only the properties and the references to
 * the data units, which may be located in different files. Only when this
 * function is called the all referenced datasets are copied and written to
 * the output file.
 */

cpl_error_code
cpl_multiframe_write(cpl_multiframe *self, const char *filename)
{

    if ((self == NULL) || (filename == NULL)) {

        return cpl_error_set_(CPL_ERROR_NULL_INPUT);

    }


    cxint status = 0;

    fitsfile *fp = NULL;


    fits_create_file(&fp, filename, &status);

    if (status == FILE_NOT_CREATED) {

        cx_string *_filename = cx_string_create("!");

        cx_string_append(_filename, filename);

        status = 0;
        fits_create_file(&fp, cx_string_get(_filename), &status);

        cx_string_delete(_filename);
        _filename = NULL;

    }

    if (status) {

        return cpl_error_set_(CPL_ERROR_FILE_NOT_CREATED);

    }


    cxsize sz = cx_deque_size(self->_m_datasets);

    if (sz == 0) {

        fits_close_file(fp, &status);
        return cpl_error_set_(CPL_ERROR_DATA_NOT_FOUND);

    }


    /*
     * Write datasets to the output file
     */

    cpl_fitsdataset *dataset = cx_deque_get(self->_m_datasets, 0);

    _cpl_fitsheader_sort(_cpl_fitsdataset_get_header(dataset), _cpl_fitscard_compare_dicb);
    _cpl_fitsdataset_write(dataset, fp, TRUE, FALSE);

    cxsize i;

    for (i = 1; i < sz; ++i) {

        dataset = cx_deque_get(self->_m_datasets, i);

        _cpl_fitsheader_sort(_cpl_fitsdataset_get_header(dataset), _cpl_fitscard_compare_dicb);
        _cpl_fitsdataset_write(dataset, fp, FALSE, FALSE);

    }


    /*
     * Update file creation timestamp
     */

    int type;

    fits_movabs_hdu(fp, 1, &type, &status);
    fits_write_date(fp, &status);


    status = 0;
    fits_close_file(fp, &status);

    return CPL_ERROR_NONE;

}
/**@}*/
