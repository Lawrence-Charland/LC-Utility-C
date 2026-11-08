/**
 * @file        lcsorts.h
 * @brief       Generic comparison-based sorting algorithms for fixed-size
 *              elements. Part of lcutilityc, an LC Projects toolbox.
 * @author      Lawrence Charland
 * @version     0.1.0
 *
 * Overview
 * --------
 * Every algorithm in this header sorts an array of `size` elements, each
 * `element_size` bytes wide, using a caller-supplied three-way comparator
 * (`lccmp`, see lccmp.h). Elements are moved as raw bytes, so the element type
 * must be trivially copyable; nothing inside an element is duplicated, freed
 * or otherwise interpreted by the library.
 *
 * Calling convention
 * ------------------
 * All sorting entry points share one signature (`lcsort`) and return
 * `LC_SUCCESS` (0) or `LC_FAILURE` (1) from lcdef.h. A zero-length range is a
 * no-op and succeeds without dereferencing `begin` or `cmp`. Invalid
 * arguments (null pointers, `element_size == 0`) are asserted in debug builds
 * and reported as `LC_FAILURE` with `NDEBUG`. Allocation failure is always
 * reported as `LC_FAILURE`. When a function fails, the contents of the array
 * are unspecified.
 *
 * The comparator must implement a strict weak ordering and return a negative
 * value, zero or a positive value when the left element sorts before, equal
 * to, or after the right one. The temporary buffers used by the algorithms are
 * allocated and freed internally; the caller only owns the array itself.
 *
 * Stability
 * ---------
 * `lcisort`, `lcmsort`, `lcmisort` and `lcmerge` are stable. `lcssort` and
 * `lchsort` are not. "Stable" here means equal elements keep their original
 * relative order.
 */

#ifndef LCPROJECTS_LCSORTS_H
#define LCPROJECTS_LCSORTS_H

#include <lccmp.h>
#include <stddef.h>

/**
 * @defgroup lcsorts_api Sorting API
 * @brief       Sorting and merging entry points.
 * @{
 */

/**
 * @brief       Common function type of every sorting algorithm.
 * @details     Useful when algorithms are selected at run time, for example
 *              when a test driver keeps a table of `lcsort *` pointers.
 *
 * @param[in,out] begin        First element of the array.
 * @param[in]     size         Number of elements.
 * @param[in]     element_size Size of one element in bytes.
 * @param[in]     cmp          Comparator; see lccmp.h.
 *
 * @return `LC_SUCCESS` on success, `LC_FAILURE` on invalid arguments or
 *         allocation failure.
 */
typedef int lcsort(void *begin, size_t size, size_t element_size, lccmp cmp);

/**
 * @brief       Merge two already sorted ranges into a third range.
 * @details     Copies `left_size + right_size` elements into `dst`, preserving
 *              order. When the current elements compare equal, the one from
 *              `left` is written first, which makes the merge stable.
 *
 * @param[in]  left        Start of the first sorted range.
 * @param[in]  left_size   Number of elements in `left` (must be > 0).
 * @param[in]  right       Start of the second sorted range.
 * @param[in]  right_size  Number of elements in `right` (must be > 0).
 * @param[in]  element_size Size of one element in bytes.
 * @param[in]  cmp         Comparator; see lccmp.h.
 * @param[out] dst         Destination for `left_size + right_size` elements.
 *
 * @retval LC_SUCCESS  The merge was performed.
 * @retval LC_FAILURE  A pointer was NULL, a size was 0, `element_size` was 0,
 *                     or the three ranges overlap.
 *
 * @pre  `left` and `right` are each sorted in ascending order.
 * @pre  `dst` has room for `left_size + right_size` elements.
 * @pre  `left`, `right` and `dst` do not overlap.
 * @note This function performs no allocation.
 * @note Equal elements from `left` are emitted before those from `right`.
 */
int lcmerge(void *left, size_t left_size, void *right, size_t right_size,
            size_t element_size, lccmp cmp, void *dst);

