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

#include "cxmemory.h"
#include "cxmessages.h"
#include "cxlist.h"



/**
 * @defgroup cxlist Doubly Linked Lists
 *
 * The module implements a doubly linked list object which can be traversed
 * in both directions, forward and backward, and methods to create, destroy
 * and manipulate it.
 *
 * @par Synopsis:
 * @code
 *   #include <cxlist.h>
 * @endcode
 */

/**@{*/

/*
 * Doubly linked list node and list opaque data types
 */

typedef struct _cx_lnode_ cx_lnode;

struct _cx_lnode_ {
    cxptr data;
    struct _cx_lnode_ *prev;
    struct _cx_lnode_ *next;
};

struct _cx_list_ {
    cx_lnode head;
};


/*
 * Attach data to a list node.
 */

inline static void
_cx_lnode_put(cx_lnode *node, cxcptr data)
{

    cx_assert(node != NULL);

    node->data = (cxptr)data;
    return;

}


/*
 * Retrieve data from a list node. The previously stored data is returned
 * to the caller.
 */

inline static cxptr
_cx_lnode_get(const cx_lnode *node)
{

    if (!node)
        return NULL;

    return node->data;

}


/*
 * Create a new list node.
 */

inline static cx_lnode *
_cx_lnode_create(cxcptr data)
{

    cx_lnode *node = cx_malloc(sizeof *node);

    _cx_lnode_put(node, data);
    node->next = NULL;
    node->prev = NULL;

    return node;

}


/*
 * Deallocate a list node. A handle to the node's data is returned to the
 * caller.
 */

inline static cxptr
_cx_lnode_delete(cx_lnode *node)
{

    cxptr data;

    if (!node)
        return NULL;

    data = _cx_lnode_get(node);
    cx_free(node);

    return data;

}


/*
 * Destroy a list node, i.e. at first the node's data is deallocated using
 * the passed deallocator and then the node itself. If NULL is passed as
 * deallocator the data is not deallocated and the function call is
 * equivalent to _cx_lnode_delete().
 */

inline static void
_cx_lnode_destroy(cx_lnode *node, cx_free_func dealloc)
{

    if (!node)
        return;

    if (dealloc && node->data)
        dealloc(node->data);
    cx_free(node);

    return;

}


/*
 * Check if the given list node follows another.
 */

inline static cxbool
_cx_lnode_follows(const cx_list *list, const cx_lnode *head,
                  const cx_lnode *node)
{
    cx_lnode *n = head->next;

    while (n != &list->head) {
        if (n == node)
            return TRUE;
        n = n->next;
    }

    return FALSE;
}


/*
 * Check if the given list node is in a list or not. Note that according
 * to this function the head node is not part of the list!
 */

inline static cxbool
_cx_lnode_exists(const cx_list *list, const cx_lnode *node)
{

    return _cx_lnode_follows(list, &list->head, node);

}


/*
 * Initialize a newly created list object
 */

inline static void
_cx_list_init(cx_list *list)
{

    if (!list)
        return;

    list->head.next = &list->head;
    list->head.prev = &list->head;

}


/*
 * Get an iterator to the beginning of a list.
 */

inline static cx_list_iterator
_cx_list_begin(const cx_list *list)
{

    return list->head.next;

}


/*
 * Get an iterator to the end of a list.
 */

inline static cx_list_iterator
_cx_list_end(const cx_list *list)
{

    return &(((cx_list *)list)->head);

}


/*
 * Calculate the distance between two list nodes.
 */

inline static cxsize
_cx_list_distance(const cx_lnode *first, const cx_lnode *last)
{

    register cxsize sz = 0;
    register cx_lnode *node;

    for (node = (cx_lnode *)first; node != last; node = node->next)
        ++sz;

    return sz;

}


/*
 * Extract a single node at the given position.
 */

inline static cx_lnode *
_cx_list_extract(cx_lnode *node)
{

    cx_lnode *prev, *next;


    prev = node->prev;
    next = node->next;
    prev->next = next;
    next->prev = prev;

    return node;

}


/*
 * Insert a single node at the given position.
 */

