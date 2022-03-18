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

#include <stdio.h>
#include <string.h>

#include <cxmultimap.h>
#include <cxlist.h>
#include <cxmemory.h>
#include <cxmessages.h>
#include <cxstrutils.h>
#include <cxutils.h>

#include <cpl_msg.h>
#include "cpl_frameset.h"
#include "cpl_errorstate.h"
#include "cpl_error_impl.h"


/**
 * @defgroup cpl_frameset Frame Sets
 *
 * The module implements a container type for frames. Frames can be stored
 * in a frame set and retrieved, either by searching for a particular
 * frame tag or by sequential access. Frame sets can be created, filled and
 * saved to a so called `set of frames' file or loaded from such a file.
 *
 * @par Synopsis:
 * @code
 *   #include <cpl_frameset.h>
 * @endcode
 */

/**@{*/

/*
 * The frame set type.
 */

enum _cpl_frameset_cacheid {
    TAG,
    POS,
    ALL
};

typedef enum _cpl_frameset_cacheid cpl_frameset_cacheid;


struct _cpl_frameset_cache_ {
    cx_multimap_iterator tag;
    cx_multimap_iterator pos;
};

typedef struct _cpl_frameset_cache_ cpl_frameset_cache;


struct _cpl_frameset_ {
    cx_multimap *frames;
    cx_list *history;

    cpl_frameset_cache cache;
};


/*
 * Private methods
 */

inline static cxbool
_cpl_frameset_cache_empty(const cpl_frameset *self, cpl_frameset_cacheid id)
{

    cxbool state;


    switch (id) {
    case TAG:
        state = self->cache.tag == NULL;
        break;

    case POS:
        state = self->cache.pos == NULL;
        break;

    case ALL:
        state = (self->cache.tag == NULL && self->cache.pos == NULL);
        break;
    }

    return state;

}


/*
  @internal

  @brief Reset the internal cache of the frameset

  @param self     The frameset
  @param id       The caching type (TAG/POS/ALL)

  @return void
  The internal modification means that even though the accessor
  cpl_frameset_get_next_first()
  has only const modifiers, the call does have side-effects, and it can thus
  not be subjected to optimizations applicable to 'pure' functions (e.g. to
  call the function fewer times than the program says).

  @note The internal (private) members of self are modified! The const
        modifier is still used, because this change is not visible outside
        this module.

 */

inline static void
_cpl_frameset_cache_reset(const cpl_frameset *self, cpl_frameset_cacheid id)
{

    cpl_frameset * myself = (cpl_frameset *)self;

    switch (id) {
    case TAG:
        myself->cache.tag = NULL;
        break;

    case POS:
        myself->cache.pos = NULL;
        break;

    case ALL:
        myself->cache.tag = NULL;
        myself->cache.pos = NULL;
        break;
    }

    return;

}



/*
  @internal

  @brief Update the internal cache of the frameset

  @param self     The frameset
  @param id       The caching type (TAG/POS/ALL)
  @param position The position to cache

  @return void

  The internal modification means that even though the accessor
  cpl_frameset_get_next_const()
  has only const modifiers, the call does have side-effects, and it can thus
  not be subjected to optimizations applicable to 'pure' functions (e.g. to
  call the function fewer times than the program says).

  @note The internal (private) members of self are modified! The const
        modifier is still used, because this change is not visible outside
        this module.

 */

inline static void
_cpl_frameset_cache_push(const cpl_frameset *self, cpl_frameset_cacheid id,
                         cx_multimap_const_iterator position)
{

    cpl_frameset * myself = (cpl_frameset *)self;

    switch (id) {
    case TAG:
        myself->cache.tag = (cx_multimap_iterator)position;
        break;

    case POS:
        myself->cache.pos = (cx_multimap_iterator)position;
        break;

    case ALL:
        myself->cache.tag = (cx_multimap_iterator)position;
        myself->cache.pos = (cx_multimap_iterator)position;
        break;
    }

    return;

}


inline static cx_multimap_iterator
_cpl_frameset_cache_get(const cpl_frameset *self, cpl_frameset_cacheid id)
{

    cxptr entry;

    switch (id) {
    case TAG:
        entry = self->cache.tag;
        break;

    case POS:
        entry = self->cache.pos;
        break;

    default:
        entry = NULL;
        break;
    }

    return entry;

}

static cxbool
_cpl_frameset_compare(cxcptr s, cxcptr t)
{

    return strcmp(s, t) < 0 ? TRUE : FALSE;

}


/*
 * Get the first frame in the set with the provided tag, or NULL if such
 * a frame does not exist.
 */

inline static cpl_frame *
_cpl_frameset_get(const cpl_frameset *self, const char *tag)
{

    cx_multimap_iterator entry;


    entry = cx_multimap_lower_bound(self->frames, tag);

    if (entry == cx_multimap_upper_bound(self->frames, tag))
        return NULL;

    return cx_multimap_get_value(self->frames, entry);

}


/*
 *  Helper functions for sorting frame sets
 */

inline static void
_cpl_frameset_history_merge(cx_list *self, cx_list *other,
                            const cx_multimap *frames,
                            cpl_frame_compare_func compare)
{

    cx_assert((self != NULL) && (other != NULL));
    cx_assert(frames != NULL);
    cx_assert(compare != NULL);


    if (self != other) {

        cx_list_iterator first1 = cx_list_begin(self);
        cx_list_iterator last1  = cx_list_end(self);

        cx_list_iterator first2 = cx_list_begin(other);
        cx_list_iterator last2  = cx_list_end(other);

        while ((first1 != last1) && (first2 != last2)) {

            cx_multimap_const_iterator node1 = cx_list_get(self, first1);
            cx_multimap_const_iterator node2 = cx_list_get(other, first2);

            if (compare(cx_multimap_get_value(frames, node1),
                        cx_multimap_get_value(frames, node2)) > 0) {

                cx_list_iterator next = cx_list_next(other, first2);

                cx_list_splice(self, first1, other,first2, next);
                first2 = next;

            }
            else {
                first1 = cx_list_next(self, first1);
            }

        }

        if (first2 != last2) {
            cx_list_splice(self, last1, other, first2, last2);
        }

    }

    return;

}


inline static cpl_error_code
_cpl_frameset_history_sort(cx_list *self, const cx_multimap *frames,
                           cpl_frame_compare_func compare)
{

    if (cx_list_size(self) > 1) {

        cx_list *tmp = cx_list_new();
        cx_list_iterator position = cx_list_begin(self);

        cxsize middle = cx_list_size(self) / 2;

        while (middle--) {
            position = cx_list_next(self, position);
        }

        cx_list_splice(tmp, cx_list_begin(tmp),
                       self, position, cx_list_end(self));

        _cpl_frameset_history_sort(self, frames, compare);
        _cpl_frameset_history_sort(tmp, frames, compare);

        _cpl_frameset_history_merge(self, tmp, frames, compare);

        cx_assert(cx_list_empty(tmp));
        cx_list_delete(tmp);

    }

    return CPL_ERROR_NONE;

}


