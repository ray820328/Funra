/*
 * This file is part of the ESO C Extension Library
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

#include <stdlib.h>
#include <string.h>

#include "cxmemory.h"
#include "cxmessages.h"
#include "cxdeque.h"


/**
 * @defgroup cxdeque Double-ended queue.
 *
 * The module implements a double-ended queue. This is a linear list
 * which is optimized for insertions and deletions that are made at the
 * ends of the list.
 *
 * @par Synopsis:
 * @code
 *   #include <cxdeque.h>
 * @endcode
 */

/**@{*/


/*
 * Double-ended queue structure and data types
 */

struct _cx_deque_
{
    cxptr *members;

    cxsize front;
    cxsize size;
    cxsize back;
};


inline static cxsize _cx_deque_capacity(const cx_deque *deque);



/*
 * Reserve space at the beginning of a deque. If there is still enough
 * space left, nothing is done.
 */

inline static void
_cx_deque_reserve_at_front(cx_deque *deque, cxsize size)
{

    if (deque->front < size) {

        cxptr *_members = NULL;


        deque->front = size;

        _members = cx_calloc(_cx_deque_capacity(deque), sizeof(cxptr));

        if (deque->size > 0) {

            cx_assert(deque->members != NULL);
            memcpy(&_members[deque->front], deque->members,
                   deque->size * sizeof(cxptr));

        }

        cx_free(deque->members);
        deque->members = _members;

    }

    return;

}


/*
 * Reserve space at the end of a deque.  If there is still enough
 * space left, nothing is done.
 */

inline static void
_cx_deque_reserve_at_back(cx_deque *deque, cxsize size)
{

    if (deque->back < size)
    {

        cxptr *_members = NULL;


        deque->back = size;

        _members = cx_calloc(_cx_deque_capacity(deque), sizeof(cxptr));

        if (deque->size > 0) {

            cx_assert(deque->members != NULL);
            memcpy(&_members[deque->front], &deque->members[deque->front],
                   deque->size * sizeof(cxptr));

        }

        cx_free(deque->members);
        deque->members = _members;

    }

    return;

}


/*
 * Shifts size number of deque members staring at position to the left
 * by n nodes.
 *
 * Note: No checks on the array sizes are done and no adjustment of
 *       the front, size or back are performed! The node at the position
 *       one before the start of the given range is overwritten!
 */

inline static void
_cx_deque_shift_left(cx_deque *deque, cxsize n, cxsize position, cxsize size)
{

#if 0

    cxsize i = 0;


    for (i = 0; i < size; ++i) {

        register cxsize current = deque->front + position + i;

        deque->members[current - n] = deque->members[current];

    }

    return;

#else

    if (n > 0) {

        register cxsize start = deque->front + position;

        memmove(&deque->members[start - n],
                &deque->members[start], size * sizeof(cxptr));

    }

    return;

#endif

}


/*
 * Shifts size number of deque members staring at position to the right
 * by n nodes.
 *
 * Note: No checks on the array sizes are done and no adjustment of
 *       the front, size or back are performed! The node at the position
 *       one after the end of the given range is overwritten!
 */

inline static void
_cx_deque_shift_right(cx_deque *deque, cxsize n, cxsize position, cxsize size)
{

#if 0

    cxsize i = 0;

    for (i = size; i > 0; --i) {

        register cxsize current = deque->front + position + i - 1;

        deque->members[current + n] = deque->members[current];

    }

    return;

#else

    if (n > 0) {

        register cxsize start = deque->front + position;

        memmove(&deque->members[start + n],
                &deque->members[start], size * sizeof(cxptr));

    }

    return;

#endif

}


/*
 * Balance the head and the tail buffer sections of a deque.
 */

inline static void
_cx_deque_balance(cx_deque *deque, cxsize limit)
{

    if ((deque->front != 0) && (deque->back != 0)) {

        if (deque->front > limit * deque->back) {

            cxsize difference = deque->front - deque->back;
            cxsize shift = difference / 2;

            _cx_deque_shift_left(deque, shift, deque->front, deque->size);

            deque->front -= shift;
            deque->back += difference - shift;

        }
        else {

            if (deque->back > limit * deque->front) {

                cxsize difference = deque->back - deque->front;
                cxsize shift = difference / 2;

                _cx_deque_shift_right(deque, shift, deque->front, deque->size);

                deque->back -= shift;
                deque->front += difference - shift;

            }

        }

    }

    return;

}