inline static cx_lnode *
_cx_list_insert(cx_lnode *position, cx_lnode *node)
{

    node->next = position;
    node->prev = position->prev;

    position->prev->next = node;
    position->prev = node;

    return node;

}


/*
 * Move a range of list nodes in front of position.
 */

inline static void
_cx_list_transfer(cx_lnode *position, cx_lnode *first, cx_lnode *last)
{

    if (position != last) {

        cx_lnode *tmp;

        /*
         * Remove [first, last) from its old position.
         */

        last->prev->next = position;
        first->prev->next = last;
        position->prev->next = first;

        /*
         * Splice [first, last) into its new position.
         */

        tmp = position->prev;
        position->prev = last->prev;
        last->prev = first->prev;
        first->prev = tmp;

    }

    return;

}


/*
 * Check if a given list is sorted using the provided comparison function.
 * Returns 1 if the list is sorted, 0 otherwise.
 */

inline static cxbool
_cx_list_sorted(cx_list *list, cx_compare_func compare)
{

    cx_lnode *node = list->head.next;
    cx_lnode *next = node != NULL ? node->next : NULL;


    while (next != &list->head) {
        if (compare(_cx_lnode_get(node), _cx_lnode_get(next)) > 0)
            return 0;

        node = next;
        next = next->next;
    }

    return 1;

}


/*
 * Get the next element of a list, i.e. the element directly following the
 * current position in the list.
 *
 * Note that there are no checks whether the current list position actually
 * exists in the given list.
 */

inline static cx_list_iterator
_cx_list_next(const cx_list *list, cx_list_const_iterator position)
{

    if (position == _cx_list_end(list))
        return _cx_list_end(list);

    return position->next;

}


/*
 * Get the previous element of a list, i.e. the element directly preceding
 * the current position in the list.
 *
 * Note that there are no checks whether the current list position actually
 * exists in the given list.
 */

inline static cx_list_iterator
_cx_list_previous(const cx_list *list, cx_list_const_iterator position)
{

    if (position == _cx_list_begin(list))
        return _cx_list_end(list);

    return position->prev;

}


/*
 * Remove all elements from a list
 */

inline static void
_cx_list_clear(cx_list *list)
{
    cx_list_iterator l;

    if (!list)
        return;

    l = _cx_list_begin(list);
    while (l != _cx_list_end(list)) {
        cx_lnode *node = l;

        l = _cx_list_next(list, l);
        _cx_lnode_delete(_cx_list_extract(node));
    }

    _cx_list_init(list);

    return;

}


/*
 * Check whether a list is empty.
 */

inline static cxbool
_cx_list_empty(const cx_list *list)
{

    return (list->head.next == &list->head);

}


/*
 * Compute the size of a list as the distance between the first and the
 * sentinel element.
 */

inline static cxsize
_cx_list_size(const cx_list *list)
{

    return _cx_list_distance(_cx_list_begin(list), _cx_list_end(list));

}


/*
 * Merge two sorted lists keeping the sorting order with respect to the
 * given comparison function.
 */

inline static void
_cx_list_merge(cx_list *list1, cx_list *list2, cx_compare_func compare)
{

    if (list1 != list2) {
        cx_lnode *first1, *last1;
        cx_lnode *first2, *last2;


        cx_assert(_cx_list_size(list1) + _cx_list_size(list2) >=
                  _cx_list_size(list1));

        cx_assert(_cx_list_sorted(list1, compare));
        cx_assert(_cx_list_sorted(list2, compare));

        first1 = _cx_list_begin(list1);
        last1 = _cx_list_end(list1);
        first2 = _cx_list_begin(list2);
        last2 = _cx_list_end(list2);

        while (first1 != last1 && first2 != last2)
            if (compare(_cx_lnode_get(first2), _cx_lnode_get(first1)) < 0) {
                cx_lnode *next = _cx_list_next(list2, first2);

                _cx_list_transfer(first1, first2, next);
                first2 = next;
            }
            else
                first1 = _cx_list_next(list1, first1);

        if (first2 != last2)
            _cx_list_transfer(last1, first2, last2);
    }

    return;

}


/*
 * Sort the elements of a list with respect to the given comparison function.
 */

