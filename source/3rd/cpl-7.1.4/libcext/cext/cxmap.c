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
#include "cxmap.h"


/**
 * @defgroup cxmap Maps
 *
 * The module implements a map data type, i.e. a container managing key/value
 * pairs as elements. Their elements are automatically sorted according to
 * a sorting criterion used for the key. The container is optimized for
 * lookup operations. Maps are restriced to unique keys, i.e. a key can
 * only appear once in a map.
 *
 * @par Synopsis:
 * @code
 *   #include <cxmap.h>
 * @endcode
 */

/**@{*/

/**
 * @brief
 *   Get an iterator to the first pair in a map.
 *
 * @param map  The map to query.
 *
 * @return Iterator for the first pair or @b cx_map_end() if the map is
 *   empty.
 *
 * The function returns a handle for the first pair in the map @em map.
 * The returned iterator cannot be used directly to access the value field
 * of the key/value pair, but only through the appropriate methods.
 */

cx_map_iterator
cx_map_begin(const cx_map *map)
{

    return cx_tree_begin(map);

}


/**
 * @brief
 *   Get an iterator for the position after the last pair in the map.
 *
 * @param map  The map to query.
 *
 * @return Iterator for the end of the map.
 *
 * The function returns an iterator for the position one past the last pair
 * in the map @em map. The iteration is done in ascending order according
 * to the keys. The returned iterator cannot be used directly to access the
 * value field of the key/value pair, but only through the appropriate
 * methods.
 */

cx_map_iterator
cx_map_end(const cx_map *map)
{

    return cx_tree_end(map);

}


/**
 * @brief
 *   Get an iterator for the next pair in the map.
 *
 * @param map       A map.
 * @param position  Current iterator position.
 *
 * @return Iterator for the pair immediately following @em position.
 *
 * The function returns an iterator for the next pair in the map @em map
 * with respect to the current iterator position @em position. Iteration
 * is done in ascending order according to the keys. If the map is empty
 * or @em position points to the end of the map the function returns
 * @b cx_map_end().
 */

cx_map_iterator
cx_map_next(const cx_map *map, cx_map_const_iterator position)
{

    return cx_tree_next(map, position);

}


/**
 * @brief
 *   Get an iterator for the previous pair in the map.
 *
 * @param map       A map.
 * @param position  Current iterator position.
 *
 * @return Iterator for the pair immediately preceding @em position.
 *
 * The function returns an iterator for the previous pair in the map
 * @em map with respect to the current iterator position @em position.
 * Iteration is done in ascending order according to the keys. If the
 * map is empty or @em position points to the beginning of the map the
 * function returns @b cx_map_end().
 */

cx_map_iterator
cx_map_previous(const cx_map *map, cx_map_const_iterator position)
{

    return cx_tree_previous(map, position);

}


/**
 * @brief
 *   Remove all pairs from a map.
 *
 * @param map  Map to be cleared.
 *
 * @return Nothing.
 *
 * The map @em map is cleared, i.e. all pairs are removed from the map.
 * Keys and values are destroyed using the key and value destructors set up
 * during map creation. After calling this function the map is empty.
 */

void
cx_map_clear(cx_map *map)
{

    cx_tree_clear(map);
    return;

}


/**
 * @brief
 *   Check whether a map is empty.
 *
 * @param map  A map.
 *
 * @return The function returns @c TRUE if the map is empty, and @c FALSE
 *   otherwise.
 *
 * The function checks if the map contains any pairs. Calling this function
 * is equivalent to the statement:
 * @code
 *   return (cx_map_size(map) == 0);
 * @endcode
 */

cxbool
cx_map_empty(const cx_map *map)
{

    return cx_tree_empty(map);

}