/*
 * Get the position of the first node in the map array.
 */

inline static cxsize
_cx_deque_first(const cx_deque *deque)
{
    return deque->front;
}


/*
 * Get the position of the last node in the map array.
 */

inline static cxsize
_cx_deque_last(const cx_deque *deque)
{
    return deque->front + deque->size - 1;
}


/*
 * Initialize a deque object
 */

inline static void
_cx_deque_init(cx_deque *deque, cxsize front, cxsize back)
{

    deque->members = NULL;

    deque->front = front;
    deque->size  = 0;
    deque->back  = back;

    if (front + back > 0) {
        deque->members = cx_calloc(_cx_deque_capacity(deque), sizeof(cxptr));
    }

    return;

}


/*
 * Remove all elements from a deque
 */

inline static void
_cx_deque_clear(cx_deque *deque)
{

    if (!deque)
        return;

    cx_free(deque->members);
    _cx_deque_init(deque, 0, 0);

    return;

}


/*
 * Get the current size of a deque.
 */

inline static cxsize
_cx_deque_size(const cx_deque *deque)
{
    return deque->size;
}


/*
 * Get the current capacity of a deque
 */

inline static cxsize
_cx_deque_capacity(const cx_deque *deque)
{
    return deque->front + deque->size + deque->back;
}


/*
 * Check whether a deque is empty
 */

inline static cxbool
_cx_deque_empty(const cx_deque *deque)
{
    return (_cx_deque_size(deque) == 0);
}


/*
 * Add a data element at the beginning
 */

inline static void
_cx_deque_push_front(cx_deque *deque, cxcptr data)
{

    --deque->front;
    ++deque->size;

    deque->members[deque->front] = (cxptr)data;

    return;

}


/*
 * Add a data element at the end
 */

inline static void
_cx_deque_push_back(cx_deque *deque, cxcptr data)
{

    deque->members[deque->front + deque->size] = (cxptr)data;

    --deque->back;
    ++deque->size;

    return;

}


/*
 * Assign data to a given position
 */

inline static void
_cx_deque_assign(cx_deque *deque, cxsize position, cxcptr data)
{

    deque->members[deque->front + position] = (cxptr)data;
    return;

}


/*
 * Get the data element at the given position from a deque object
 */

inline static cxptr
_cx_deque_get(const cx_deque *deque, cxsize position)
{
    return deque->members[deque->front + position];
}


/*
 * Set the data element at the given position
 */

inline static void
_cx_deque_put(cx_deque *deque, cxsize position, cxptr data)
{
    deque->members[deque->front + position] = data;
    return;
}


/*
 * Get an iterator to the beginning of the deque
 */

inline static cx_deque_iterator
_cx_deque_begin(const cx_deque *deque)
{
    (void) deque;   /* To avoid a compiler warning */
    return 0;
}


/*
 * Get an iterator to the end of the deque
 */

inline static cx_deque_iterator
_cx_deque_end(const cx_deque *deque)
{
    return _cx_deque_size(deque);
}


/*
 * Returns the position of the next element of a deque, i.e. the element
 * immediately following the current position
 */

inline static cx_deque_iterator
_cx_deque_next(const cx_deque *deque, cx_deque_const_iterator position)
{

    if (_cx_deque_empty(deque) || (position == _cx_deque_end(deque))) {
        return _cx_deque_end(deque);
    }

    return ++position;

}


/*
 * Returns the position of the previous element of a deque, i.e. the element
 * immediately preceding the current position
 */

inline static cx_deque_iterator
_cx_deque_previous(const cx_deque *deque, cx_deque_const_iterator position)
{

    if (_cx_deque_empty(deque) || (position == _cx_deque_begin(deque))) {
        return _cx_deque_end(deque);
    }

    return --position;

}


/*
 * Check whether the given queue is sorted using the provided comparison
 * function. Returns 1 if the deque is sorted, and 0 otherwise.
 */

inline static cxbool
_cx_deque_sorted(const cx_deque *deque, cx_compare_func compare)
{

    cxsize i = 0;

    for (i = 0; i < deque->size - 1; ++i) {

        register cxsize current = deque->front + i;

        if (compare(deque->members[current], deque->members[current + 1]) > 0) {
            return 0;
        }

    }

    return 1;

}


/*
 * Remove the deque node at the given position from the deque
 */