inline static void
_cx_list_sort(cx_list *list, cx_compare_func compare)
{

    if (_cx_list_size(list) > 1 && !_cx_list_sorted(list, compare)) {
        cx_list tmp;
        cx_lnode *node;
        cxsize middle = _cx_list_size(list) / 2;


        _cx_list_init(&tmp);
        node = _cx_list_begin(list);

        while (middle--)
            node = _cx_list_next(list, node);

        _cx_list_transfer(_cx_list_begin(&tmp), node, _cx_list_end(list));
        _cx_list_sort(list, compare);
        _cx_list_sort(&tmp, compare);

        _cx_list_merge(list, &tmp, compare);
    }

    return;

}


/**
 * @brief
 *   Get an iterator for the first list element.
 *
 * @param list  A list.
 *
 * @return Iterator for the first element in the list or @b cx_list_end()
 *   if the list is empty.
 *
 * The function returns a handle to the first element of @em list. The
 * handle cannot be used directly to access the element data, but only
 * through the appropriate functions.
 */

cx_list_iterator
cx_list_begin(const cx_list *list)
{

    cx_assert(list != NULL);

    return _cx_list_begin(list);

}


/**
 * @brief
 *   Get an iterator for the position after the last list element.
 *
 * @param list  A list.
 *
 * @return Iterator for the end of the list.
 *
 * The function returns an iterator for the position one past the last
 * element of the list @em list. The handle cannot be used to directly
 * access the element data, but only through the appropriate functions.
 */

cx_list_iterator
cx_list_end(const cx_list *list)
{

    cx_assert(list != NULL);

    return _cx_list_end(list);

}


/**
 * @brief
 *   Get an iterator for the next list element.
 *
 * @param list      A list.
 * @param position  Current iterator position.
 *
 * @return Iterator for the next list element.
 *
 * The function returns an iterator for the next element in the list
 * @em list with respect to the current iterator position @em position.
 * If the list @em list is empty or @em position points to the list end
 * the function returns @b cx_list_end().
 */

cx_list_iterator
cx_list_next(const cx_list *list, cx_list_const_iterator position)
{

    cx_assert(list != NULL);
    cx_assert(position == _cx_list_end(list) ||
              _cx_lnode_exists(list, position));

    return _cx_list_next(list, position);

}


/**
 * @brief
 *   Get an iterator for the previous list element.
 *
 * @param list      A list.
 * @param position  Current iterator position.
 *
 * @return Iterator for the previous list element.
 *
 * The function returns an iterator for the previous element in the list
 * @em list with respect to the current iterator position @em position.
 * If the list @em list is empty or @em position points to the beginning
 * of the list the function returns @b cx_list_end().
 */

cx_list_iterator
cx_list_previous(const cx_list *list, cx_list_const_iterator position)
{

    cx_assert(list != NULL);
    cx_assert(position == _cx_list_end(list) ||
              _cx_lnode_exists(list, position));

    return _cx_list_previous(list, position);

}


/**
 * @brief
 *   Remove all elements from a list.
 *
 * @param list  List to be cleared.
 *
 * @return Nothing.
 *
 * The list @em list is cleared, i.e. all elements are removed from the list.
 * The removed data objects are left untouched, in particular they are not
 * deallocated. It is the responsibility of the caller to ensure that there
 * are still other references to the removed data objects. After calling
 * @b cx_list_clear() the list @em list is empty.
 */

void
cx_list_clear(cx_list *list)
{

    _cx_list_clear(list);
    return;

}


/**
 * @brief
 *   Check whether a list is empty.
 *
 * @param list  A list.
 *
 * @return The function returns @c TRUE if the list is empty, and @c FALSE
 *   otherwise.
 *
 * The function tests if the list @em list contains data. A call to this
 * function is equivalent to the statement:
 *
 * @code
 *   return (cx_list_size(list) == 0);
 * @endcode
 */

cxbool
cx_list_empty(const cx_list *list)
{

    cx_assert(list != NULL);

    return _cx_list_empty(list);

}


/**
 * @brief
 *   Create a new list without any elements.
 *
 * @return Handle to the newly allocated list.
 *
 * The function allocates memory for the list object and initializes
 * it to a empty list.
 */