/**
 * @brief
 *   Create a new map without any elements.
 *
 * @param  compare        Function used to compare keys.
 * @param  key_destroy    Destructor for the keys.
 * @param  value_destroy  Destructor for the value field.
 *
 * @return Handle for the newly allocated map.
 *
 * Memory for a new map is allocated and the map is initialized to be a
 * valid empty map.
 *
 * The map's key comparison function is set to @em compare. It must
 * return @c TRUE if the comparison of its first argument with its second
 * argument succeeds, and @c FALSE otherwise.
 *
 * The destructors for a map node's key and value field are set to
 * @em key_destroy and @em value_destroy. Whenever a map node is
 * destroyed these functions are used to deallocate the memory used
 * by the key and the value. Each of the destructors might be @c NULL, i.e.
 * keys and values are not deallocated during destroy operations.
 *
 * @see cx_map_compare_func()
 */

cx_map *
cx_map_new(cx_map_compare_func compare, cx_free_func key_destroy,
            cx_free_func value_destroy)
{

    return cx_tree_new(compare, key_destroy, value_destroy);

}


/**
 * @brief
 *   Destroy a map and all its elements.
 *
 * @param map  The map to destroy.
 *
 * @return Nothing.
 *
 * The map @em map is deallocated. All data values and keys are
 * deallocated using the map's key and value destructor. If no
 * key and/or value destructor was set when the @em map was created
 * the keys and the stored data values are left untouched. In this
 * case the key and value deallocation is the responsibility of the
 * user.
 *
 * @see cx_map_new()
 */

void
cx_map_delete(cx_map *map)
{

    cx_tree_delete(map);
    return;

}


/**
 * @brief
 *   Get the actual number of pairs in the map.
 *
 * @param map  A map.
 *
 * @return The current number of pairs, or 0 if the map is empty.
 *
 * Retrieves the current number of pairs stored in the map.
 */

cxsize
cx_map_size(const cx_map *map)
{

    return cx_tree_size(map);

}


/**
 * @brief
 *   Get the maximum number of pairs possible.
 *
 * @param map  A map.
 *
 * @return The maximum number of pairs that can be stored in the map.
 *
 * Retrieves the map's capacity, i.e. the maximum possible number of
 * pairs a map can manage.
 */

cxsize
cx_map_max_size(const cx_map *map)
{

    return cx_tree_max_size(map);

}


/**
 * @brief
 *   Retrieve a map's key comparison function.
 *
 * @param map  The map to query.
 *
 * @return Handle for the map's key comparison function.
 *
 * The function retrieves the function used by the map methods
 * for comparing keys. The key comparison function is set during
 * map creation.
 *
 * @see cx_map_new()
 */

cx_map_compare_func
cx_map_key_comp(const cx_map *map)
{

    return cx_tree_key_comp(map);

}


/**
 * @brief
 *   Swap the contents of two maps.
 *
 * @param map1  First map.
 * @param map2  Second map.
 *
 * @return Nothing.
 *
 * All pairs stored in the first map @em map1 are moved to the second map
 * @em map2, while the pairs from @em map2 are moved to @em map1. Also
 * the key comparison function, the key and the value destructor are
 * exchanged.
 */

void
cx_map_swap(cx_map *map1, cx_map *map2)
{

    cx_tree_swap(map1, map2);
    return;

}


/**
 * @brief
 *   Assign data to an iterator position.
 *
 * @param map       A map.
 * @param position  Iterator positions where the data will be stored.
 * @param data      Data to store.
 *
 * @return Handle to the previously stored data object.
 *
 * The function assigns a data object reference @em data to the iterator
 * position @em position of the map @em map.
 */

cxptr
cx_map_assign(cx_map *map, cx_map_iterator position, cxcptr data)
{

    return cx_tree_assign(map, position, data);

}


/**
 * @brief
 *    Set the value of a pair matching the given key.
 *
 * @param map   A map.
 * @param key   The key of the map element to be changed.
 * @param data  Data value to be stored.
 *
 * @return Previously stored data value of the (key, value) pair.
 *
 * The function replaces the value of the map element with the key @em key
 * with @em value, if the @em key is present in the map @em map. The old
 * value of the map element is returned. If the key is not yet present in
 * the map the pair (@em key, @em data) is inserted in the map. In this case
 * the returned handle to the previously stored data points to @em data.
 */