inline static cxptr
_cx_deque_extract(cx_deque *deque, cxsize position)
{

    cxptr data = _cx_deque_get(deque, position);

    _cx_deque_shift_left(deque, 1, position + 1,
                         _cx_deque_end(deque) - position - 1);

    --deque->size;
    ++deque->back;

    return data;

}


/*
 * Move a range of deque nodes in front of position
 */

inline static void
_cx_deque_transfer(cx_deque *deque, cxsize position, cx_deque *other,
                   cxsize first, cxsize last)
{

    if (deque == other) {

        if (position != last) {

            cxsize n_members = last - first;


            /*
             * Reserve space in the target
             */

            _cx_deque_reserve_at_back(deque, n_members);

            if (position != _cx_deque_end(deque)) {

                register cxsize n = _cx_deque_size(deque) - position;

                _cx_deque_shift_right(deque, n_members, position, n);


                /*
                 * Adjust source range start and end position
                 */

                if (first > position) {
                    first += n_members;
                }

            }


            /*
             * Transfer [first, last) to the target. No adjustment of the
             * size is needed, since the number of nodes is conserved.
             */

            memcpy(&deque->members[position], &other->members[first],
                   n_members * sizeof(cxptr));


            /*
             * Remove [first, last) from its old position. Again, there is
             * no change in the size of the source deque.
             */

            if (last != _cx_deque_end(other)) {

                register cxsize n = _cx_deque_size(other) - last;

                if (last > position) {
                    last += n_members;
                }
                else {
                    n += n_members;
                }

                _cx_deque_shift_left(other, n_members, last, n);

            }

        }

    }
    else {

        cxsize n_members = last - first;


        /*
         * Reserve space in the target
         */

        _cx_deque_reserve_at_back(deque, n_members);

        if (position != _cx_deque_end(deque)) {

            register cxsize n = _cx_deque_size(deque) - position;

            _cx_deque_shift_right(deque, n_members, position, n);

        }


        /*
         * Transfer [first, last) to the target
         */

        memcpy(&deque->members[position], &other->members[first],
               n_members * sizeof(cxptr));

        deque->size += n_members;
        deque->back -= n_members;


        /*
         * Remove [first, last) from its old position.
         */

        if (last != _cx_deque_end(other)) {

            register cxsize n = _cx_deque_size(other) - last;

            _cx_deque_shift_left(other, n_members, last, n);

        }

        other->size -= n_members;
        other->back += n_members;

    }

    return;

}


/*
 * Merge two sorted deques keeping the sorting order with respect to the
 * given comparison function.
 */

inline static void
_cx_deque_merge(cx_deque *deque1, cx_deque *deque2, cx_compare_func compare)
{

    if (deque1 != deque2) {

        cx_deque_iterator first1 = _cx_deque_begin(deque1);
        cx_deque_iterator first2 = _cx_deque_begin(deque2);

        cx_assert(_cx_deque_size(deque1) + _cx_deque_size(deque2) >=
                  _cx_deque_size(deque1));

        cx_assert(_cx_deque_sorted(deque1, compare));
        cx_assert(_cx_deque_sorted(deque2, compare));

        while ((first1 != _cx_deque_end(deque1)) &&
               (first2 != _cx_deque_end(deque2))) {

            if (compare(_cx_deque_get(deque2, first2),
                        _cx_deque_get(deque1, first1)) < 0) {

                _cx_deque_transfer(deque1, first1, deque2, first2, first2 + 1);
                first1 = _cx_deque_next(deque2, first1);
            }
            else {
                first1 = _cx_deque_next(deque1, first1);
            }

        }

        if (first2 != _cx_deque_end(deque2)) {
            _cx_deque_transfer(deque1, _cx_deque_end(deque1), deque2, first2,
                               _cx_deque_end(deque2));
        }

    }

    return;

}


/*
 * Sort a deque using the given comparison function.
 */