inline static cpl_frame *
_cpl_frameset_get_first(const cpl_frameset *self)
{

    cx_list_iterator first;
    cx_multimap_iterator position;


    first = cx_list_begin(self->history);
    if (first == cx_list_end(self->history)) {
        return NULL;
    }

    position = cx_list_get(self->history, first);
    cx_assert(position != cx_multimap_end(self->frames));

    /* Will modify internals of self! */
    _cpl_frameset_cache_push(self, POS, position);

    return cx_multimap_get_value(self->frames, position);

}


inline static cpl_frame *
_cpl_frameset_get_next(const cpl_frameset *self)
{

    cx_list_iterator first;
    cx_list_iterator last;

    cx_multimap_iterator next;


    cx_assert(_cpl_frameset_cache_get(self, POS) != NULL);


    first = cx_list_begin(self->history);
    last = cx_list_end(self->history);

    while (first != last) {

        next = cx_list_get(self->history, first);

        if (next == _cpl_frameset_cache_get(self, POS)) {
            break;
        }

        first = cx_list_next(self->history, first);

    }

    first =  cx_list_next(self->history, first);
    if (first == cx_list_end(self->history)) {
        return NULL;
    }

    next = cx_list_get(self->history, first);
    if (next == cx_multimap_end(self->frames)) {
        return NULL;
    }

    /* Will modify internals of self! */
    _cpl_frameset_cache_push(self, POS, next);

    return cx_multimap_get_value(self->frames, next);

}


/*
 * Public methods
 */

/**
 * @brief
 *   Create a new, empty frame set.
 *
 * @return
 *   The handle for the newly created frame set.
 *
 * The function allocates the memory for the new frame set, initialises the
 * set to be empty and returns a handle for it.
 */

cpl_frameset *
cpl_frameset_new(void)
{

    cpl_frameset *self = cx_malloc(sizeof *self);


    self->frames = cx_multimap_new(_cpl_frameset_compare, NULL,
                                  (cx_free_func)cpl_frame_delete);
    self->history = cx_list_new();
    _cpl_frameset_cache_reset(self, ALL);

    return self;

}


/**
 * @brief
 *   Create a copy of the given frame set.
 *
 * @param other  The frame set to be copied.
 *
 * @return
 *   A handle for the created clone. The function returns @c NULL if an
 *   error occurs and sets an appropriate error code.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The parameter <i>other</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The function creates a deep copy, i.e. the frame set object and its
 * contents, of the frame set @em other. The created copy and the original
 * set do not share any resources.
 */

cpl_frameset *
cpl_frameset_duplicate(const cpl_frameset *other)
{


    cx_list_iterator first, last;

    cpl_frameset *self = NULL;


    if (other == NULL) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return NULL;
    }

    self = cpl_frameset_new();

    first = cx_list_begin(other->history);
    last = cx_list_end(other->history);

    while (first != last) {

        cx_multimap_iterator position = cx_list_get(other->history, first);

        cpl_frame *frame = cx_multimap_get_value(other->frames, position);
        cpl_frame *tmp = cpl_frame_duplicate(frame);
        cxptr key = (cxptr)cpl_frame_get_tag(tmp);

        position = cx_multimap_insert(self->frames, key, tmp);
        cx_list_push_back(self->history, position);

        first = cx_list_next(other->history, first);

    }

    _cpl_frameset_cache_reset(self, ALL);

    return self;

}


/**
 * @brief
 *    Destroy a frame set.
 *
 * @param self  The frame set to destroy.
 *
 * @return Nothing.
 *
 * The function destroys the frame set @em self and its whole contents.
 * If @em self is @c NULL, nothing is done and no error is set.
 */

void
cpl_frameset_delete(cpl_frameset *self)
{

    if (self) {
        cx_multimap_delete(self->frames);
        cx_list_delete(self->history);
        cx_free(self);
    }

    return;

}


/**
 * @brief
 *    Get the current size of a frame set.
 *
 * @param self  A frame set.
 *
 * @return
 *   The frame set's current size, or 0 if it is empty. The function
 *   returns 0 if an error occurs and sets an appropriate error code.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The parameter <i>self</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The reports the current number of frames stored in the frame set
 * @em self.
 */

cpl_size
cpl_frameset_get_size(const cpl_frameset *self)
{



    if (self == NULL) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return 0;
    }

    return cx_multimap_size(self->frames);

}


/**
 * @brief
 *   Check whether a frame set is empty.
 *
 * @param self  A frame set.
 *
 * @return
 *   The function returns 1 if the set is empty, and 0 otherwise. If an
 *   error occurs 0 is returned and an appropriate error code is set.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The parameter <i>self</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The function checks if @em self contains any frames.
 */

int
cpl_frameset_is_empty(const cpl_frameset *self)
{



    if (self == NULL) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return 0;
    }

    return cx_multimap_empty(self->frames);

}


/**
 * @brief
 *   Counts the frames stored in a frame set having the given tag.
 *
 * @param self  A frame set.
 * @param tag   The frame tag.
 *
 * @return
 *   The number of frames with tag @em tag. The function returns 0 if an
 *   error occurs and sets an appropriate error code.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The parameter <i>self</i> or <i>tag</i> is a <tt>NULL</tt>
 *         pointer.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The function scans the frame set @em self for frames with the tag @em tag
 * and returns the number of frames found.
 */

int
cpl_frameset_count_tags(const cpl_frameset *self, const char *tag)
{



    if (self == NULL || tag == NULL) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return 0;
    }

    return cx_multimap_count(self->frames, tag);

}


/**
 * @brief
 *   Find a frame with the given tag in a frame set.
 *
 * @param self  A frame set.
 * @param tag   The frame tag to search for.
 *
 * @return
 *   The handle for a frame with tag @em tag, or @c NULL if no
 *   such frame was found. The function returns @c NULL if an error
 *   occurs and sets an appropriate error code.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The parameter <i>self</i> or <i>tag</i> is a <tt>NULL</tt>
 *         pointer.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The function searches the frame set @em self for the frames with the
 * tag @em tag.  If such a frame is present, a handle for it is returned.
 * If the set contains several frames with the tag @em tag the first
 * one is returned. The remaining frames with this tag can be accessed
 * sequentially by using @c NULL as tag when calling this function
 * repeatedly, since the most recent frame accessed is cached. This
 * cache is reset whenever the provided tag is not @c NULL. If no frame
 * with the tag @em tag is present in @em self or no more frames with
 * this tag are found the function returns @c NULL.
 *
 * @note
 *   Since the most recently accessed frame is cached in the frameset
 *   this function is not re-entrant!
 */

