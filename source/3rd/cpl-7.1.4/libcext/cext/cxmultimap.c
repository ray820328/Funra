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
#include "cxmultimap.h"


/**
 * @defgroup cxmultimap Multi Maps
 *
 * The module implements a map data type, i.e. a container managing key/value
 * pairs as elements. Their elements are automatically sorted according to
 * a sorting criterion used for the key. The container is optimized for
 * lookup operations. Contrary to ordinary maps a multimap is not restricted
 * to unique keys, but may contain multiple duplicate keys.
 *
 * @par Synopsis:
 * @code
 *   #include <cxmultimap.h>
 * @endcode
 */

/**@{*/

/**
 * @brief
 *   Get an iterator to the first pair in a multimap.
 *
 * @param multimap  The multimap to query.
 *
 * @return Iterator for the first pair or @b cx_multimap_end() if the
 *   multimap is empty.
 *
 * The function returns a handle for the first pair in the multimap
 * @em multimap. The returned iterator cannot be used directly to access
 * the value field of the key/value pair, but only through the appropriate
 * methods.
 */

cx_multimap_iterator
cx_multimap_begin(const cx_multimap *multimap)
{

    return cx_tree_begin(multimap);

}


/**
 * @brief
 *   Get an iterator for the position after the last pair in the multimap.
 *
 * @param multimap  The multimap to query.
 *
 * @return Iterator for the end of the multimap.
 *
 * The function returns an iterator for the position one past the last pair
 * in the multimap @em multimap. The iteration is done in ascending order
 * according to the keys. The returned iterator cannot be used directly to
 * access the value field of the key/value pair, but only through the
 * appropriate methods.
 */

cx_multimap_iterator
cx_multimap_end(const cx_multimap *multimap)
{

    return cx_tree_end(multimap);

}


/**
 * @brief
 *   Get an iterator for the next pair in the multimap.
 *
 * @param multimap  A multimap.
 * @param position  Current iterator position.
 *
 * @return Iterator for the pair immediately following @em position.
 *
 * The function returns an iterator for the next pair in the multimap
 * @em multimap with respect to the current iterator position @em position.
 * Iteration is done in ascending order according to the keys. If the
 * multimap is empty or @em position points to the end of the multimap the
 * function returns @b cx_multimap_end().
 */

cx_multimap_iterator
cx_multimap_next(const cx_multimap *multimap,
                 cx_multimap_const_iterator position)
{

    return cx_tree_next(multimap, position);

}


/**
 * @brief
 *   Get an iterator for the previous pair in the multimap.
 *
 * @param multimap  A multimap.
 * @param position  Current iterator position.
 *
 * @return Iterator for the pair immediately preceding @em position.
 *
 * The function returns an iterator for the previous pair in the multimap
 * @em multimap with respect to the current iterator position @em position.
 * Iteration is done in ascending order according to the keys. If the
 * multimap is empty or @em position points to the beginning of the multimap
 * the function returns @b cx_multimap_end().
 */

cx_multimap_iterator
cx_multimap_previous(const cx_multimap *multimap,
                     cx_multimap_const_iterator position)
{

    return cx_tree_previous(multimap, position);

}


/**
 * @brief
 *   Remove all pairs from a multimap.
 *
 * @param multimap  Multimap to be cleared.
 *
 * @return Nothing.
 *
 * The multimap @em multimap is cleared, i.e. all pairs are removed from
 * the multimap. Keys and values are destroyed using the key and value
 * destructors set up during multimap creation. After calling this function
 * the multimap is empty.
 */

void
cx_multimap_clear(cx_multimap *multimap)
{

    cx_tree_clear(multimap);
    return;

}


/**
 * @brief
 *   Check whether a multimap is empty.
 *
 * @param multimap  A multimap.
 *
 * @return The function returns @c TRUE if the multimap is empty, and
 *   @c FALSE otherwise.
 *
 * The function checks if the multimap contains any pairs. Calling this
 * function is equivalent to the statement:
 * @code
 *   return (cx_multimap_size(multimap) == 0);
 * @endcode
 */

cxbool
cx_multimap_empty(const cx_multimap *multimap)
{

    return cx_tree_empty(multimap);

}


/**
 * @brief
 *   Create a new multimap without any elements.
 *
 * @param  compare        Function used to compare keys.
 * @param  key_destroy    Destructor for the keys.
 * @param  value_destroy  Destructor for the value field.
 *
 * @return Handle for the newly allocated multimap.
 *
 * Memory for a new multimap is allocated and the multimap is initialized
 * to be a valid empty multimap.
 *
 * The multimap's key comparison function is set to @em compare. It must
 * return @c TRUE or @c FALSE if the comparison of the first argument
 * passed to it with the second argument is found to be true or false
 * respectively.
 *
 * The destructors for a multimap node's key and value field are set to
 * @em key_destroy and @em value_destroy. Whenever a multimap node is
 * destroyed these functions are used to deallocate the memory used
 * by the key and the value. Each of the destructors might be @c NULL, i.e.
 * keys and values are not deallocated during destroy operations.
 *
 * @see cx_multimap_compare_func()
 */