inline static void
_cx_deque_sort(cx_deque *deque, cx_compare_func compare)
{

#if 0

    struct stable *ss;
    unsigned long i;

    current_compare = compare;

   /*
    * create temporary copy of array, including the
    * indices. note that the execution time is dominated
    * by the actual sorting
    */

    ss = cx_malloc(deque->size * sizeof(struct stable));
    for (i = 0; i < deque->size; ++i) {
        ss[i].indx = i;
        ss[i].member = deque->members[deque->front + i];
    }

    qsort(ss, deque->size, sizeof(struct stable), compare_stable);

    for (i = 0; i < deque->size; ++i) {
        deque->members[deque->front + i] = ss[i].member;
    }

    cx_free(ss);

#else

    if (_cx_deque_size(deque) > 1 && !_cx_deque_sorted(deque, compare)) {

        cx_deque tmp;

        cxsize middle = _cx_deque_size(deque) / 2;

        _cx_deque_init(&tmp, 0, 0);

        _cx_deque_transfer(&tmp, _cx_deque_end(&tmp),
                           deque, middle, _cx_deque_end(deque));
        _cx_deque_sort(deque, compare);
        _cx_deque_sort(&tmp, compare);

        _cx_deque_merge(deque, &tmp, compare);
        _cx_deque_clear(&tmp);

    }

#endif

    return;

}


/**
 * @brief
 *   Create a new deque without any elements.
 *
 * @return Handle to the newly allocated deque.
 *
 * The function allocates memory for a deque object and initializes
 * it to an empty deque.
 */

cx_deque *
cx_deque_new(void)
{

    cx_deque *deque = cx_malloc(sizeof *deque);

    _cx_deque_init(deque, 0, 0);
    return deque;

}


/**
 * @brief
 *   Destroy a deque.
 *
 * @param deque  The deque to delete.
 *
 * @return Nothing.
 *
 * The function deallocates the deque object, but not the data objects
 * currently stored in the deque.
 */

void
cx_deque_delete(cx_deque *deque)
{

    _cx_deque_clear(deque);
    cx_assert(cx_deque_empty(deque));

    cx_free(deque);

    return;

}


/**
 * @brief
 *   Destroy a deque and all its elements.
 *
 * @param deque       Deque container to destroy.
 * @param deallocate  Data deallocator.
 *
 * @return Nothing.
 *
 * The function deallocates all data objects referenced by @em deque using
 * the data deallocation function @em deallocate and finally deallocates
 * the deque object itself.
 */

void
cx_deque_destroy(cx_deque *deque, cx_free_func deallocate)
{

    if (deque != NULL) {

        if (deallocate != NULL) {

            cxsize i = 0;

            for (i = 0; i < deque->size; ++i) {
                deallocate(_cx_deque_get(deque, i));
            }

        }

        cx_free(deque->members);
        cx_free(deque);

    }

    return;

}


/**
 * @brief
 *   Get the actual number of deque elements.
 *
 * @param deque  A deque.
 *
 * @return The current number of elements the deque contains, or 0 if the
 *   deque is empty.
 *
 * Retrieves the number of elements currently stored in the deque @em deque.
 */

cxsize
cx_deque_size(const cx_deque *deque)
{

    cx_assert(deque != NULL);
    return _cx_deque_size(deque);

}


/**
 * @brief
 *   Check whether a deque is empty.
 *
 * @param deque  A deque.
 *
 * @return The function returns @c TRUE if the deque is empty, and @c FALSE
 *   otherwise.
 *
 * The function tests if the deque @em deque contains data.
 */

cxbool
cx_deque_empty(const cx_deque *deque)
{

    cx_assert(deque != NULL);
    return _cx_deque_empty(deque);

}


/**
 * @brief
 *   Get the maximum number of deque elements possible.
 *
 * @param deque  A deque.
 *
 * @return The maximum number of elements that can be stored in the deque.
 *
 * Retrieves the deques capacity, i.e. the maximum possible number of data
 * items a deque can hold.
 */

cxsize
cx_deque_max_size(const cx_deque *deque)
{

    (void) deque;  /* Prevent warnings if cx_assert is disabled. */
    cx_assert(deque != NULL);
    return (cxsize)(-1);

}


/**
 * @brief
 *   Swap the data of two deques.
 *
 * @param deque  The first deque.
 * @param other  The second deque.
 *
 * @return Nothing.
 *
 * The contents of the deque @em other will be moved to the deque @em deque,
 * while the contents of @em deque is moved to @em other.
 */

void
cx_deque_swap(cx_deque *deque, cx_deque *other)
{

    cx_deque tmp = {NULL, 0, 0, 0};

    cx_assert(deque != NULL);
    cx_assert(other != NULL);

    tmp.front = other->front;
    tmp.back = other->back;
    tmp.size = other->size;
    tmp.members = other->members;

    other->front = deque->front;
    other->back = deque->back;
    other->size = deque->size;
    other->members = deque->members;

    deque->front = tmp.front;
    deque->back = tmp.back;
    deque->size = tmp.size;
    deque->members = tmp.members;

    return;

}