const cpl_frame *
cpl_frameset_find_const(const cpl_frameset *self, const char *tag)
{


    cx_multimap_iterator pos;
    cx_multimap_iterator cached;

    cpl_frame *frame;



    if (self == NULL) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return NULL;
    }

    if (tag) {

        /*
         * Find the first frame with this tag in the set. Make sure that
         * we really found something.
         */

        pos = cx_multimap_lower_bound(self->frames, tag);

        if (pos == cx_multimap_upper_bound(self->frames, tag)) {
            /* Will modify internals of self! */
            _cpl_frameset_cache_reset(self, TAG);
            return NULL;
        }

    }
    else {

        cxptr last_tag;


        /*
         * Check that the cache is not empty.
         */

        if (_cpl_frameset_cache_empty(self, TAG)) {
            return NULL;
        }

        cached = _cpl_frameset_cache_get(self, TAG);

        /*
         * Get the successor of the cached entry.
         */

        last_tag = cx_multimap_get_key(self->frames, cached);
        pos = cx_multimap_next(self->frames, cached);

        if (pos == cx_multimap_upper_bound(self->frames, last_tag)) {
            return NULL;
        }

    }

    /* Will modify internals of self! */
    _cpl_frameset_cache_push(self, TAG, pos);
    frame = cx_multimap_get_value(self->frames, pos);

    return frame;

}

/**
 * @brief
 *   Find a frame with the given tag in a frame set.
 *
 * @param self  A frame set.
 * @param tag   The frame tag to search for.
 *
 * @return
 *   The handle for a frame with tag @em tag, or @c NULL if no
 *   such frame was found. The function returns @c NULL if an error
 *   occurs and sets an appropriate error code.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The parameter <i>self</i> or <i>tag</i> is a <tt>NULL</tt>
 *         pointer.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The function searches the frame set @em self for the frames with the
 * tag @em tag.  If such a frame is present, a handle for it is returned.
 * If the set contains several frames with the tag @em tag the first
 * one is returned. The remaining frames with this tag can be accessed
 * sequentially by using @c NULL as tag when calling this function
 * repeatedly, since the most recent frame accessed is cached. This
 * cache is reset whenever the provided tag is not @c NULL. If no frame
 * with the tag @em tag is present in @em self or no more frames with
 * this tag are found the function returns @c NULL.
 *
 * @note
 *   Since the most recently accessed frame is cached in the frameset
 *   this function is not re-entrant!
 */

cpl_frame *
cpl_frameset_find(cpl_frameset *self, const char *tag)
{

    cpl_errorstate prestate = cpl_errorstate_get();
    cpl_frame *frame = (cpl_frame *)cpl_frameset_find_const(self, tag);

    if (!cpl_errorstate_is_equal(prestate))
        (void)cpl_error_set_where(cpl_func);

    return frame;
}


/**
 * @brief
 *   Get the first frame in the given set.
 *
 * @param self  A frame set.
 *
 * @return
 *   A handle for the first frame in the set, or @c NULL if the set
 *   is empty. The function returns @c NULL if an error occurs and
 *   sets an appropriate error code.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The parameter <i>self</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The function returns the first frame in the frame set @em self if it
 * exists. If a first frame does not exist, i.e. the frame set is empty,
 * @c NULL is returned. The function also updates the internal cache.
 *
 * @see cpl_frameset_get_next_const()
 *
 * @deprecated
 *   This function will be removed from CPL version 7. Code using these
 *   functions should be ported to make use of frame set iterators instead!
 */

const cpl_frame *
cpl_frameset_get_first_const(const cpl_frameset *self)
{


    if (self == NULL) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return NULL;
    }

    return (const cpl_frame *)_cpl_frameset_get_first(self);

}

/**
 * @brief
 *   Get the first frame in the given set.
 *
 * @param self  A frame set.
 *
 * @return
 *   A handle for the first frame in the set, or @c NULL if the set
 *   is empty. The function returns @c NULL if an error occurs and
 *   sets an appropriate error code.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The parameter <i>self</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The function returns the first frame in the frame set @em self if it
 * exists. If a first frame does not exist, i.e. the frame set is empty,
 * @c NULL is returned. The function also updates the internal cache.
 *
 * @see cpl_frameset_get_next()
 *
 * @deprecated
 *   This function will be removed from CPL version 7. Code using these
 *   functions should be ported to make use of frame set iterators instead!
 */

cpl_frame *
cpl_frameset_get_first(cpl_frameset *self)
{


    if (self == NULL) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return NULL;
    }

    return _cpl_frameset_get_first(self);

}

/**
 * @brief
 *   Get the next frame in the given set.
 *
 * @param self  A frame set.
 *
 * @return
 *   A handle for the next frame in a set. If there are no more
 *   frames in the set the function returns @c NULL. The function returns
 *   @c NULL if an error occurs and sets an appropriate error code.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The parameter <i>self</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The function returns the next frame in the frame set @em self if it
 * exists and otherwise @c NULL. The function uses the internal cache
 * to determine the most recently accessed frame. This means that the
 * function only works as expected if @em self has been initialised by
 * a call to @b cpl_frameset_get_first_const(), and if no function updating the
 * internal cache was called between two subsequent calls to this
 * function.
 *
 * @see cpl_frameset_get_first_const()
 *
 * @deprecated
 *   This function will be removed from CPL version 7. Code using these
 *   functions should be ported to make use of frame set iterators instead!
 */

const cpl_frame *
cpl_frameset_get_next_const(const cpl_frameset *self)
{



    if (self == NULL) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return NULL;
    }

    return (const cpl_frame *)_cpl_frameset_get_next(self);

}


/**
 * @brief
 *   Get the next frame in the given set.
 *
 * @param self  A frame set.
 *
 * @return
 *   A handle for the next frame in a set. If there are no more
 *   frames in the set the function returns @c NULL. The function returns
 *   @c NULL if an error occurs and sets an appropriate error code.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The parameter <i>self</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The function returns the next frame in the frame set @em self if it
 * exists and otherwise @c NULL. The function uses the internal cache
 * to determine the most recently accessed frame. This means that the
 * function only works as expected if @em self has been initialised by
 * a call to @b cpl_frameset_get_first(), and if no function updating the
 * internal cache was called between two subsequent calls to this
 * function.
 *
 * @see cpl_frameset_get_first()
 *
 * @deprecated
 *   This function will be removed from CPL version 7. Code using these
 *   functions should be ported to make use of frame set iterators instead!
 */

