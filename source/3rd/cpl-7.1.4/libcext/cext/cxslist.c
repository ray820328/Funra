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
#include "cxslist.h"



/**
 * @defgroup cxslist Singly Linked Lists
 *
 * The module implements a linked list object restricted to iterations in
 * just one direction and methods to create, destroy and manipulate it.
 *
 * @par Synopsis:
 * @code
 *   #include <cxslist.h>
 * @endcode
 */

/**@{*/

/*
 * Singly linked list node and list opaque data types
 */

typedef struct _cx_slnode_ cx_slnode;

struct _cx_slnode_ {
    cxptr data;
    struct _cx_slnode_ *next;
};

struct _cx_slist_ {
    cx_slnode head;
};


/*
 * Attach data to a list node.
 */

inline static void
_cx_slnode_put(cx_slnode *node, cxcptr data)
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
_cx_slnode_get(const cx_slnode *node)
{

    if (!node)
        return NULL;

    return node->data;

}


/*
 * Create a new list node.
 */

inline static cx_slnode *
_cx_slnode_create(cxcptr data)
{

    cx_slnode *node = cx_malloc(sizeof *node);

    _cx_slnode_put(node, data);
    node->next = NULL;

    return node;

}


/*
 * Deallocate a list node. A handle to the node's data is returned to the
 * caller.
 */

inline static cxptr
_cx_slnode_delete(cx_slnode *node)
{

    cxptr data;

    if (!node)
        return NULL;

    data = _cx_slnode_get(node);
    cx_free(node);

    return data;

}


/*
 * Destroy a list node, i.e. at first the node's data is deallocated using
 * the passed deallocator and then the node itself. If NULL is passed as
 * deallocator the data is not deallocated and the function call is
 * equivalent to _cx_slnode_delete().
 */