cx_list *
cx_list_new(void)
{

    cx_list *list = cx_malloc(sizeof *list);

    _cx_list_init(list);
    return list;

}


/**
 * @brief
 *   Destroy a list.
 *
 * @param list  The list to delete.
 *
 * @return Nothing.
 *
 * The function deallocates the list object, but not the data objects
 * currently stored in the list.
 */

void
cx_list_delete(cx_list *list)
{

    _cx_list_clear(list);

    cx_assert(cx_list_empty(list));
    cx_free(list);

    return;

}


/**
 * @brief
 *   Destroy a list and all its elements.
 *
 * @param list        List container to destroy.
 * @param deallocate  Data deallocator.
 *
 * @return Nothing.
 *
 * The function deallocates all data objects referenced by the list using
 * the data deallocation function @em deallocate and finally deallocates
 * the list itself.
 */

void
cx_list_destroy(cx_list *list, cx_free_func deallocate)
{
    cx_list_iterator l;


    if (!list)
        return;

    cx_assert(deallocate != NULL);

    l = _cx_list_begin(list);
    while (l != _cx_list_end(list)) {
        cx_lnode *node = l;

        l = _cx_list_next(list, l);
        _cx_lnode_destroy(_cx_list_extract(node), deallocate);
    }

    cx_assert(cx_list_empty(list));
    cx_free(list);

    return;

}


/**
 * @brief
 *   Get the actual number of list elements.
 *
 * @param list  A list.
 *
 * @return The current number of elements the list contains, or 0 if the
 *   list is empty.
 *
 * Retrieves the number of elements currently stored in the list @em list.
 */

cxsize
cx_list_size(const cx_list *list)
{

    cx_assert(list != NULL);
    return _cx_list_size(list);

}


/**
 * @brief
 *   Get the maximum number of list elements possible.
 *
 * @param list  A list.
 *
 * @return The maximum number of elements that can be stored in the list.
 *
 * Retrieves the lists capacity, i.e. the maximum possible number of data
 * items a list can hold.
 */

cxsize
cx_list_max_size(const cx_list *list)
{

    (void) list;  /* Prevent warnings if cx_assert is disabled. */
    cx_assert(list != NULL);

    return (cxsize)(-1);

}


/**
 * @brief
 *   Swap the data of two lists.
 *
 * @param list1  First list.
 * @param list2  Second list.
 *
 * @return Nothing.
 *
 * The contents of the first list @em list1 will be moved to the second
 * list @em list2, while the contents of @em list2 is moved to @em list1.
 */

void
cx_list_swap(cx_list *list1, cx_list *list2)
{

    cx_lnode *tmp;


    tmp = list1->head.next;

    list1->head.next = list2->head.next;
    list1->head.next->prev = &list1->head;

    list2->head.next = tmp;
    list2->head.next->prev = &list2->head;


    tmp = list1->head.prev;

    list1->head.prev = list2->head.prev;
    list1->head.prev->next = &list1->head;

    list2->head.prev = tmp;
    list2->head.prev->next = &list2->head;

    return;

}


/**
 * @brief
 *   Assign data to a list element.
 *
 * @param list      A list.
 * @param position  List position where the data will be stored
 * @param data      Data to store.
 *
 * @return Handle to the previously stored data object.
 *
 * The function assigns the data object reference @em data
 * to the iterator position @em position of the list @em list.
 */

cxptr
cx_list_assign(cx_list *list, cx_list_iterator position, cxcptr data)
{

    cxptr tmp;


    (void) list;  /* Prevent warnings if cx_assert is disabled. */
    cx_assert(list != NULL);
    cx_assert(_cx_lnode_exists(list, position));

    tmp = _cx_lnode_get(position);
    _cx_lnode_put(position, data);

    return tmp;

}


/**
 * @brief
 *   Get the first element of a list.
 *
 * @param list  The list to query.
 *
 * @return Handle to the data object stored in the first list element.
 *
 * The function returns a reference to the first data item in the list
 * @em list.
 *
 * Calling this function with an empty list is an invalid operation, and
 * the result is undefined.
 */