/**
 * @brief       Sort an array with insertion sort.
 * @details     Stable, adaptive on nearly sorted input, and allocation-free
 *              apart from a one-element scratch buffer. Worst case O(n^2)
 *              comparisons and moves; best case O(n).
 *
 * @param[in,out] begin        First element of the array.
 * @param[in]     size         Number of elements.
 * @param[in]     element_size Size of one element in bytes.
 * @param[in]     cmp          Comparator; see lccmp.h.
 *
 * @retval LC_SUCCESS  The array is sorted.
 * @retval LC_FAILURE  Invalid arguments or out of memory.
 *
 * @note Stable.
 * @note Extra memory: one element.
 */
int lcisort(void *begin, size_t size, size_t element_size, lccmp cmp);

/**
 * @brief       Sort an array with Shell sort.
 * @details     Uses the original shell increment sequence (n/2, n/4, ...)
 *              with insertion sort inside each gap. Usually much faster than
 *              plain insertion sort on medium-sized inputs, but the worst
 *              case remains O(n^2).
 *
 * @param[in,out] begin        First element of the array.
 * @param[in]     size         Number of elements.
 * @param[in]     element_size Size of one element in bytes.
 * @param[in]     cmp          Comparator; see lccmp.h.
 *
 * @retval LC_SUCCESS  The array is sorted.
 * @retval LC_FAILURE  Invalid arguments or out of memory.
 *
 * @note Not stable.
 * @note Extra memory: one element.
 */
int lcssort(void *begin, size_t size, size_t element_size, lccmp cmp);

/**
 * @brief       Sort an array with heap sort.
 * @details     Builds a binary max-heap in place and repeatedly moves the
 *              maximum to the end. Runs in O(n log n) time in every case and
 *              needs only a one-element scratch buffer.
 *
 * @param[in,out] begin        First element of the array.
 * @param[in]     size         Number of elements.
 * @param[in]     element_size Size of one element in bytes.
 * @param[in]     cmp          Comparator; see lccmp.h.
 *
 * @retval LC_SUCCESS  The array is sorted.
 * @retval LC_FAILURE  Invalid arguments or out of memory.
 *
 * @note Not stable.
 * @note Extra memory: one element.
 */
int lchsort(void *begin, size_t size, size_t element_size, lccmp cmp);

/**
 * @brief       Sort an array with top-down merge sort.
 * @details     Recursively splits the range in half, sorts both halves and
 *              merges them through a scratch buffer of n elements. Guaranteed
 *              O(n log n) time, at the cost of O(n) extra memory.
 *
 * @param[in,out] begin        First element of the array.
 * @param[in]     size         Number of elements.
 * @param[in]     element_size Size of one element in bytes.
 * @param[in]     cmp          Comparator; see lccmp.h.
 *
 * @retval LC_SUCCESS  The array is sorted.
 * @retval LC_FAILURE  Invalid arguments or out of memory.
 *
 * @note Stable.
 * @note Extra memory: `size` elements.
 */
int lcmsort(void *begin, size_t size, size_t element_size, lccmp cmp);

/**
 * @brief       Sort an array with merge sort and an insertion-sort cutoff.
 * @details     Hybrid of lcmsort and lcisort. Ranges of at most 32 elements
 *              are finished with insertion sort; larger ranges are split and
 *              merged as in lcmsort. This removes most of the recursion
 *              overhead on small subranges while keeping O(n log n) worst-case
 *              behaviour and stability.
 *
 * @param[in,out] begin        First element of the array.
 * @param[in]     size         Number of elements.
 * @param[in]     element_size Size of one element in bytes.
 * @param[in]     cmp          Comparator; see lccmp.h.
 *
 * @retval LC_SUCCESS  The array is sorted.
 * @retval LC_FAILURE  Invalid arguments or out of memory.
 *
 * @note Stable.
 * @note Extra memory: `size` elements (one element for the insertion-only
 *       path).
 */
int lcmisort(void *begin, size_t size, size_t element_size, lccmp cmp);

/** @} */

#endif