cpl_frame *
cpl_frameset_get_next(cpl_frameset *self)
{


    if (self == NULL) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return NULL;
    }

    return _cpl_frameset_get_next(self);

}
/**
 * @brief
 *   Insert a frame into the given frame set.
 *
 * @param self    A frame set.
 * @param frame   The frame to insert.
 *
 * @return
 *   The function returns @c CPL_ERROR_NONE on success or a CPL error
 *   code otherwise.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The parameter <i>self</i> or <i>frame</i> is a <tt>NULL</tt>
 *         pointer.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ILLEGAL_INPUT</td>
 *       <td class="ecr">
 *         The parameter <i>frame</i> has an invalid tag.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The function adds the frame @em frame to the frame set @em self using the
 * frame's tag as key.
 *
 * The insertion of a frame into a frameset transfers the ownership of the
 * frame @em frame to the frameset @em self. This means that the frame must
 * not be deallocated through the pointer @em frame.
 *
 * In addition, the frame pointer returned by any member function call
 * returning a handle to a frameset's member frame, must not be used to
 * insert the returned frame into another framset without prior duplication
 * of this frame, and, it must not be used to modify the frames tag without
 * removing it from the frameset first and re-inserting it with the new
 * tag afterwards.
 */

cpl_error_code
cpl_frameset_insert(cpl_frameset *self, cpl_frame *frame)
{



    const cxchar *tag = NULL;

    cx_multimap_iterator position;


    if (self == NULL || frame == NULL) {
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);
    }

    tag = cpl_frame_get_tag(frame);

    if (tag == NULL) {
        return cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);
    }

    position = cx_multimap_insert(self->frames, tag, frame);
    cx_list_remove(self->history, position);
    cx_list_push_back(self->history, position);

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Erase all frames with the given tag from a frame set.
 *
 * @param self  A frame set.
 * @param tag   The tag used to locate the frames to remove.
 *
 * @return
 *   The function returns the number of frames removed. If an error occurs
 *   0 is returned and an appropriate error code is set.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The parameter <i>self</i> or <i>tag</i> is a <tt>NULL</tt>
 *         pointer.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The function searches the frame set @em self for frames having the
 * tag @em tag and removes them from the set. The removed frames are
 * destroyed. If no frame with the tag @em tag is found the function
 * has no effect.
 */

cpl_size
cpl_frameset_erase(cpl_frameset *self, const char *tag)
{


    cxsize count = 0;

    cx_multimap_iterator first;
    cx_multimap_iterator last;
    cx_multimap_iterator position;


    if (self == NULL || tag == NULL) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return count;
    }

    cx_multimap_equal_range(self->frames, tag, &first, &last);

    position = first;

    while (position != last) {

        cx_assert(strcmp(cx_multimap_get_key(self->frames, position),
                         tag) == 0);

        if (_cpl_frameset_cache_get(self, TAG) == position) {

            cx_multimap_iterator p;

            p = cx_multimap_previous(self->frames, position);

            if (p == cx_multimap_end(self->frames)) {
                p = cx_multimap_begin(self->frames);
            }

            _cpl_frameset_cache_push(self, TAG, p);
        }

        if (_cpl_frameset_cache_get(self, POS) == position) {

            cx_list_iterator p = cx_list_begin(self->history);

            while (cx_list_get(self->history, p) != position &&
                   p != cx_list_end(self->history)) {
                p = cx_list_next(self->history, p);
            }

            cx_assert(p != cx_list_end(self->history));
            p = cx_list_previous(self->history, p);

            if (p == cx_list_end(self->history)) {
                p = cx_list_begin(self->history);
            }

            _cpl_frameset_cache_push(self, POS, cx_list_get(self->history, p));

        }

        ++count;

        cx_list_remove(self->history, position);
        position = cx_multimap_next(self->frames, position);

    }

    cx_multimap_erase_range(self->frames, first, last);

    return count;

}


/**
 * @brief
 *   Erase the given frame from a frame set.
 *
 * @param self    A frame set.
 * @param frame   The frame to remove.
 *
 * @return
 *   The function returns @c CPL_ERROR_NONE on success or a CPL error
 *   code otherwise.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The parameter <i>self</i> or <i>frame</i> is a <tt>NULL</tt>
 *         pointer.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The function searches the frame set @em self for the first occurrance
 * of @em frame. If it is present, the frame is removed from the set and
 * destroyed. If frame is not present in @em self the function has no
 * effect.
 */

cpl_error_code
cpl_frameset_erase_frame(cpl_frameset *self, cpl_frame *frame)
{


    cx_multimap_iterator first, last;


    if (self == NULL) {
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);
    }

    if (!frame) {
        return CPL_ERROR_NONE;
    }

    first = cx_multimap_lower_bound(self->frames, cpl_frame_get_tag(frame));
    last = cx_multimap_upper_bound(self->frames, cpl_frame_get_tag(frame));

    while (first != last) {
        if (cx_multimap_get_value(self->frames, first) == frame) {

            if (_cpl_frameset_cache_get(self, TAG) == first) {

                cx_multimap_iterator p;

                p = cx_multimap_previous(self->frames, first);

                if (p == cx_multimap_end(self->frames)) {
                    p = cx_multimap_begin(self->frames);
                }

                _cpl_frameset_cache_push(self, TAG, p);
            }

            if (_cpl_frameset_cache_get(self, POS) == first) {

                cx_list_iterator p = cx_list_begin(self->history);

                while (cx_list_get(self->history, p) != first &&
                       p != cx_list_end(self->history)) {
                    p = cx_list_next(self->history, p);
                }

                cx_assert(p != cx_list_end(self->history));
                p = cx_list_previous(self->history, p);

                if (p == cx_list_end(self->history)) {
                    p = cx_list_begin(self->history);
                }

                _cpl_frameset_cache_push(self, POS,
                                         cx_list_get(self->history, p));

            }

            cx_multimap_erase_position(self->frames, first);
            cx_list_remove(self->history, first);
            break;
        }

        first = cx_multimap_next(self->frames, first);
    }

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Join two frame sets.
 *
 * @param self    The target frame set
 * @param other   The source frame set
 *
 * @return
 *   The function returns @c CPL_ERROR_NONE on success or a CPL error
 *   code otherwise.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The parameter <i>self</i> or <i>frame</i> is a <tt>NULL</tt>
 *         pointer.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The function adds the contents of the frame set @em other to @em self by
 * inserting copies of the elements of the frame set @em other. If the source
 * frame set @em other is @c NULL, or if it is empty, the function has no
 * effect.
 */