cxptr
cx_list_front(const cx_list *list)
{

    cx_assert(list != NULL);
    cx_assert(!cx_list_empty(list));

    return _cx_lnode_get(_cx_list_begin(list));

}


/**
 * @brief
 *   Get the last element of a list.
 *
 * @param list  The list to query.
 *
 * @return Handle to the data object stored in the last list element.
 *
 * The function returns a reference to the last data item in the list
 * @em list.
 *
 * Calling this function with an empty list is an invalid operation, and
 * the result is undefined.
 *
 */

cxptr
cx_list_back(const cx_list *list)
{

    cx_assert(list != NULL);
    cx_assert(!cx_list_empty(list));

    return _cx_lnode_get(_cx_list_previous(list, _cx_list_end(list)));

}


/**
 * @brief
 *   Get the data at a given iterator position.
 *
 * @param list      A list.
 * @param position  List position the data is retrieved from.
 *
 * @return Handle to the data object.
 *
 * The function returns a reference to the data item stored in the list
 * @em list at the iterator position @em position.
 */

cxptr
cx_list_get(const cx_list *list, cx_list_const_iterator position)
{

    (void) list;  /* Prevent warnings if cx_assert is disabled. */
    cx_assert(list != NULL);
    cx_assert(_cx_lnode_exists(list, position));


    return _cx_lnode_get(position);

}


/**
 * @brief
 *   Insert data into a list at a given iterator position.
 *
 * @param list      The list to update.
 * @param position  List iterator position.
 * @param data      Data item to insert.
 *
 * @return List iterator position of the inserted data item.
 *
 * The function inserts the data object reference @em data into the list
 * @em list at the list position given by the list iterator @em position.
 */

cx_list_iterator
cx_list_insert(cx_list *list, cx_list_iterator position, cxcptr data)
{

    cx_lnode *node;

    (void) list;  /* Prevent warnings if cx_assert is disabled. */
    cx_assert(list != NULL);
    cx_assert(position == _cx_list_end(list) ||
              _cx_lnode_exists(list, position));
    cx_assert(_cx_list_size(list) + 1 > _cx_list_size(list));

    node = _cx_list_insert(position, _cx_lnode_create(data));

    cx_assert(_cx_list_size(list) <= cx_list_max_size(list));

    return node;

}


/**
 * @brief
 *   Insert data at the beginning of a list.
 *
 * @param list  The list to update.
 * @param data  Data to add to the list.
 *
 * @return Nothing.
 *
 * The data @em data is inserted into the list @em list before the first
 * element of the list, so that it becomes the new list head.
 *
 * It is equivalent to the statement
 * @code
 *   cx_list_insert(list, cx_list_begin(list), data);
 * @endcode
 */

void
cx_list_push_front(cx_list *list, cxcptr data)
{

    cx_assert(list != NULL);
    _cx_list_insert(_cx_list_begin(list), _cx_lnode_create(data));

    return;

}


/**
 * @brief
 *   Append data at the end of a list.
 *
 * @param list  The list to update.
 * @param data  Data to append.
 *
 * @return Nothing.
 *
 * The data @em data is inserted into the list @em list after the last
 * element, so that it becomes the new list tail.
 *
 * It is equivalent to the statement
 * @code
 *   cx_list_insert(list, cx_list_end(list), data);
 * @endcode
 */

void
cx_list_push_back(cx_list *list, cxcptr data)
{

    cx_assert(list != NULL);
    _cx_list_insert(_cx_list_end(list), _cx_lnode_create(data));

    return;

}


/**
 * @brief
 *   Erase a list element.
 *
 * @param list        The list to update.
 * @param position    List iterator position.
 * @param deallocate  Data deallocator.
 *
 * @return The iterator for the list position after @em position.
 *
 * The function removes the data object stored at position @em position
 * from the list @em list. The data object itself is deallocated by calling
 * the data deallocator @em deallocate.
 */

cx_list_iterator
cx_list_erase(cx_list *list, cx_list_iterator position,
              cx_free_func deallocate)
{

    cx_lnode *next;


    cx_assert(list != NULL);
    cx_assert(deallocate != NULL);
    cx_assert(_cx_lnode_exists(list, position));

    next = _cx_list_next(list, position);
    _cx_lnode_destroy(_cx_list_extract(position), deallocate);

    return next;

}