cx_multimap *
cx_multimap_new(cx_multimap_compare_func compare, cx_free_func key_destroy,
                cx_free_func value_destroy)
{

    return cx_tree_new(compare, key_destroy, value_destroy);

}


/**
 * @brief
 *   Destroy a multimap and all its elements.
 *
 * @param multimap  The multimap to destroy.
 *
 * @return Nothing.
 *
 * The multimap @em multimap is deallocated. All data values and keys are
 * deallocated using the multimap's key and value destructor. If no
 * key and/or value destructor was set when the @em multimap was created
 * the keys and the stored data values are left untouched. In this
 * case the key and value deallocation is the responsibility of the
 * user.
 *
 * @see cx_multimap_new()
 */

void
cx_multimap_delete(cx_multimap *multimap)
{

    cx_tree_delete(multimap);
    return;

}


/**
 * @brief
 *   Get the actual number of pairs in the multimap.
 *
 * @param multimap  A multimap.
 *
 * @return The current number of pairs, or 0 if the multimap is empty.
 *
 * Retrieves the current number of pairs stored in the multimap.
 */

cxsize
cx_multimap_size(const cx_multimap *multimap)
{

    return cx_tree_size(multimap);

}


/**
 * @brief
 *   Get the maximum number of pairs possible.
 *
 * @param multimap  A multimap.
 *
 * @return The maximum number of pairs that can be stored in the multimap.
 *
 * Retrieves the multimap's capacity, i.e. the maximum possible number of
 * pairs a multimap can manage.
 */

cxsize
cx_multimap_max_size(const cx_multimap *multimap)
{

    return cx_tree_max_size(multimap);

}


/**
 * @brief
 *   Retrieve a multimap's key comparison function.
 *
 * @param multimap  The multimap to query.
 *
 * @return Handle for the multimap's key comparison function.
 *
 * The function retrieves the function used by the multimap methods
 * for comparing keys. The key comparison function is set during
 * multimap creation.
 *
 * @see cx_multimap_new()
 */

cx_multimap_compare_func
cx_multimap_key_comp(const cx_multimap *multimap)
{

    return cx_tree_key_comp(multimap);

}


/**
 * @brief
 *   Swap the contents of two multimaps.
 *
 * @param multimap1  First multimap.
 * @param multimap2  Second multimap.
 *
 * @return Nothing.
 *
 * All pairs stored in the first multimap @em multimap1 are moved to the
 * second multimap @em multimap2, while the pairs from @em multimap2 are
 * moved to @em multimap1. Also the key comparison function, the key and
 * the value destructor are exchanged.
 */

void
cx_multimap_swap(cx_multimap *multimap1, cx_multimap *multimap2)
{

    cx_tree_swap(multimap1, multimap2);
    return;

}


/**
 * @brief
 *   Assign data to an iterator position.
 *
 * @param multimap  A multimap.
 * @param position  Iterator positions where the data will be stored.
 * @param data      Data to store.
 *
 * @return Handle to the previously stored data object.
 *
 * The function assigns a data object reference @em data to the iterator
 * position @em position of the multimap @em multimap.
 */

cxptr
cx_multimap_assign(cx_multimap *multimap,
                   cx_multimap_iterator position, cxcptr data)
{

    return cx_tree_assign(multimap, position, data);

}


/**
 * @brief
 *   Get the key from a given iterator position.
 *
 * @param multimap  A multimap.
 * @param position  Iterator position the data is retrieved from.
 *
 * @return Reference for the key.
 *
 * The function returns a reference to the key associated with the iterator
 * position @em position in the multimap @em multimap.
 *
 * @note
 *   One must not modify the key of @em position through the returned
 *   reference, since this might corrupt the multimap!
 */

cxptr
cx_multimap_get_key(const cx_multimap *multimap,
                    cx_multimap_const_iterator position)
{

    return cx_tree_get_key(multimap, position);

}


/**
 * @brief
 *   Get the data from a given iterator position.
 *
 * @param multimap  A multimap.
 * @param position  Iterator position the data is retrieved from.
 *
 * @return Handle for the data object.
 *
 * The function returns a reference to the data stored at iterator position
 * @em position in the multimap @em multimap.
 */

cxptr
cx_multimap_get_value(const cx_multimap *multimap,
                      cx_multimap_const_iterator position)
{

    return cx_tree_get_value(multimap, position);

}


/**
 * @brief
 *   Locate an element in the multimap.
 *
 * @param multimap  A multimap.
 * @param key       Key of the (key, value) pair to locate.
 *
 * @return Iterator pointing to the sought-after element, or
 *   @b cx_multimap_end() if it was not found.
 *
 * The function searches the multimap @em multimap for the first element
 * with a key matching @em key. If the search was successful an iterator
 * to the sought-after pair is returned. If the search did not succeed, i.e.
 * @em key is not present in the multimap, a one past the end iterator is
 * returned.
 */

cx_multimap_iterator
cx_multimap_find(const cx_multimap *multimap, cxcptr key)
{

    return cx_tree_find(multimap, key);

}