/**
 * @brief
 *   Assign data to a deque element.
 *
 * @param deque     A deque.
 * @param position  Position of the deque element where the data will be stored.
 * @param data      Data to store.
 *
 * @return Handle to the previously stored data object.
 *
 * The function assigns the data object reference @em data
 * to the iterator position @em position of the deque @em deque.
 */

cxptr
cx_deque_assign(cx_deque *deque, cx_deque_iterator position, cxptr data)
{

    cxptr tmp = NULL;

    cx_assert(deque != NULL);
    cx_assert((position >= _cx_deque_begin(deque)) &&
              (position < _cx_deque_end(deque)));

    tmp = _cx_deque_get(deque, position);
    _cx_deque_put(deque, position, data);

    return tmp;

}


/**
 * @brief
 *   Get the first element of a deque.
 *
 * @param deque  The deque to query.
 *
 * @return Handle to the data object stored in the first deque element.
 *
 * The function returns a reference to the first data item in the deque
 * @em deque.
 */

cxptr
cx_deque_front(const cx_deque *deque)
{

    cx_assert(deque != NULL);
    cx_assert(!_cx_deque_empty(deque));

    return _cx_deque_get(deque, _cx_deque_begin(deque));

}


/**
 * @brief
 *   Get the last element of a deque.
 *
 * @param deque  The deque to query.
 *
 * @return Handle to the data object stored in the last deque element.
 *
 * The function returns a reference to the last data item in the deque
 * @em deque.
 */

cxptr
cx_deque_back(const cx_deque *deque)
{
    cx_assert(deque != NULL);
    cx_assert(!_cx_deque_empty(deque));

    return _cx_deque_get(deque,
                         _cx_deque_previous(deque, _cx_deque_end(deque)));
}


/**
 * @brief
 *   Retrieve an element from a deque.
 *
 * @param deque     The deque to query.
 * @param position  The position of the element to get.
 *
 * @return A handle to the data object.
 *
 * The function returns a reference to the data item stored in the deque
 * @em deque at the iterator position @em position.
 *
 */

cxptr
cx_deque_get(const cx_deque *deque, cx_deque_const_iterator position)
{

    cx_assert(deque != NULL);
    cx_assert((position >= _cx_deque_begin(deque)) &&
              (position < _cx_deque_end(deque)));

    return _cx_deque_get(deque, position);

}


/**
 * @brief
 *   Get an iterator for the first deque element.
 *
 * @param deque  A deque.
 *
 * @return Iterator for the first element in the deque or @b cx_deque_end()
 *   if the deque is empty.
 *
 * The function returns a handle to the first element of @em deque. The
 * handle cannot be used directly to access the element data, but only
 * through the appropriate functions.
 */

cx_deque_iterator
cx_deque_begin(const cx_deque *deque)
{

    cx_assert(deque != NULL);
    return _cx_deque_begin(deque);

}


/**
 * @brief
 *   Get an iterator for the position after the last deque element.
 *
 * @param deque  A deque.
 *
 * @return Iterator for the end of the deque.
 *
 * The function returns an iterator for the position one past the last
 * element of the deque @em deque. The handle cannot be used to directly
 * access the element data, but only through the appropriate functions.
 */

cx_deque_iterator
cx_deque_end(const cx_deque *deque)
{

    cx_assert(deque != NULL);
    return _cx_deque_end(deque);

}


/**
 * @brief
 *   Get an iterator for the next deque element.
 *
 * @param deque     A deque.
 * @param position  Current iterator position.
 *
 * @return Iterator for the next deque element.
 *
 * The function returns an iterator for the next element in the deque
 * @em deque with respect to the current iterator position @em position.
 * If the deque @em deque is empty or @em position points to the deque end
 * the function returns @b cx_deque_end().
 */

cx_deque_iterator
cx_deque_next(const cx_deque *deque, cx_deque_const_iterator position)
{

    cx_assert(deque != NULL);
    return _cx_deque_next(deque, position);

}