/**
 * @brief
 *   Extract a list element.
 *
 * @param list      A list.
 * @param position  List iterator position.
 *
 * @return Handle to the previously stored data object.
 *
 * The function removes a data object from the list @em list located at the
 * iterator position @em position without destroying the data object.
 *
 * @see cx_list_erase(), cx_list_remove()
 */

cxptr
cx_list_extract(cx_list *list, cx_list_iterator position)
{

    (void) list;  /* Prevent warnings if cx_assert is disabled. */
    cx_assert(list != NULL);
    cx_assert(_cx_lnode_exists(list, position));

    _cx_list_extract(position);

    return _cx_lnode_delete(position);

}


/**
 * @brief
 *   Remove the first list element.
 *
 * @param list  The list to update.
 *
 * @return Handle to the data object previously stored as the first
 *    list element.
 *
 * The function removes the first element from the list @em list returning
 * a handle to the previously stored data.
 *
 * It is equivalent to the statement
 * @code
 *   cx_list_extract(list, cx_list_begin(list));
 * @endcode
 *
 * Calling this function with an empty list is an invalid operation, and
 * the result is undefined.
 */

cxptr
cx_list_pop_front(cx_list *list)
{

    cx_assert(!cx_list_empty(list));

    return _cx_lnode_delete(_cx_list_extract(cx_list_begin(list)));

}


/**
 * @brief
 *   Remove the last element of a list.
 *
 * @param list  The list to update.
 *
 * @return Handle to the data object previously stored as the last
 *    list element.
 *
 * The function removes the last element from the list @em list returning
 * a handle to the previously stored data.
 *
 * It is equivalent to the statement
 * @code
 *   cx_list_extract(list, cx_list_previous(list, cx_list_end(list)));
 * @endcode
 *
 * Calling this function with an empty list is an invalid operation, and
 * the result is undefined.
 */

cxptr
cx_list_pop_back(cx_list *list)
{

    cx_lnode *position = NULL;


    cx_assert(!cx_list_empty(list));

    position = _cx_list_previous(list, cx_list_end(list));

    return _cx_lnode_delete(_cx_list_extract(position));

}


/**
 * @brief
 *   Remove all elements with a given value from a list.
 *
 * @param list  A list object.
 * @param data  Data to remove.
 *
 * @return Nothing.
 *
 * The value @em data is searched in the list @em list. If the data is
 * found it is removed from the list. The data object itself is not
 * deallocated.
 */

void
cx_list_remove(cx_list *list, cxcptr data)
{

    cx_lnode *first, *last;


    first = cx_list_begin(list);
    last = _cx_list_end(list);

    while (first != last) {
        cx_lnode *next = _cx_list_next(list, first);

        if (_cx_lnode_get(first) == data) {
            _cx_list_extract(first);
            _cx_lnode_delete(first);
        }

        first = next;
    }

    return;

}


/**
 * @brief
 *   Remove duplicates of consecutive elements.
 *
 * @param list     A list.
 * @param compare  Function comparing the list elements.
 *
 * @return Nothing.
 *
 * The function removes duplicates of consecutive list elements, i.e. list
 * elements with the same value, from the list @em list. The equality of
 * the list elements is checked using the comparison function @em compare.
 * The comparison function @em compare must return an integer less than,
 * equal or greater than zero if the first argument passed to it is found,
 * respectively, to be less than, match, or be greater than the second
 * argument.
 */

void
cx_list_unique(cx_list *list, cx_compare_func compare)
{

    cx_lnode *first, *last, *next;


    cx_assert(list != NULL);
    cx_assert(compare != NULL);

    if (cx_list_empty(list))
        return;

    first = _cx_list_begin(list);
    last = _cx_list_end(list);

    next = _cx_list_next(list, first);
    while (next != last) {
        if (compare(_cx_lnode_get(first), _cx_lnode_get(next)) == 0) {
            _cx_list_extract(next);
            _cx_lnode_delete(next);
        }
        else
            first = next;

        next = _cx_list_next(list, first);
    }

    return;

}


