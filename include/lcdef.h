/**
 * @file        lcdef.h
 * @brief       Common definitions for lcutilityc: status codes, byte-level
 *              memory primitives and optional operation counters.
 * @author      Lawrence Charland
 * @version     0.1.0
 */

#ifndef LCPROJECTS_LCDEF_H
#define LCPROJECTS_LCDEF_H

/** @brief Operation completed successfully. */
#define LC_SUCCESS 0
/** @brief Operation failed (invalid argument or allocation failure). */
#define LC_FAILURE 1

#include <stddef.h>
#include <stdint.h>
#include <string.h>

/**
 * @defgroup lcdef_counters Operation counters
 * @brief Optional comparison/copy counters for experiments and benchmarks.
 *
 * Counting code is compiled in only when `LC_ENABLE_COUNT` is defined at
 * build time (the `LC_ENABLE_COUNT` CMake option). Otherwise every counting
 * macro expands to a no-op and costs nothing at run time.
 * @{
 */
#ifdef LC_ENABLE_COUNT
/** @brief Number of comparisons recorded since the last reset. */
extern size_t cnt_cmp;
/** @brief Number of element copies recorded since the last reset. */
extern size_t cnt_cpy;

/** @brief Increment the comparison counter. */
#define LC_CNT_CMP() (cnt_cmp++)
/** @brief Increment the copy counter. */
#define LC_CNT_CPY() (cnt_cpy++)
/** @brief Reset both counters to zero. */
#define LC_CNT_RESET() (cnt_cmp = cnt_cpy = 0)
#else
/** @brief No-op when counting is disabled. */
#define LC_CNT_CMP() ((void)0)
/** @brief No-op when counting is disabled. */
#define LC_CNT_CPY() ((void)0)
/** @brief No-op when counting is disabled. */
#define LC_CNT_RESET() ((void)0)
#endif
/** @} */

/**
 * @brief       Copy `size` bytes, counting the operation when enabled.
 * @details     Four- and eight-byte copies use direct integer loads and
 *              stores; every other size falls back to `memcpy`. This keeps
 *              the sorting algorithms fast on the common element sizes
 *              (int, pointers, size_t) without special-casing them there.
 *
 * @param[out] dst  Destination buffer; must not overlap `src`.
 * @param[in]  src  Source buffer.
 * @param[in]  size Number of bytes to copy.
 *
 * @pre  `dst` and `src` do not overlap.
 * @pre  `dst` has room for `size` bytes and `src` holds at least `size`.
 * @note The 4- and 8-byte paths cast to `uint32_t *` / `uint64_t *`, so the
 *       buffers should be suitably aligned (as `malloc` and normal objects
 *       are for their own type).
 */
static inline void lcmemcpy(void *restrict dst, const void *restrict src,
                            size_t size)
{
    LC_CNT_CPY();
    switch (size)
    {
    case 4:
        *(uint32_t *)dst = *(const uint32_t *)src;
        break;
    case 8:
        *(uint64_t *)dst = *(const uint64_t *)src;
        break;
    default:
        memcpy(dst, src, size);
        break;
    }
}

/**
 * @brief       Swap two elements of `size` bytes through a scratch buffer.
 * @param[in,out] left  First element.
 * @param[in,out] right Second element.
 * @param[in]     size  Element size in bytes.
 * @param[in,out] cache Scratch buffer of at least `size` bytes; it may alias
 *                      neither `left` nor `right`.
 * @pre  `cache` has room for `size` bytes and does not overlap either input.
 */
static inline void lcswap(void *restrict left, void *restrict right,
                          size_t size, void *restrict cache)
{
    lcmemcpy(cache, left, size);
    lcmemcpy(left, right, size);
    lcmemcpy(right, cache, size);
}

#endif