/**
 * @brief
 *   Get an iterator for the previous deque element.
 *
 * @param deque     A deque.
 * @param position  Current iterator position.
 *
 * @return Iterator for the previous deque element.
 *
 * The function returns an iterator for the previous element in the deque
 * @em deque with respect to the current iterator position @em position.
 * If the deque @em deque is empty or @em position points to the beginning
 * of the deque the function returns @b cx_deque_end().
 */

cx_deque_iterator
cx_deque_previous(const cx_deque *deque, cx_deque_const_iterator position)
{

    cx_assert(deque != NULL);
    return _cx_deque_previous(deque, position);

}


/**
 * @brief
 *   Insert data at the beginning of a deque.
 *
 * @param deque  The deque to update.
 * @param data   Data to add to the deque.
 *
 * @return Nothing.
 *
 * The data @em data is inserted into the deque @em deque before the first
 * element of the deque, so that it becomes the new deque head.
 *
 * It is equivalent to the statement
 * @code
 *   cx_deque_insert(deque, cx_deque_begin(deque), data);
 * @endcode
 */

void
cx_deque_push_front(cx_deque *deque, cxcptr data)
{

    cx_assert(deque != NULL);


    if (deque->front == 0) {
        _cx_deque_reserve_at_front(deque, _cx_deque_size(deque) + 1);
    }

    _cx_deque_push_front(deque, data);
    return;

}


/**
 * @brief
 *   Remove the first deque element.
 *
 * @param deque  The deque to update.
 *
 * @return Handle to the data object previously stored as the first
 *    deque element.
 *
 * The function removes the first element from the deque @em deque returning
 * a handle to the previously stored data.
 *
 * It is equivalent to the statement
 * @code
 *   cx_deque_extract(deque, cx_deque_begin(deque));
 * @endcode
 */

cxptr
cx_deque_pop_front(cx_deque *deque)
{

    cx_assert(deque != NULL);
    cx_assert(!_cx_deque_empty(deque));

    return _cx_deque_extract(deque, _cx_deque_begin(deque));

}


/**
 * @brief
 *   Append data at the end of a deque.
 *
 * @param deque  The deque to update.
 * @param data   Data to append.
 *
 * @return Nothing.
 *
 * The data @em data is inserted into the deque @em deque after the last
 * element, so that it becomes the new deque tail.
 *
 * It is equivalent to the statement
 * @code
 *   cx_deque_insert(deque, cx_deque_end(deque), data);
 * @endcode
 */

void
cx_deque_push_back(cx_deque *deque, cxcptr data)
{

    cx_assert(deque != NULL);


    /*
     * If back is 0, the maximum allocated memory has been
     * reached. This means, it's necessary to allocate new
     * memory for inserting new members.
     */

    if (deque->back == 0) {
        _cx_deque_reserve_at_back(deque, _cx_deque_size(deque) + 1);
    }

    _cx_deque_push_back(deque, data);
    return;

}


/**
 * @brief
 *   Remove the last deque element.
 *
 * @param deque  The deque to update.
 *
 * @return Handle to the data object previously stored as the last
 *    deque element.
 *
 * The function removes the last element from the deque @em deque returning
 * a handle to the previously stored data.
 *
 * It is equivalent to the statement
 * @code
 *   cx_deque_extract(deque, cx_deque_previous(deque, cx_deque_end(deque)));
 * @endcode
 */

cxptr
cx_deque_pop_back(cx_deque *deque)
{

    cx_deque_iterator position;

    cx_assert(deque != NULL);
    cx_assert(!_cx_deque_empty(deque));

    position = _cx_deque_previous(deque, _cx_deque_end(deque));

    return _cx_deque_extract(deque, position);

}


/**
 * @brief
 *   Insert data into a deque at a given iterator position.
 *
 * @param deque     The deque to update.
 * @param position  List iterator position.
 * @param data      Data item to insert.
 *
 * @return Deque iterator position of the inserted data item.
 *
 * The function inserts the data object reference @em data into the deque
 * @em deque at the position given by the deque iterator @em position.
 */

cx_deque_iterator
cx_deque_insert(cx_deque *deque, cx_deque_iterator position, cxcptr data)
{

    cx_assert(deque != NULL);
    cx_assert(position <= _cx_deque_size(deque));

    if (position == _cx_deque_size(deque)) {
        cx_deque_push_back(deque, data);
    }
    else {

        cx_assert(position < _cx_deque_size(deque));
        cx_assert(_cx_deque_size(deque) > 1);

        cx_deque_push_back(deque, _cx_deque_get(deque,
                                                _cx_deque_size(deque) - 1));

        _cx_deque_shift_right(deque, 1, position,
                              _cx_deque_size(deque) - position - 1);
        _cx_deque_assign(deque, position, data);

    }

    return position;

}