cpl_error_code
cpl_frameset_join(cpl_frameset *self, const cpl_frameset *other)
{



    if (self == NULL) {
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);
    }

    if ((other != NULL) && (cpl_frameset_is_empty(other) != 1)) {

        cx_multimap_iterator position = cx_multimap_begin(other->frames);

        cpl_error_code status = CPL_ERROR_NONE;


        while (position != cx_multimap_end(other->frames)) {

            cpl_frame *frame  = cx_multimap_get_value(other->frames, position);

            frame = cpl_frame_duplicate(frame);

            /*
             * Use high level insertion here, so that insertion order tracking
             * is handled implicitly
             */

            status = cpl_frameset_insert(self, frame);

            if (status != CPL_ERROR_NONE) {
                cpl_frame_delete(frame);
                cpl_error_set_(status);

                return status;
            }

            position = cx_multimap_next(other->frames, position);

        }

    }

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Sort a frame set.
 *
 * @param self     The frame set to sort.
 * @param compare  Comparison function for frames.
 *
 * @return
 *   The function returns @c CPL_ERROR_NONE on success, or an appropriate
 *   CPL error code otherwise.
 *
 * @error
 * *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The parameter <i>self</i> or <i>compare</i> is a <tt>NULL</tt>
 *         pointer.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The function replaces the existing order of the frame set @em self by
 * sorting its contents according to the comparison function @em compare.
 *
 * By default, the order of a frame set, i.e. the order of any newly created
 * frame set object, is defined by the order in which frames are inserted into
 * the frame set. By calling this function, this order will be lost. If this
 * order has to be preserved, sorting has to be done on a copy of @em self.
 *
 * The function @em compare compares two frames and must return -1, 0, or
 * 1 if the first frame is considered to be less than, equal or larger than
 * the second frame, respectively.
 *
 * @see cpl_frame_compare_func
 */

cpl_error_code
cpl_frameset_sort(cpl_frameset *self, cpl_frame_compare_func compare)
{



    if ((self == NULL) || (compare == NULL)) {
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);
    }

    return _cpl_frameset_history_sort(self->history, self->frames, compare);

}


/**
 * @brief
 *   Separate a list of frames into groups, using a comparison function
 *
 * @param self       Input frame set
 * @param compare    Pointer to comparison function to use.
 * @param nb_labels  Number of different sets or undefined on error
 *
 * @return
 * array of labels defining the selection or NULL on error
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The parameter <i>self</i>, <i>compare</i> or <i>nb_labels</i>
 *         is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * This function takes a set of frames and groups the frames that are
 * 'identical' together. The user provided comparison function defines what
 * being identical means for two frames. A label (non-negative int) is associated
 * to each group of identical frames, these labels are returned in an array of
 * length equal to the size of the frameset.
 *
 * The comparison function should be commutative, must take two frames and
 * return 1 if they are identical, 0 if they are different, and -1 on error.
 *
 * The number of calls to the comparison functions is O(n*m), where n is the
 * number of frames in the set, and m is the number of different labels found
 * in the set. In the worst case m equals n, and the call requires n(n-1)/2
 * calls to the comparison function. If all identical frames appear together
 * in the list, the number of required calls is only n + O(m^2).
 *
 * The returned array must be deallocated with cpl_free().
 */

cpl_size *
cpl_frameset_labelise(const cpl_frameset *self,
                      int (*compare)(const cpl_frame*, const cpl_frame*),
                      cpl_size *nb_labels)
{

    cpl_size *labels;
    cpl_size *labelsinv;
    cpl_size nframes;
    cpl_size i = 0;
    cpl_size j = 0;
    cpl_size nlabels = 0;
    cpl_size ncomp = 0;
    const cpl_frame ** framelist;
    const cpl_frame * frame1;

    cpl_frameset_iterator *it = NULL;


    cpl_ensure(self      != NULL, CPL_ERROR_NULL_INPUT, NULL);
    cpl_ensure(compare   != NULL, CPL_ERROR_NULL_INPUT, NULL);
    cpl_ensure(nb_labels != NULL, CPL_ERROR_NULL_INPUT, NULL);

    nframes = cpl_frameset_get_size(self);

    cpl_ensure(nframes >= 1, CPL_ERROR_ILLEGAL_INPUT, NULL);

    labels    = cx_malloc(nframes * sizeof(cpl_size));
    labelsinv = cx_malloc(nframes * sizeof(cpl_size));
    framelist = cx_malloc(nframes * sizeof(const cpl_frame *));

    it = cpl_frameset_iterator_new(self);

    while ((frame1 = cpl_frameset_iterator_get(it)) != NULL)
    {

    	cpl_errorstate status;

        cpl_size jj;


        for (jj = 0; jj < nlabels; jj++, j = (j == nlabels - 1) ? 0 : j + 1) {

            const cpl_frame *frame2 = framelist[j];

            /*
             * Compare the frames i and j
             *  - frame i is first compared to the frame which matched the
             *    previous comparison. In this way only one comparison is
             *    needed for frames tagged as the previous one
             */

            const int comp = (*compare)(frame1, frame2);

            ncomp++;

            if (comp == 1) {
                /* Identical */
                break;
            } else if (comp != 0) {
                /* Error */

            	cpl_frameset_iterator_delete(it);

                cx_free(framelist);
                cx_free(labelsinv);
                cx_free(labels);

                /* Propagate error */
                (void)cpl_error_set_where_();
                return NULL;
            }
        }

        if (jj == nlabels) {
            /* Labelise the newly found type of frame */

            framelist[nlabels] = frame1;
            labelsinv[nlabels] = i;
            labels[i] = nlabels++;

        } else {
            /* Identical */
            labels[i] = labels[labelsinv[j]];
        }


    	status = cpl_errorstate_get();

    	cpl_frameset_iterator_advance(it, 1);

        if (cpl_error_get_code() == CPL_ERROR_ACCESS_OUT_OF_RANGE)
        {
            cpl_errorstate_set(status);
        }

    	++i;

    }

    cpl_frameset_iterator_delete(it);


    *nb_labels = nlabels;
    cx_free(framelist);
    cx_free(labelsinv);

    cpl_msg_debug(cpl_func, "%" CPL_SIZE_FORMAT " frames labelized with %"
                  CPL_SIZE_FORMAT " labels after %" CPL_SIZE_FORMAT
                  "comparisons", nframes, nlabels, ncomp);

    return labels;

}