cxptr
cx_map_put(cx_map *map, cxcptr key, cxcptr data)
{

    cxptr value = NULL;
    cx_map_iterator pos;


    pos = cx_tree_lower_bound(map, key);

    if (pos == cx_tree_end(map)) {
        value = (cxptr)data;
        cx_tree_insert_unique(map, key, value);

    }
    else
        value = cx_tree_assign(map, pos, data);

    return value;

}


/**
 * @brief
 *   Get the key from a given iterator position.
 *
 * @param map       A map.
 * @param position  Iterator position the data is retrieved from.
 *
 * @return Reference for the key.
 *
 * The function returns a reference to the key associated with the iterator
 * position @em position in the map @em map.
 *
 * @note
 *   One must not modify the key of @em position through the returned
 *   reference, since this might corrupt the map!
 */

cxptr
cx_map_get_key(const cx_map *map, cx_map_const_iterator position)
{

    return cx_tree_get_key(map, position);

}


/**
 * @brief
 *   Get the data from a given iterator position.
 *
 * @param map       A map.
 * @param position  Iterator position the data is retrieved from.
 *
 * @return Handle for the data object.
 *
 * The function returns a reference to the data stored at iterator position
 * @em position in the map @em map.
 */

cxptr
cx_map_get_value(const cx_map *map, cx_map_const_iterator position)
{

    return cx_tree_get_value(map, position);

}


/**
 * @brief
 *   Get the data for a given key.
 *
 * @param map  A map.
 * @param key  Key for which the data should be retrieved.
 *
 * @return Handle to the value of the (key, value) pair matching the key
 *   @em key.
 *
 * The function looks for the key @em key in the map @em map and returns
 * the data associated with this key. If @em key is not present in @em map
 * it is inserted using @c NULL as the associated default value, which is
 * then returned.
 */

cxptr
cx_map_get(cx_map *map, cxcptr key)
{

    cx_map_iterator i = cx_tree_lower_bound(map, key);
    cx_map_compare_func keycmp = cx_tree_key_comp(map);

    if (i == cx_tree_end(map) || keycmp(key, cx_tree_get_key(map, i)))
        i = cx_tree_insert_unique(map, key, NULL);

    return cx_tree_get_value(map, i);

}


/**
 * @brief
 *   Locate an element in the map.
 *
 * @param map  A map.
 * @param key  Key of the (key, value) pair to locate.
 *
 * @return Iterator pointing to the sought-after element, or @b cx_map_end()
 *   if it was not found.
 *
 * The function searches the map @em map for an element with a key
 * matching @em key. If the search was successful an iterator to the
 * sought-after pair is returned. If the search did not succeed, i.e.
 * @em key is not present in the map, a one past the end iterator is
 * returned.
 */

cx_map_iterator
cx_map_find(const cx_map *map, cxcptr key)
{

    return cx_tree_find(map, key);

}


/**
 * @brief
 *   Find the beginning of a subsequence matching a given key.
 *
 * @param map  A map.
 * @param key  Key of the (key, value) pair(s) to locate.
 *
 * @return Iterator pointing to the first position where an element with
 *   key @em key would get inserted, i.e. the first element with a key greater
 *   or equal than @em key.
 *
 * The function returns the first element of a subsequence of elements in the
 * map that match the given key @em key. If @em key is not present in the
 * map @em map an iterator pointing to the first element that has a greater
 * key than @em key or @b cx_map_end() if no such element exists.
 *
 * @note
 *   For maps, where a key can occur only once, is a call to this function
 *   equivalent to calling @b cx_map_find().
 */

cx_map_iterator
cx_map_lower_bound(const cx_map *map, cxcptr key)
{

    return cx_tree_lower_bound(map, key);

}


/**
 * @brief
 *   Find the end of a subsequence matching a given key.
 *
 * @param map  A map.
 * @param key  Key of the (key, value) pair(s) to locate.
 *
 * @return Iterator pointing to the last position where an element with
 *   key @em key would get inserted, i.e. the first element with a key
 *   greater than @em key.
 *
 * The function returns the last element of a subsequence of elements in the
 * map that match the given key @em key. If @em key is not present in the
 * map @em map an iterator pointing to the first element that has a greater
 * key than @em key or @b cx_map_end() if no such element exists.
 *
 * @note
 *   For maps, calling this function is equivalent to:
 *   @code
 *     cx_map_iterator it;
 *
 *     it = cx_map_find(map, key);
 *     it = cx_map_next(map, it);
 *   @endcode
 *   omitting all error checks.
 */