/**
 * @brief
 *   Erase a deque element.
 *
 * @param deque       The deque to update.
 * @param position    Deque iterator position.
 * @param deallocate  Data deallocator.
 *
 * @return The iterator for the deque position after @em position.
 *
 * The function removes the data object stored at position @em position
 * from the deque @em deque. The data object itself is deallocated by
 * calling the data deallocator @em deallocate.
 */

cx_deque_iterator
cx_deque_erase(cx_deque *deque, cx_deque_iterator position,
               cx_free_func deallocate)
{

    cxsize next = position + 1;


    cx_assert(deque != NULL);
    cx_assert(deallocate != NULL);
    cx_assert((position >= _cx_deque_begin(deque)) &&
              (position < _cx_deque_size(deque)));

    deallocate(_cx_deque_get(deque, position));
    _cx_deque_shift_left(deque, 1, next, _cx_deque_size(deque) - next);

    --deque->size;
    ++deque->back;

    return position;

}


/**
 * @brief
 *   Remove all elements from a deque.
 *
 * @param deque  Deque to be cleared.
 *
 * @return Nothing.
 *
 * The deque @em deque is cleared, i.e. all elements are removed from the
 * deque. The removed data objects are left untouched, in particular they
 * are not deallocated. It is the responsibility of the caller to ensure
 * that there are still other references to the removed data objects.
 * After calling @b cx_deque_clear() the deque @em deque is empty.
 */

void
cx_deque_clear(cx_deque *deque)
{

    _cx_deque_clear(deque);
    return;

}


/**
 * @brief
 *   Extract a deque element.
 *
 * @param deque     A deque.
 * @param position  Deque iterator position.
 *
 * @return Handle to the previously stored data object.
 *
 * The function removes a data object from the deque @em deque located at the
 * iterator position @em position without destroying the data object.
 *
 * @see cx_deque_erase(), cx_deque_remove()
 */

cxptr
cx_deque_extract(cx_deque *deque, cx_deque_iterator position)
{

    cx_assert(deque != NULL);
    cx_assert((position >= _cx_deque_begin(deque)) &&
              (position < _cx_deque_end(deque)));

    return _cx_deque_extract(deque, position);

}


/**
 * @brief
 *   Remove all elements with a given value from a deque.
 *
 * @param deque  A deque.
 * @param data   Data to remove.
 *
 * @return Nothing.
 *
 * The value @em data is searched in the deque @em deque. If the data is
 * found it is removed from the deque. The data object itself is not
 * deallocated.
 */

void
cx_deque_remove(cx_deque *deque, cxcptr data)
{

    cx_deque_iterator first;


    cx_assert(deque != NULL);

    first = _cx_deque_begin(deque);

    while (first != _cx_deque_end(deque)) {

        if (_cx_deque_get(deque, first) == data) {
            _cx_deque_extract(deque, first);
        }
        else {
            first = _cx_deque_next(deque, first);
        }

    }

    return;

}


/**
 * @brief
 *   Remove duplicates of consecutive elements.
 *
 * @param deque    A deque.
 * @param compare  Function comparing the deque elements.
 *
 * @return Nothing.
 *
 * The function removes duplicates of consecutive deque elements, i.e. deque
 * elements with the same value, from the deque @em deque. The equality of
 * the deque elements is checked using the comparison function @em compare.
 * The comparison function @em compare must return an integer less than,
 * equal or greater than zero if the first argument passed to it is found,
 * respectively, to be less than, match, or be greater than the second
 * argument.
 */

void
cx_deque_unique(cx_deque *deque, cx_compare_func compare)
{

    cx_assert(deque != NULL);
    cx_assert(compare != NULL);

    if (!_cx_deque_empty(deque)) {

        cx_deque_iterator first = _cx_deque_begin(deque);
        cx_deque_iterator next = _cx_deque_next(deque, first);


        while (next != _cx_deque_end(deque)) {

            if (compare(_cx_deque_get(deque, first),
                        _cx_deque_get(deque, next)) == 0) {
                _cx_deque_extract(deque, next);
            }
            else {
                first = next;
                next = _cx_deque_next(deque, next);
            }

        }

    }

    return;

}