/**
 * @brief
 *   Get a frame from a frame set.
 *
 * @param set       Input frame set.
 * @param position  The requested frame.
 *
 * @return
 *   The function returns a handle to the frame at position @em position in
 *   the set, or @c NULL in case an error occurs.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The parameter <i>self</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ILLEGAL_INPUT</td>
 *       <td class="ecr">
 *         The parameter <i>position</i> is out of range.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The function returns a handle to the frame at the index @em position in
 * the set. The frame position ranges from @c 0 to one less than the size of
 * the frame set.
 *
 * The returned frame is still owned by the frame set @em set, i.e. the
 * obtained frame must not be deleted through the returned handle and also
 * its tag must not be modified.
 *
 * As an alternative to using this function, the functions
 * cpl_frameset_get_first_const() and cpl_frameset_get_next_const() should be
 * considered, if performance is an issue.
 *
 * @see
 *   cpl_frameset_get_size(), cpl_frameset_get_first_const(),
 *   cpl_frameset_get_next_const()
 *
 * @deprecated
 *   This function will be removed from CPL version 7. Code using these
 *   functions should use cpl_frameset_get_position_const() instead!
 */

const cpl_frame *
cpl_frameset_get_frame_const(const cpl_frameset *set, cpl_size position)
{


    if (set == NULL) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return NULL;
    }

    if ((position < 0) || (position >= cpl_frameset_get_size(set))) {
        cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);
        return NULL;
    }

    return cpl_frameset_get_position_const(set, position);

}


/**
 * @brief
 *   Get a frame from a frame set.
 *
 * @param set       Input frame set.
 * @param position  The requested frame.
 *
 * @return
 *   The function returns a handle to the frame at position @em position in
 *   the set, or @c NULL in case an error occurs.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The parameter <i>self</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ILLEGAL_INPUT</td>
 *       <td class="ecr">
 *         The parameter <i>position</i> is out of range.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The function returns a handle to the frame at the index @em position in
 * the set. The frame position ranges from @c 0 to one less than the size of
 * the frame set.
 *
 * The returned frame is still owned by the frame set @em set, i.e. the
 * obtained frame must not be deleted through the returned handle and also
 * its tag must not be modified.
 *
 * As an alternative to using this function, the functions
 * cpl_frameset_get_first() and cpl_frameset_get_next() should be considered,
 * if performance is an issue.
 *
 * @see
 *   cpl_frameset_get_size(), cpl_frameset_get_first(),
 *   cpl_frameset_get_next()
 *
 * @deprecated
 *   This function will be removed from CPL version 7. Code using these
 *   functions should use cpl_frameset_get_position() instead!
 *
 */

cpl_frame *
cpl_frameset_get_frame(cpl_frameset *set, cpl_size position)
{


    if (set == NULL) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return NULL;
    }

    if ((position < 0) || (position >= cpl_frameset_get_size(set))) {
        cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);
        return NULL;
    }

    return cpl_frameset_get_position(set, position);

}


/**
 * @brief
 *   Get the frame at a given position in the frame set.
 *
 * @param self      The frame set
 * @param position  Frame position.
 *
 * @return
 *   The function returns the frame at the given position, or @c NULL
 *   if an error occurs.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The parameter <i>self</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ILLEGAL_INPUT</td>
 *       <td class="ecr">
 *         The parameter <i>position</i> is invalid, i.e. the given value is
 *         outside of its domain.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The function retrieves the frame stored in the frame set @em self at the
 * index position @em position. The index positions are counted from zero,
 * and reach up to one less than the number of frames in the frame set.
 */

cpl_frame *
cpl_frameset_get_position(cpl_frameset *self, cpl_size position)
{


    cx_list_iterator _position = NULL;

    cx_multimap_iterator frame = NULL;


    if (self == NULL) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return NULL;
    }

    if ((position < 0) || (position >= cpl_frameset_get_size(self))) {
        cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);
        return NULL;
    }


    // FIXME: Should use iterators for that!

    _position = cx_list_begin(self->history);

    while (position > 0) {
        _position = cx_list_next(self->history, _position);
        --position;
    }

    frame = cx_list_get(self->history, _position);
    cx_assert(frame != cx_multimap_end(self->frames));

    return cx_multimap_get_value(self->frames, frame);

}


/**
 * @brief
 *   Get the frame at a given iterator position.
 *
 * @param self      The iterator to dereference
 * @param position  Iterator offset from the beginning of the frame set.
 *
 * @return
 *   The function returns the frame at the iterator position, or @c NULL
 *   if an error occurs.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The parameter <i>self</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ILLEGAL_INPUT</td>
 *       <td class="ecr">
 *         The parameter <i>position</i> is invalid, i.e. the given value is
 *         outside of its domain.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The function retrieves the frame stored in the frame set @em self at the
 * index position @em position. The index positions are counted from zero,
 * and reach up to one less than the number of frames in the frame set.
 */

const cpl_frame *
cpl_frameset_get_position_const(const cpl_frameset *self, cpl_size position)
{


    cx_list_iterator _position = NULL;

    cx_multimap_iterator frame = NULL;


    if (self == NULL) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return NULL;
    }

    if ((position < 0) || (position >= cpl_frameset_get_size(self))) {
        cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);
        return NULL;
    }


    // FIXME: Should use iterators for that!

    _position = cx_list_begin(self->history);

    while (position > 0) {
        _position = cx_list_next(self->history, _position);
        --position;
    }

    frame = cx_list_get(self->history, _position);
    cx_assert(frame != cx_multimap_end(self->frames));

    return cx_multimap_get_value(self->frames, frame);

}


/**
 * @brief
 *   Extract a subset of frames from a set of frames
 *
 * @param self           Input frame set
 * @param labels         The array of labels associated to each input frame
 * @param desired_label  The label identifying the requested frames
 *
 * @note  The array of labels must have (at least) the length of the frame set
 *
 * @return
 *  A pointer to a newly allocated frame set or NULL on error
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The parameter <i>self</i> or <i>labels</i> is a <tt>NULL</tt>
 *         pointer.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The returned object must be deallocated with cpl_frameset_delete()
 */

cpl_frameset *
cpl_frameset_extract(const cpl_frameset *self, const cpl_size *labels,
                     cpl_size desired_label)
{

    cpl_frameset    *selected = NULL;
    const cpl_frame *frame;
    cpl_size i = 0;


    cpl_ensure(self   != NULL, CPL_ERROR_NULL_INPUT, NULL);
    cpl_ensure(labels != NULL, CPL_ERROR_NULL_INPUT, NULL);

    cpl_frameset_iterator *it = cpl_frameset_iterator_new(self);

    while ((frame = cpl_frameset_iterator_get_const(it))) {

        if (labels[i] == desired_label) {

            /*
             * Duplicate frame and insert it in the selected object
             */

            cpl_frame *_frame = cpl_frame_duplicate(frame);

            if (selected == NULL) {
                selected = cpl_frameset_new();
            }

            cpl_frameset_insert(selected, _frame);

        }


        cpl_errorstate status = cpl_errorstate_get();

        cpl_frameset_iterator_advance(it, 1);

        if (cpl_error_get_code() == CPL_ERROR_ACCESS_OUT_OF_RANGE)
        {
            cpl_errorstate_set(status);
        }

        ++i;

    }

    cpl_frameset_iterator_delete(it);

    return selected;

}