/**
 * @brief
 *   Find the beginning of a subsequence matching a given key.
 *
 * @param multimap  A multimap.
 * @param key       Key of the (key, value) pair(s) to locate.
 *
 * @return Iterator pointing to the first position where an element with
 *   key @em key would get inserted, i.e. the first element with a key greater
 *   or equal than @em key.
 *
 * The function returns the first element of a subsequence of elements in the
 * multimap that match the given key @em key. If @em key is not present in the
 * multimap @em multimap an iterator pointing to the first element that has
 * a greater key than @em key or @b cx_multimap_end() if no such element
 * exists.
 */

cx_multimap_iterator
cx_multimap_lower_bound(const cx_multimap *multimap, cxcptr key)
{

    return cx_tree_lower_bound(multimap, key);

}


/**
 * @brief
 *   Find the end of a subsequence matching a given key.
 *
 * @param multimap  A multimap.
 * @param key       Key of the (key, value) pair(s) to locate.
 *
 * @return Iterator pointing to the last position where an element with
 *   key @em key would get inserted, i.e. the first element with a key
 *   greater than @em key.
 *
 * The function returns the last element of a subsequence of elements in the
 * multimap that match the given key @em key. If @em key is not present in the
 * multimap @em multimap an iterator pointing to the first element that has
 * a greater key than @em key or @b cx_multimap_end() if no such element
 * exists.
 */

cx_multimap_iterator
cx_multimap_upper_bound(const cx_multimap *multimap, cxcptr key)
{

    return cx_tree_upper_bound(multimap, key);

}


/**
 * @brief
 *   Find a subsequence matching a given key.
 *
 * @param multimap  A multimap.
 * @param key       The key of the (key, value) pair(s) to be located.
 * @param begin     First element with key @em key.
 * @param end       Last element with key @em key.
 *
 * @return Nothing.
 *
 * The function returns the beginning and the end of a subsequence of
 * multimap elements with the key @em key through through the @em begin and
 * @em end arguments. After calling this function @em begin possibly points
 * to the first element of @em multimap matching the key @em key and @em end
 * possibly points to the last element of the sequence. If key is not
 * present in the multimap @em begin and @em end point to the next greater
 * element or, if no such element exists, to @b cx_multimap_end().
 */

void
cx_multimap_equal_range(const cx_multimap *multimap, cxcptr key,
                   cx_multimap_iterator *begin, cx_multimap_iterator *end)
{

    cx_tree_equal_range(multimap, key, begin, end);
    return;

}


/**
 * @brief
 *   Get the number of elements matching a key.
 *
 * @param multimap  A multimap.
 * @param key       Key of the (key, value) pair(s) to locate.
 *
 * @return The number of elements with the specified key.
 *
 * Counts all elements of the multimap @em multimap matching the key @em key.
 */

cxsize
cx_multimap_count(const cx_multimap *multimap, cxcptr key)
{

    return cx_tree_count(multimap, key);

}


/**
 * @brief
 *   Insert data into a multimap.
 *
 * @param multimap  A multimap.
 * @param key       Key used to store the data.
 * @param data      Data to insert.
 *
 * @return An iterator that points to the inserted pair.
 *
 * This function inserts a (key, value) pair into the multimap @em multimap.
 * The same key may be inserted with different data values.
 */

cx_multimap_iterator
cx_multimap_insert(cx_multimap *multimap, cxcptr key, cxcptr data)
{

    return cx_tree_insert_equal(multimap, key, data);

}


/**
 * @brief
 *   Erase an element from a multimap.
 *
 * @param multimap  A multimap.
 * @param position  Iterator position of the element to be erased.
 *
 * @return Nothing.
 *
 * This function erases an element, specified by the iterator @em position,
 * from @em multimap. Key and value associated with the erased pair are
 * deallocated using the multimap's key and value destructors, provided
 * they have been set.
 */

void
cx_multimap_erase_position(cx_multimap *multimap,
                           cx_multimap_iterator position)
{

    cx_tree_erase_position(multimap, position);
    return;

}


/**
 * @brief
 *   Erase a range of elements from a multimap.
 *
 * @param multimap    A multimap.
 * @param begin       Iterator pointing to the start of the range to erase.
 * @param end         Iterator pointing to the end of the range to erase.
 *
 * @return Nothing.
 *
 * This function erases all elements in the range [begin, end) from
 * the multimap @em multimap. Key and value associated with the erased
 * pair(s) are deallocated using the multimap's key and value destructors,
 * provided they have been set.
 */

void
cx_multimap_erase_range(cx_multimap *multimap, cx_multimap_iterator begin,
                        cx_multimap_iterator end)
{

    cx_tree_erase_range(multimap, begin, end);
    return;

}


/**
 * @brief
 *   Erase an element from a multimap according to the provided key.
 *
 * @param multimap  A multimap.
 * @param key       Key of the element to be erased.
 *
 * @return The number of removed elements.
 *
 * This function erases the element with the specified key @em key,
 * from @em multimap. Key and value associated with the erased pair are
 * deallocated using the multimap's key and value destructors, provided
 * they have been set.
 */

cxsize
cx_multimap_erase(cx_multimap *multimap, cxcptr key)
{

    return cx_tree_erase(multimap, key);

}
/**@}*/