/**
 * @brief
 *   Move a range of elements in front of a given position.
 *
 * @param deque     Target deque.
 * @param position  Target iterator position.
 * @param other     Source deque.
 * @param first     Position of the first element to move.
 * @param last      Position of the last element to move.
 *
 * @return Nothing.
 *
 * The range of deque elements from the iterator position @em first to
 * @em last, but not including @em last, is moved from the source deque
 * @em other in front of the position @em position of the target deque
 * @em deque. Target and source deque may be identical, provided that the
 * target position @em position does not fall within the range of deque
 * elements to move.
 */

void
cx_deque_splice(cx_deque *deque, cx_deque_iterator position,
                     cx_deque *other, cx_deque_iterator first,
                     cx_deque_iterator last)
{

    cx_assert(other != NULL);
    cx_assert((first == _cx_deque_end(other)) ||
              ((first >= _cx_deque_begin(other)) &&
               (first < _cx_deque_end(other))));
    cx_assert((last == _cx_deque_end(other)) ||
              ((last > first) && (last < _cx_deque_end(other))));

    if (first != last) {
        cx_assert(deque != NULL);
        cx_assert((position == _cx_deque_end(deque)) ||
                  ((position >= _cx_deque_begin(deque)) &&
                   (position < _cx_deque_end(deque))));
        cx_assert((deque != other) ||
                  ((position < first) || (position > last)));

        _cx_deque_transfer(deque, position, other, first, last);
    }

    return;

}


/**
 * @brief
 *   Merge two sorted deques.
 *
 * @param deque    Target deque of the merge operation.
 * @param other    The deque to merge into the target.
 * @param compare  Function comparing the deque elements.
 *
 * @return Nothing.
 *
 * The function combines the two deques @em deque and @em other by moving all
 * elements from @em other into @em deque, so that all elements are still
 * sorted. The function requires that both input deques are already sorted.
 * The sorting order in which the elements of @em other are inserted
 * into @em deque is determined by the comparison function @em compare.
 * The comparison function @em compare must return an integer less than, equal
 * or greater than zero if the first argument passed to it is found,
 * respectively, to be less than, match, or be greater than the second
 * argument.
 *
 * The deque @em other is consumed by this process, i.e. after the successful
 * merging of the two deques, deque @em other will be empty.
 */

void
cx_deque_merge(cx_deque *deque, cx_deque *other, cx_compare_func compare)
{

    cx_assert(deque != NULL);
    cx_assert(other != NULL);
    cx_assert(compare != NULL);

    _cx_deque_merge(deque, other, compare);
    return;

}


/**
 * @brief
 *   Sort all elements of a deque using the given comparison function.
 *
 * @param deque    The deque to sort.
 * @param compare  Function comparing the deque elements.
 *
 * @return Nothing.
 *
 * The input deque @em deque is sorted using the comparison function
 * @em compare to determine the order of two deque elements. The comparison
 * function @em compare must return an integer less than, equal
 * or greater than zero if the first argument passed to it is found,
 * respectively, to be less than, equal, or be greater than the second
 * argument. This function uses the stdlib function qsort().
 */


void
cx_deque_sort(cx_deque *deque, cx_compare_func compare)
{

    cx_assert(deque != NULL);
    cx_assert(compare != NULL);

    _cx_deque_sort(deque, compare);
    return;

}


/**
 * @brief
 *   Reverse the order of all deque elements.
 *
 * @param deque  The deque to reverse.
 *
 * @return Nothing.
 *
 * The order of the elements of the deque @em deque is reversed.
 */

void
cx_deque_reverse(cx_deque *deque)
{

    cx_assert(deque != NULL);

    if (!_cx_deque_empty(deque)) {

        cx_deque_iterator current = _cx_deque_begin(deque);
        cx_deque_iterator middle = current + _cx_deque_size(deque) / 2;
        cx_deque_iterator last = _cx_deque_previous(deque,
                                                    _cx_deque_end(deque));

        while (current < middle) {

            cxptr data = _cx_deque_get(deque, last);


            _cx_deque_put(deque, last, _cx_deque_get(deque, current));
            _cx_deque_put(deque, current, data);

            current = _cx_deque_next(deque, current);
            last = _cx_deque_previous(deque, last);

        }

    }

    return;

}
/**@}*/