/**
 * @brief
 *   Dump the frameset debugging information to the given stream.
 *
 * @param self     The frameset.
 * @param stream   The output stream to use.
 *
 * @return Nothing.
 *
 * The function dumps the contents of the frameset @em self to the
 * output stream @em stream. If @em stream is @c NULL the function writes
 * to the standard output. If @em self is @c NULL or the frameset is
 * empty, the function does nothing.
 */

void
cpl_frameset_dump(const cpl_frameset *self, FILE *stream)
{
    int i = 0;
    int n = 0;
    const cpl_frame *frame;

    if (self == NULL)
        return;

    if (stream == NULL)
        stream = stdout;

    n = cpl_frameset_get_size(self);

    if (n == 0)
        return;

    fprintf (stream, "+++ Frameset at address: %p\n", (const void *)self);

    cpl_frameset_iterator *it = cpl_frameset_iterator_new(self);

    while ((frame = cpl_frameset_iterator_get_const(it))) {

        fprintf(stream, "%3d  ", i++);
        cpl_frame_dump(frame, stream);

        cpl_errorstate status = cpl_errorstate_get();

        cpl_frameset_iterator_advance(it, 1);

        if (cpl_error_get_code() == CPL_ERROR_ACCESS_OUT_OF_RANGE)
        {
            cpl_errorstate_set(status);
        }

    }

    cpl_frameset_iterator_delete(it);

    return;
}
/**@}*/


/**
 * @defgroup cpl_frameset_iterator Frame Set Iterators
 * @ingroup cpl_frameset
 *
 * @brief
 *   Iterator support for frame sets.
 *
 * Frame set iterators allow to access the contents of a frame set sequentially,
 * in an ordered manner. An iterator is created for a particular frame set,
 * and it stays bound to that frame set until it is destroyed. However multiple
 * iterators can be defined for the same frame set.
 *
 * By default, the order of frames in a frame set is the order in which the
 * individual frames have been inserted. However, a particular sorting order
 * can be defined by sorting the frame set using a custom comparison function
 * for frames.
 *
 * Frame set iterators are supposed to be short-lived objects, and it is not
 * recommended to use them for keeping, or passing around references to the
 * frame set contents. In particular, changing the contents of the underlying
 * frame set by erasing, or inserting frames may invalidate all existing
 * iterators.
 */

/**@{*/

/*
 * Frame set iterator type
 */

struct _cpl_frameset_iterator_
{
    const cpl_frameset *parent;
    cx_list_iterator position;

    cxint direction;
};


/**
 * @brief
 *   Create a new frame set iterator.
 *
 * @param parent  The frame set for which the iterator is created.
 *
 * @return
 *   The newly allocated frame set iterator object, or @c NULL if an error
 *   occurred.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The parameter <i>parent</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The function creates a new iterator object bound to the frame set @em
 * parent. The iterator is initialized such that it points to the beginning
 * of @em parent.
 *
 * The beginning is defined by the current ordering defined for the frame set
 * @em parent.
 *
 * @see cpl_frameset_sort()
 */

cpl_frameset_iterator *
cpl_frameset_iterator_new(const cpl_frameset *parent)
{


    cpl_frameset_iterator *self = NULL;


    if (parent == NULL) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return NULL;
    }


    self = cx_malloc(sizeof *self);

    self->parent    = parent;
    self->position  = cx_list_begin(parent->history);
    self->direction = 0;

    return self;

}


/**
 * @brief
 *   Create a frame set iterator from an existing frame set iterator.
 *
 * @param other  The frame set iterator to clone.
 *
 * @return
 *   A copy of the frame set iterator @em other on success, or @c NULL
 *   otherwise.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The parameter <i>other</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The function creates a copy of the iterator object @em other. An iterator
 * copy constructed by this function is bound to the same frame set @em other
 * has been constructed for, and it points to the same frame that @em other
 * points to.
 */

cpl_frameset_iterator *
cpl_frameset_iterator_duplicate(const cpl_frameset_iterator *other)
{


    cpl_frameset_iterator *self = NULL;


    if (other == NULL) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return NULL;
    }

    self = cx_malloc(sizeof *self);

    self->parent    = other->parent;
    self->position  = other->position;
    self->direction = other->direction;

    return self;

}


/**
 * @brief
 *   Destroy a frame set iterator.
 *
 * @param self  The frame set to destroy.
 *
 * @return
 *   Nothing.
 *
 * The function destroys the frame set iterator object @em self. If @em self
 * is @c NULL, no operation is performed.
 */

void
cpl_frameset_iterator_delete(cpl_frameset_iterator *self)
{
    cx_free(self);
    return;
}


/**
 * @brief
 *   Assign a frame set iterator to another.
 *
 * @param self   The frame set iterator to assign to.
 * @param other  The frame set iterator to be assigned.
 *
 * @return
 *   The function returns @c CPL_ERROR_NONE on success, or an appropriate
 *   CPL error code otherwise.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The parameters <i>self</i> or <i>other</i> is a <tt>NULL</tt>
 *         pointer.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ILLEGAL_INPUT</td>
 *       <td class="ecr">
 *         The parameters <i>self</i> and <i>other</i> are not bound to the
 *         same frame set.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The function assigns the iterator @em other to the iterator @em self.
 * Only iterators which are bound to the same frame set can be assigned to
 * each other!
 */

cpl_error_code
cpl_frameset_iterator_assign(cpl_frameset_iterator *self,
                             const cpl_frameset_iterator *other)
{



    if ((self == NULL) || (other == NULL)) {
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);
    }

    if (self->parent != other->parent) {
        return cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);
    }


    self->position  = other->position;
    self->direction = other->direction;

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Reset a frame set iterator to the beginning of a frame set.
 *
 * @param self  The iterator to reposition.
 *
 * @return
 *   Nothing.
 *
 * The function moves the frame set iterator @em self back to the beginning
 * of its underlying frame set. The first frame in the frame set is defined by
 * the established sorting order.
 *
 * @see cpl_frameset_sort()
 */

void
cpl_frameset_iterator_reset(cpl_frameset_iterator *self)
{

    if (self != NULL) {
        self->position  = cx_list_begin(self->parent->history);
        self->direction = 0;
    }

    return;

}