cx_map_iterator
cx_map_upper_bound(const cx_map *map, cxcptr key)
{

    return cx_tree_upper_bound(map, key);

}


/**
 * @brief
 *   Find a subsequence matching a given key.
 *
 * @param map    A map.
 * @param key    The key of the (key, value) pair(s) to be located.
 * @param begin  First element with key @em key.
 * @param end    Last element with key @em key.
 *
 * @return Nothing.
 *
 * The function returns the beginning and the end of a subsequence of
 * map elements with the key @em key through through the @em begin and
 * @em end arguments. After calling this function @em begin possibly points
 * to the first element of @em map matching the key @em key and @em end
 * possibly points to the last element of the sequence. If key is not
 * present in the map @em begin and @em end point to the next greater
 * element or, if no such element exists, to @b cx_map_end().
 */

void
cx_map_equal_range(const cx_map *map, cxcptr key,
                   cx_map_iterator *begin, cx_map_iterator *end)
{

    cx_tree_equal_range(map, key, begin, end);
    return;

}


/**
 * @brief
 *   Get the number of elements matching a key.
 *
 * @param map  A map.
 * @param key  Key of the (key, value) pair(s) to locate.
 *
 * @return The number of elements with the specified key.
 *
 * Counts all elements of the map @em map matching the key @em key.
 */

cxsize
cx_map_count(const cx_map *map, cxcptr key)
{

    return cx_tree_find(map, key) == cx_tree_end(map) ? 0 : 1;

}


/**
 * @brief
 *   Attempt to insert data into a map.
 *
 * @param map   A map.
 * @param key   Key used to store the data.
 * @param data  Data to insert.
 *
 * @return An iterator that points to the inserted pair, or @c NULL if the
 *   pair could not be inserted.
 *
 * This function attempts to insert a (key, value) pair into the map
 * @em map. The insertion fails if the key already present in the map,
 * since a key may only occur once in a map.
 */

cx_map_iterator
cx_map_insert(cx_map *map, cxcptr key, cxcptr data)
{

    return cx_tree_insert_unique(map, key, data);

}


/**
 * @brief
 *   Erase an element from a map.
 *
 * @param map       A map.
 * @param position  Iterator position of the element to be erased.
 *
 * @return Nothing.
 *
 * This function erases an element, specified by the iterator @em position,
 * from @em map. Key and value associated with the erased pair are
 * deallocated using the map's key and value destructors, provided
 * they have been set.
 */

void
cx_map_erase_position(cx_map *map, cx_map_iterator position)
{

    cx_tree_erase_position(map, position);
    return;

}


/**
 * @brief
 *   Erase a range of elements from a map.
 *
 * @param map    A map.
 * @param begin  Iterator pointing to the start of the range to erase.
 * @param end    Iterator pointing to the end of the range to erase.
 *
 * @return Nothing.
 *
 * This function erases all elements in the range [begin, end) from
 * the map @em map. Key and value associated with the erased pair(s) are
 * deallocated using the map's key and value destructors, provided
 * they have been set.
 */

void
cx_map_erase_range(cx_map *map, cx_map_iterator begin,
                   cx_map_iterator end)
{

    cx_tree_erase_range(map, begin, end);
    return;

}


/**
 * @brief
 *   Erase an element from a map according to the provided key.
 *
 * @param map  A map.
 * @param key  Key of the element to be erased.
 *
 * @return The number of removed elements.
 *
 * This function erases the element with the specified key @em key,
 * from @em map. Key and value associated with the erased pair are
 * deallocated using the map's key and value destructors, provided
 * they have been set.
 *
 * @note
 *   For maps the the returned number should only be 0 or 1, due to the
 *   nature of maps.
 */

cxsize
cx_map_erase(cx_map *map, cxcptr key)
{

    return cx_tree_erase(map, key);

}
/**@}*/