inline static void
_cx_slnode_destroy(cx_slnode *node, cx_free_func dealloc)
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
_cx_slnode_follows(const cx_slnode *head, const cx_slnode *node)
{
    cx_slnode *n = head->next;

    while (n) {
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
_cx_slnode_exists(const cx_slist *list, const cx_slnode *node)
{

    return _cx_slnode_follows(&list->head, node);

}


/*
 * Initialize a newly created list object
 */

inline static void
_cx_slist_init(cx_slist *list)
{

    if (!list)
        return;

    list->head.next = NULL;

}


/*
 * Get an iterator to the beginning of a list.
 */

inline static cx_slist_iterator
_cx_slist_begin(const cx_slist *list)
{

    return list->head.next;

}


/*
 * Get an iterator for the end of a list.
 */

inline static cx_slist_iterator
_cx_slist_end(const cx_slist *list)
{

    (void) list;  /* unused parameter */
    return NULL;

}


/*
 * Calculate the distance between two list nodes.
 */

inline static cxsize
_cx_slist_distance(const cx_slnode *first, const cx_slnode *last)
{

    register cxsize sz = 0;
    register cx_slnode *node;

    for (node = (cx_slnode *)first; node != last; node = node->next)
        ++sz;

    return sz;

}


/*
 * Inserts a single node.
 */

inline static cx_slnode *
_cx_slist_make_link(cx_slnode *position, cx_slnode *node)
{

    node->next = position->next;
    position->next = node;

    return node;

}


/*
 * Returns the previous list node.
 */

inline static cx_slnode *
_cx_slist_previous(const cx_slnode *head, const cx_slnode *node)
{

    while (head && head->next != node)
        head = head->next;

    return (cx_slnode *)head;

}


/*
 * Insert data into a list after a given iterator position. Returns the
 * iterator position of the inserted item.
 */

inline static cx_slnode *
_cx_slist_insert_after(cx_slist_iterator position, cxcptr data)
{

    return _cx_slist_make_link(position, _cx_slnode_create(data));

}


/*
 * Erase a list element after a given position. Returns the node immediately
 * following the erased list node. The data is deallocated using the provided
 * deallocator function. If this is NULL, the data is not destroyed.
 */

inline static cx_slnode *
_cx_slist_erase_after(cx_slist_iterator position, cx_free_func deallocate)
{

    cx_slnode *next = position->next;

    if (!next)
        return NULL;

    position->next = next->next;
    _cx_slnode_destroy(next, deallocate);

    return position->next;

}


/*
 * Extract a list element after a given position. Returns a handle to the
 * data previously stored at this list position.
 */

inline static cxptr
_cx_slist_extract_after(cx_slist_iterator position)
{

    cx_slnode *next = position->next;


    if (!next)
        return NULL;

    position->next = next->next;

    return _cx_slnode_delete(next);

}


/*
 * Move a range of list nodes in front of the node following the given
 * position.
 */

inline static void
_cx_slist_splice_after(cx_slnode *position, cx_slnode *before_first,
                       cx_slnode *before_last)
{
    if (position != before_first && position != before_last) {
        cx_slnode *first = before_first->next;
        cx_slnode *after = position->next;

        before_first->next = before_last->next;
        position->next = first;
        before_last->next = after;
    }
}


/*
 * Reverse the list starting at the given node
 */

inline static cx_slnode *
_cx_slist_reverse(cx_slnode *node)
{

    cx_slnode *result = node;

    node = node->next;
    result->next = NULL;

    while (node)
    {
        cx_slnode *next = node->next;

        node->next = result;
        result = node;
        node = next;
    }

    return result;

}


/*
 * Check if a given list is sorted using the provided comparison function.
 * Returns 1 if the list is sorted, 0 otherwise.
 */

inline static cxbool
_cx_slist_sorted(cx_slist *list, cx_compare_func compare)
{

    cx_slnode *node = list->head.next;
    cx_slnode *next = NULL;

    if (node != NULL)
        next = node->next;

    while (next != NULL) {
        if (compare(_cx_slnode_get(node), _cx_slnode_get(next)) > 0)
            return 0;

        node = next;
        next = next->next;
    }

    return 1;

}


/*
 * Get an iterator for the list element directly following the
 * given list node.
 */

inline static cx_slist_iterator
_cx_slist_next(cx_slist_const_iterator position)
{

    return position->next;

}


/*
 * Remove all elements from a list. The data object pointed to by the list
 * nodes are not deallocated!
 */

inline static void
_cx_slist_clear(cx_slist *list)
{
    cx_slist_iterator sl;

    if (!list)
        return;

    sl = _cx_slist_begin(list);
    while (sl != _cx_slist_end(list)) {
        cx_slnode *node = sl;

        sl = _cx_slist_next(sl);
        list->head.next = sl;

        _cx_slnode_delete(node);
    }

    _cx_slist_init(list);

    return;

}


/*
 * Get the size of a list as the distance between the first and the sentinel
 * element of the list.
 */

inline static cxsize
_cx_slist_size(const cx_slist *list)
{

    return _cx_slist_distance(_cx_slist_begin(list), _cx_slist_end(list));

}


/*
 * Extract the list element at the given position from the list.
 * A handle to the data object, which is associated to the list
 * element is returned.
 */

inline static cxptr
_cx_slist_extract(cx_slist *list, cx_slist_iterator position)
{

    cx_slnode *prev = _cx_slist_previous(&list->head, position);

    return _cx_slist_extract_after(prev);

}


/*
 * Merge two sorted lists keeping the sorting order with respect to the
 * given comparison function.
 */

inline static void
_cx_slist_merge(cx_slist *list1, cx_slist *list2, cx_compare_func compare)
{

    cx_slnode *n1;


    if (list1 == list2)
        return;

    cx_assert(_cx_slist_size(list1) + _cx_slist_size(list2) >=
              _cx_slist_size(list1));

    cx_assert(_cx_slist_sorted(list1, compare));
    cx_assert(_cx_slist_sorted(list2, compare));

    n1 = &list1->head;

    while (n1->next && list2->head.next) {
        if (compare(_cx_slnode_get(list2->head.next),
                    _cx_slnode_get(n1->next)) < 0)
            _cx_slist_splice_after(n1, &list2->head, list2->head.next);

        n1 = n1->next;
    }

    if (list2->head.next) {
        n1->next = list2->head.next;
        list2->head.next = NULL;
    }

    return;

}


/*
 * Sort the elements of a list with respect to the given comparison function.
 */

inline static void
_cx_slist_sort(cx_slist *list, cx_compare_func compare)
{

    if (_cx_slist_size(list) > 1) {
        cx_slist tmp;
        cx_slnode *node;
        cxsize middle = _cx_slist_size(list) / 2;


        _cx_slist_init(&tmp);

        node = list->head.next;
        while (--middle)
            node = node->next;

        _cx_slist_splice_after(&tmp.head, &list->head, node);
        _cx_slist_sort(list, compare);
        _cx_slist_sort(&tmp, compare);

        _cx_slist_merge(list, &tmp, compare);
    }

    return;

}


/**
 * @brief
 *   Get list iterator to the beginning of a list.
 *
 * @param list  A list.
 *
 * @return Iterator for the first element in the list, or @b cx_slist_end()
 *   if the list is empty.
 *
 * The function returns a handle to the first element of @em list. The
 * handle cannot be used directly to access the element data, but only
 * through the appropriate functions.
 */

cx_slist_iterator
cx_slist_begin(const cx_slist *list)
{

    cx_assert(list != NULL);

    return _cx_slist_begin(list);

}


/**
 * @brief
 *   Get a list iterator to the end of a list.
 *
 * @param list  A list.
 *
 * @return Iterator for the end of the list.
 *
 * The function returns an iterator for the position one past the last
 * element of the list @em list. The handle cannot be used to directly
 * access the element data, but only through the appropriate functions.
 */

cx_slist_iterator
cx_slist_end(const cx_slist *list)
{

    cx_assert(list != NULL);

    return _cx_slist_end(list);

}


/**
 * @brief
 *   Get a list iterator to the next list element
 *
 * @param list      A list.
 * @param position  Current iterator position.
 *
 * @return Iterator for the next list element.
 *
 * The function returns an iterator for the next element in the list
 * @em list with respect to the current iterator position @em position.
 * If the list @em list is empty or @em position points to the list end
 * the function returns @b cx_slist_end().
 *
 * @see cx_slist_empty()
 */

cx_slist_iterator
cx_slist_next(const cx_slist *list, cx_slist_const_iterator position)
{

    (void) list;  /* Prevent warnings if cx_assert is disabled. */
    cx_assert(list != NULL);
    cx_assert(_cx_slnode_exists(list, position));

    return _cx_slist_next(position);

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
 * still are other references to the removed data objects. After calling
 * @b cx_slist_clear() the list @em list is empty.
 */

void
cx_slist_clear(cx_slist *list)
{

    cx_assert(list != NULL);

    _cx_slist_clear(list);
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
 *   return (cx_slist_size(list) == 0);
 * @endcode
 */

cxbool
cx_slist_empty(const cx_slist *list)
{

    cx_assert(list != NULL);

    return (list->head.next == NULL);

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

cx_slist *
cx_slist_new(void)
{

    cx_slist *list = cx_malloc(sizeof *list);

    _cx_slist_init(list);
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
cx_slist_delete(cx_slist *list)
{

    _cx_slist_clear(list);

    cx_assert(cx_slist_empty(list));
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
 * the data deallocation function @em deallocate and finally dealocates
 * the list object itself.
 */

void
cx_slist_destroy(cx_slist *list, cx_free_func deallocate)
{
    cx_slist_iterator sl;


    if (!list)
        return;

    cx_assert(deallocate != NULL);

    sl = _cx_slist_begin(list);
    while (sl != _cx_slist_end(list)) {
        cx_slnode *node = sl;

        sl = _cx_slist_next(sl);
        list->head.next = sl;

        _cx_slnode_destroy(node, deallocate);
    }

    cx_assert(cx_slist_empty(list));
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
cx_slist_size(const cx_slist *list)
{

    cx_assert(list != NULL);
    return _cx_slist_size(list);

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
cx_slist_max_size(const cx_slist *list)
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
cx_slist_swap(cx_slist *list1, cx_slist *list2)
{

    cx_slnode *tmp;

    tmp = list1->head.next;
    list1->head.next = list2->head.next;
    list2->head.next = tmp;

    return;

}


/**
 * @brief
 *   Assign data to a list position.
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
cx_slist_assign(cx_slist *list, cx_slist_iterator position, cxcptr data)
{

    cxptr tmp;


    (void) list;  /* Prevent warnings if cx_assert is disabled. */
    cx_assert(list != NULL);
    cx_assert(_cx_slnode_exists(list, position));

    tmp = _cx_slnode_get(position);
    _cx_slnode_put(position, data);

    return tmp;

}


/**
 * @brief
 *   Get the first element of a list.
 *
 * @param list  The list to query.
 *
 * @return Handle to the data object stored as the first list element.
 *
 * The function returns a reference to the first data item in the list
 * @em list.
 */

cxptr
cx_slist_front(const cx_slist *list)
{

    cx_assert(list != NULL);
    cx_assert(!cx_slist_empty(list));

    return _cx_slnode_get(_cx_slist_begin(list));

}


/**
 * @brief
 *   Get the last element of a list.
 *
 * @param list  The list to query.
 *
 * @return Handle to the data object stored as the last list element.
 *
 * The function returns a reference to the last data item in the list
 * @em list.
 */

cxptr
cx_slist_back(const cx_slist *list)
{
    cx_slnode *tail;


    cx_assert(list != NULL);
    cx_assert(!cx_slist_empty(list));

    tail = _cx_slist_previous(&list->head, _cx_slist_end(list));
    return _cx_slnode_get(tail);

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
cx_slist_get(const cx_slist *list, cx_slist_const_iterator position)
{

    (void) list;  /* Prevent warnings if cx_assert is disabled. */
    cx_assert(list != NULL);
    cx_assert(_cx_slnode_exists(list, position));


    return _cx_slnode_get(position);

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
 * The function inserts the data handle @em data into the list @em list
 * at the list position given by the list iterator @em position.
 */

cx_slist_iterator
cx_slist_insert(cx_slist *list, cx_slist_iterator position, cxcptr data)
{

    cx_slist_iterator prev, node;

    cx_assert(list != NULL);
    cx_assert(position == _cx_slist_end(list) ||
              _cx_slnode_exists(list, position));
    cx_assert(_cx_slist_size(list) + 1 > _cx_slist_size(list));

    prev = _cx_slist_previous(&list->head, position);
    node = _cx_slist_insert_after(prev, data);

    cx_assert(_cx_slist_size(list) <= cx_slist_max_size(list));

    return node;

}


/**
 * @brief
 *   Insert data at the beginning of a list.
 *
 * @param list  The list to update.
 * @param data  Data to add to the list.
 *
 * @return Nothing
 *
 * The data @em data is inserted into the list @em list before the first
 * element of the list, so that it becomes the new list head.
 *
 * It is equivalent to the statement
 * @code
 *   cx_slist_insert(list, cx_slist_begin(list), data);
 * @endcode
 */

void
cx_slist_push_front(cx_slist *list, cxcptr data)
{

    cx_assert(list != NULL);
    _cx_slist_make_link(&list->head, _cx_slnode_create(data));

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
 *   cx_slist_insert(list, cx_slist_end(list), data);
 * @endcode
 */

void
cx_slist_push_back(cx_slist *list, cxcptr data)
{

    cx_slnode *tail;


    cx_assert(list != NULL);

    tail = _cx_slist_previous(&list->head, _cx_slist_end(list));
     _cx_slist_make_link(tail, _cx_slnode_create(data));

    return;

}


/**
 * @brief
 *   Erase a list list element.
 *
 * @param list        The list to update.
 * @param position    List iterator position.
 * @param deallocate  Data deallocator.
 *
 * @return The iterator for the list position after @em position.
 *
 * The function removes the data object stored at position @em position
 * from the list @em list. The data object is deallocated bz calling
 * the data deallocator @em deallocate.
 */

cx_slist_iterator
cx_slist_erase(cx_slist *list, cx_slist_iterator position,
               cx_free_func deallocate)
{

    cx_slnode *prev;


    cx_assert(list != NULL);
    cx_assert(deallocate != NULL);
    cx_assert(_cx_slnode_exists(list, position));

    prev = _cx_slist_previous(&list->head, position);

    return _cx_slist_erase_after(prev, deallocate);

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
 * @see cx_slist_erase(), cx_slist_remove()
 */

cxptr
cx_slist_extract(cx_slist *list, cx_slist_iterator position)
{

    cx_assert(list != NULL);
    cx_assert(_cx_slnode_exists(list, position));

    return _cx_slist_extract(list, position);

}


/**
 * @brief
 *   Remove the first list element.
 *
 * @param list  The list to update.
 *
 * @return Handle to the data object previously stored as the last
 *   list element.
 *
 * The function removes the first element from the list @em list returning
 * a handle to the previously stored data.
 */

cxptr
cx_slist_pop_front(cx_slist *list)
{

    cx_slnode *node;

    cx_assert(list != NULL);

    node = list->head.next;
    list->head.next = node->next;

    return _cx_slnode_delete(node);

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
 */

cxptr
cx_slist_pop_back(cx_slist *list)
{

    cx_slnode *node;

    cx_assert(list != NULL);
    node = _cx_slist_previous(&list->head, _cx_slist_end(list));

    return _cx_slist_extract(list, node);

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
cx_slist_remove(cx_slist *list, cxcptr data)
{

    cx_slnode *node;


    cx_assert(list != NULL);

    node = &list->head;
    while (node && node->next) {
        if (_cx_slnode_get(node->next) == data)
            _cx_slist_extract_after(node);
        else
            node = node->next;
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
cx_slist_unique(cx_slist *list, cx_compare_func compare)
{

    cx_slnode *first, *next;


    cx_assert(list != NULL);
    cx_assert(compare != NULL);

    first = _cx_slist_begin(list);
    if (first) {
        next = _cx_slist_next(first);
        while (next) {
            if (compare(_cx_slnode_get(first), _cx_slnode_get(next)) == 0)
                _cx_slist_extract_after(first);
            else
                first = next;

            next = _cx_slist_next(first);
        }
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
cx_slist_splice(cx_slist *tlist, cx_slist_iterator position,
                cx_slist *slist, cx_slist_iterator first,
                cx_slist_iterator last)
{

    cx_slnode *before_first, *before_last;


    cx_assert(slist != NULL);
    cx_assert(first == _cx_slist_end(slist) ||
              _cx_slnode_exists(slist, first));
    cx_assert(last == _cx_slist_end(slist) ||
              _cx_slnode_follows(first, last));

    before_first = _cx_slist_previous(&slist->head, first);
    before_last = _cx_slist_previous(&slist->head, last);

    if (before_first != before_last) {
        cx_slnode *before_pos;

        cx_assert(tlist != NULL);
        cx_assert(position == _cx_slist_end(tlist) ||
                  _cx_slnode_exists(tlist, position));

        before_pos = _cx_slist_previous(&tlist->head, position);

        cx_assert(slist != tlist ||
                  (_cx_slnode_follows(position, first) ||
                   _cx_slnode_follows(last, position)));

        _cx_slist_splice_after(before_pos, before_first, before_last);
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
cx_slist_merge(cx_slist *list1, cx_slist *list2, cx_compare_func compare)
{

    cx_assert(list1 != NULL);
    cx_assert(list2 != NULL);
    cx_assert(compare != NULL);

    _cx_slist_merge(list1, list2, compare);
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
cx_slist_sort(cx_slist *list, cx_compare_func compare)
{

    cx_assert(list != NULL);
    cx_assert(compare != NULL);

    _cx_slist_sort(list, compare);
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
cx_slist_reverse(cx_slist *list)
{

    cx_assert(list != NULL);

    if (list->head.next)
        list->head.next = _cx_slist_reverse(list->head.next);

    return;

}


/**@}*/