/**
 * @brief
 *   Advance an iterator by a number of elements.
 *
 * @param self      The frame set iterator to reposition.
 * @param distance  The of number of elements by which the iterator is moved.
 *
 * @return
 *   The function returns @c CPL_ERROR_NONE on success, or an appropriate
 *   CPL error code otherwise.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The parameter <i>self</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ACCESS_OUT_OF_RANGE</td>
 *       <td class="ecr">
 *         The given iterator offset would move the iterator beyond the
 *         beginning or the end of the frame set.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The functions moves the iterator by @em distance number of elements, with
 * respect to its current position. The number of elements @em distance may
 * be negative or positive, and the iterator position will move backward and
 * forward respectively.
 *
 * It is an error if the given @em distance would move the iterator either
 * past the one-before-the-beginning position or the one-past-the-end position
 * of the underlying frame set. In this case the iterator is not repositioned
 * and an out of range error is returned.
 */

cpl_error_code
cpl_frameset_iterator_advance(cpl_frameset_iterator *self, int distance)
{


    if (self == NULL) {
        return cpl_error_set_(CPL_ERROR_NULL_INPUT);
    }
    else {

        cxint _distance = distance;

        cx_list_iterator position = self->position;


        if (_distance < 0) {

            if (self->direction == -1) {
                position = cx_list_previous(self->parent->history, position);
                ++_distance;
            }

            while ((_distance != 0) &&
                   (position != cx_list_end(self->parent->history))) {
                position = cx_list_previous(self->parent->history, position);
                ++_distance;
            }

        }
        else {

            if (self->direction == 1) {
                position = cx_list_begin(self->parent->history);
                --_distance;
            }

            while ((_distance != 0) &&
                   (position != cx_list_end(self->parent->history))) {
                position = cx_list_next(self->parent->history, position);
                --_distance;
            }

        }

        if (_distance != 0) {

            return cpl_error_set_(CPL_ERROR_ACCESS_OUT_OF_RANGE);

        }
        else {

            if (position == cx_list_end(self->parent->history)) {
                self->direction = (distance < 0) ? 1 : -1;
            }
            else {
                self->direction = 0;
            }

            self->position = position;
        }

    }

    return CPL_ERROR_NONE;

}


/**
 * @brief
 *   Calculate the distance between two iterators
 *
 * @param self   First frame set iterator.
 * @param other  Second frame set iterator.
 *
 * @return
 *   The function returns the distance between the two input iterators. If an
 *   error occurs, the function returns a distance of 0 and sets an
 *   appropriate error code.
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The parameter <i>self</i> or <i>other</i> is a <tt>NULL</tt>
 *          pointer.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_ILLEGAL_INPUT</td>
 *       <td class="ecr">
 *         The two iterators iterators are not bound to the same frame set.
 *       </td>
 *     </tr>
 *     <tr>
 *       <td class="ecl">CPL_ERROR_DATA_NOT_FOUND</td>
 *       <td class="ecr">
 *         At least one input iterator is invalid.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The function calculates the distance between the iterators @em self and
 * @em other.
 *
 * To properly detect whether this function has failed or not it is
 * recommended to reset the errors before it is called, since the returned
 * value is not a distinctive feature.
 */

int
cpl_frameset_iterator_distance(const cpl_frameset_iterator *self,
                               const cpl_frameset_iterator *other)
{


    int invalid = 0;


    if ((self == NULL) || (other == NULL)) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return invalid;
    }

    if (self->parent != other->parent) {
        cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);
        return invalid;
    }


    /*
     * Nothing to be done if the iterator positions match already ...
     */

    if (self->position != other->position) {

        /*
         * ... otherwise try forward direction first ...
         */

        int distance = 0;
        cx_list_iterator position = self->position;

        while (position != cx_list_end(self->parent->history)) {

            if (position == other->position) {
                return distance;
            }

            ++distance;
            position = cx_list_next(self->parent->history, position);

        }


        /*
         * ... and finally the backward direction
         */

        distance = 0;
        position = self->position;

        while (position != cx_list_end(self->parent->history)) {

            if (position == other->position) {
                return distance;
            }

            --distance;
            position = cx_list_previous(self->parent->history, position);

        }


        /*
         * Nothing was found in any direction. This should not happen!
         */

        cpl_error_set_(CPL_ERROR_DATA_NOT_FOUND);
        return invalid;

    }

    return 0;

}


/**
 * @brief
 *   Get the frame from the frame set at the current position of the iterator.
 *
 * @param self  The frame set iterator to dereference.
 *
 * @return
 *   The frame stored in the frame set at the position of the iterator, or
 *   @c NULL if an error occurs. The function also returns @c NULL if the
 *   iterator is positioned at the sentinel element (i.e. the
 *   one-before-the-beginning or one-past-the-end position).
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The parameter <i>self</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The function dereferences the iterator @em self, i.e. it retrieves the frame
 * at the position pointed to by @em self.
 */

cpl_frame *
cpl_frameset_iterator_get(cpl_frameset_iterator *self)
{


    cx_multimap_iterator position = NULL;


    if (self == NULL) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return NULL;
    }

    if (self->position == cx_list_end(self->parent->history)) {
        return NULL;
    }

    position = cx_list_get(self->parent->history, self->position);
    cx_assert(position != cx_multimap_end(self->parent->frames));

    return cx_multimap_get_value(self->parent->frames, position);

}


/**
 * @brief
 *   Get the frame from the frame set at the current position of the iterator.
 *
 * @param self  The frame set iterator to dereference.
 *
 * @return
 *   The frame stored in the frame set at the position of the iterator or
 *   @c NULL if an error occurs. The function also returns @c NULL if the
 *   iterator is positioned at the sentinel element (i.e. the
 *   one-before-the-beginning or one-past-the-end position).
 *
 * @error
 *   <table class="ec" align="center">
 *     <tr>
 *       <td class="ecl">CPL_ERROR_NULL_INPUT</td>
 *       <td class="ecr">
 *         The parameter <i>self</i> is a <tt>NULL</tt> pointer.
 *       </td>
 *     </tr>
 *   </table>
 * @enderror
 *
 * The function dereferences the iterator @em self, i.e. it retrieves the
 * frame at the position pointed to by @em self.
 */

const cpl_frame *
cpl_frameset_iterator_get_const(const cpl_frameset_iterator *self)
{


    cx_multimap_iterator position = NULL;


    if (self == NULL) {
        cpl_error_set_(CPL_ERROR_NULL_INPUT);
        return NULL;
    }

    if (self->position == cx_list_end(self->parent->history)) {
        return NULL;
    }

    position = cx_list_get(self->parent->history, self->position);
    cx_assert(position != cx_multimap_end(self->parent->frames));

    return cx_multimap_get_value(self->parent->frames, position);

}
/**@}*/