/**
 * @brief
 *   Move a range of list elements in front of a given position.
 *
 * @param tlist     Target list.
 * @param position  Target iterator position.
 * @param slist     Source list.
 * @param first     Position of the first element to move.
 * @param last      Position of the last element to move.
 *
 * @return Nothing.
 *
 * The range of list elements from the iterator position @em first to
 * @em last, but not including @em last, is moved from the source list
 * @em slist in front of the position @em position of the target list
 * @em tlist. Target and source list may be identical, provided that the
 * target position @em position does not fall within the range of list
 * elements to move.
 */

void
cx_list_splice(cx_list *tlist, cx_list_iterator position,
                      cx_list *slist, cx_list_iterator first,
                      cx_list_iterator last)
{

    (void)slist; (void)tlist;  /* Prevent warnings if cx_assert is disabled. */
    cx_assert(slist != NULL);
    cx_assert(first == _cx_list_end(slist) || _cx_lnode_exists(slist, first));
    cx_assert(last == _cx_list_end(slist) ||
              _cx_lnode_follows(slist, first, last));

    if (first != last) {
        cx_assert(tlist != NULL);
        cx_assert(position == _cx_list_end(tlist) ||
                  _cx_lnode_exists(tlist, position));
        cx_assert(slist != tlist ||
                  (_cx_lnode_follows(slist, position, first) ||
                   _cx_lnode_follows(slist, last, position)));

        _cx_list_transfer(position, first, last);
    }

    return;

}


/**
 * @brief
 *   Merge two sorted lists.
 *
 * @param list1    First list to merge.
 * @param list2    Second list to merge.
 * @param compare  Function comparing the list elements.
 *
 * @return Nothing.
 *
 * The function combines the two lists @em list1 and @em list2 by moving all
 * elements from @em list2 into @em list1, so that all elements are still
 * sorted. The function requires that both input lists are already sorted.
 * The sorting order in which the elements of @em list2 are inserted
 * into @em list1 is determined by the comparison function @em compare.
 * The comparison function @em compare must return an integer less than, equal
 * or greater than zero if the first argument passed to it is found,
 * respectively, to be less than, match, or be greater than the second
 * argument.
 *
 * The list @em list2 is consumed by this process, i.e. after the successful
 * merging of the two lists, list @em list2 will be empty.
 */

void
cx_list_merge(cx_list *list1, cx_list *list2, cx_compare_func compare)
{

    cx_assert(list1 != NULL);
    cx_assert(list2 != NULL);
    cx_assert(compare != NULL);

    _cx_list_merge(list1, list2, compare);
    return;

}


/**
 * @brief
 *   Sort all elements of a list using the given comparison function.
 *
 * @param list     The list to sort.
 * @param compare  Function comparing the list elements.
 *
 * @return Nothing.
 *
 * The input list @em list is sorted using the comparison function
 * @em compare to determine the order of two list elements. The comparison
 * function @em compare must return an integer less than, equal
 * or greater than zero if the first argument passed to it is found,
 * respectively, to be less than, match, or be greater than the second
 * argument.
 */

void
cx_list_sort(cx_list *list, cx_compare_func compare)
{

    cx_assert(list != NULL);
    cx_assert(compare != NULL);

    _cx_list_sort(list, compare);
    return;

}


/**
 * @brief
 *   Reverse the order of all list elements.
 *
 * @param list  The list to reverse.
 *
 * @return Nothing.
 *
 * The order of the elements of the list @em list is reversed.
 */

void
cx_list_reverse(cx_list *list)
{

    cx_assert(list != NULL);


    /*
     * Nothing to be done if the list has length 0 or 1
     */

    if (list->head.next != &list->head &&
        list->head.next->next != &list->head) {

        cx_lnode *first = _cx_list_begin(list);

        first = _cx_list_next(list, first);
        while (first != _cx_list_end(list)) {
            cx_lnode *old = first;

            first = _cx_list_next(list, first);
            _cx_list_transfer(_cx_list_begin(list), old, first);
        }
    }

    return;

}
/**@}*/
