/**
 * @file        lcsorts.c
 * @brief       Implementation of the sorting algorithms declared in
 *              lcsorts.h.
 *
 * All algorithms operate on raw bytes. Each one obtains its scratch space
 * with `malloc`, performs the sort, and frees it before returning. The
 * public wrappers validate their arguments and translate allocation failure
 * into `LC_FAILURE`; the internal helpers assume valid inputs.
 */

#include <assert.h>
#include <lccmp.h>
#include <lcdef.h>
#include <lcsorts.h>
#include <stddef.h>
#include <stdlib.h>

#ifdef LC_ENABLE_COUNT
size_t cnt_cmp = 0;
size_t cnt_cpy = 0;
#endif

#define LCMISORT_CUTOFF 32

/* ==================== internal implementations ==================== */

static inline void lcisort_gapped(void *restrict begin, size_t size,
                                  size_t element_size, lccmp cmp, size_t gap,
                                  void *restrict cache)
{
    for (size_t i = gap; i < size; i += gap)
    {
        lcmemcpy(cache, (char *)begin + i * element_size, element_size);
        size_t j = i;
        while (j >= gap &&
               cmp(cache, (char *)begin + (j - gap) * element_size) < 0)
        {
            lcmemcpy((char *)begin + j * element_size,
                     (char *)begin + (j - gap) * element_size, element_size);
            j -= gap;
        }
        lcmemcpy((char *)begin + j * element_size, cache, element_size);
    }
}

static inline void lcisort_internal(void *restrict begin, size_t size,
                                    size_t element_size, lccmp cmp,
                                    void *restrict cache)
{
    lcisort_gapped(begin, size, element_size, cmp, 1, cache);
}

static inline void lcssort_internal(void *restrict begin, size_t size,
                                    size_t element_size, lccmp cmp,
                                    void *restrict cache)
{
    size_t gap = size / 2;
    while (gap != 0)
    {
        for (size_t start = 0; start < gap; start++)
            lcisort_gapped((char *)begin + start * element_size, size - start,
                           element_size, cmp, gap, cache);
        gap /= 2;
    }
}

static void lcmerge_internal(void *restrict left, size_t left_size,
                             void *restrict right, size_t right_size,
                             size_t element_size, lccmp cmp, void *restrict dst)
{
    void *l_ptr = left, *r_ptr = right, *w_ptr = dst,
         *left_end = (char *)left + left_size * element_size,
         *right_end = (char *)right + right_size * element_size;
    while (1)
    {
        if (l_ptr == left_end && r_ptr == right_end)
            break;
        else if (l_ptr == left_end)
        {
            lcmemcpy(w_ptr, r_ptr, ((char *)right_end - (char *)r_ptr));
            break;
        }
        else if (r_ptr == right_end)
        {
            lcmemcpy(w_ptr, l_ptr, ((char *)left_end - (char *)l_ptr));
            break;
        }
        else if (cmp(l_ptr, r_ptr) <= 0)
            lcmemcpy(w_ptr, l_ptr, element_size),
                l_ptr = (char *)l_ptr + element_size,
                w_ptr = (char *)w_ptr + element_size;
        else
            lcmemcpy(w_ptr, r_ptr, element_size),
                r_ptr = (char *)r_ptr + element_size,
                w_ptr = (char *)w_ptr + element_size;
    }
}

static void lcmsort_internal(void *restrict begin, size_t size,
                             size_t element_size, lccmp cmp,
                             void *restrict cache)
{
    if (size == 0 || size == 1)
        return;
    if (size == 2)
    {
        if (cmp(begin, (char *)begin + element_size) > 0)
            lcswap(begin, (char *)begin + element_size, element_size, cache);
        return;
    }
    void *mid = (char *)begin + element_size * (size / 2);
    lcmsort_internal(begin, size / 2, element_size, cmp, cache);
    lcmsort_internal(mid, size - size / 2, element_size, cmp, cache);
    lcmerge_internal(begin, size / 2, mid, size - size / 2, element_size, cmp,
                     cache);
    lcmemcpy(begin, cache, size * element_size);
}

static void lcmisort_internal(void *restrict begin, size_t size,
                              size_t element_size, lccmp cmp, size_t cutoff,
                              void *restrict cache)
{
    if (size <= cutoff)
    {
        lcisort_internal(begin, size, element_size, cmp, cache);
        return;
    }
    void *mid = (char *)begin + element_size * (size / 2);
    lcmisort_internal(begin, size / 2, element_size, cmp, cutoff, cache);
    lcmisort_internal(mid, size - size / 2, element_size, cmp, cutoff, cache);
    lcmerge_internal(begin, size / 2, mid, size - size / 2, element_size, cmp,
                     cache);
    lcmemcpy(begin, cache, size * element_size);
}

static inline void lchsort_siftdown(void *restrict begin, size_t size,
                                    size_t element_size, lccmp cmp, size_t i,
                                    void *restrict cache)
{
    char *base = (char *)begin;
    size_t start = i;
    lcmemcpy(cache, base + i * element_size, element_size);
    while (1)
    {
        size_t l = 2 * i + 1, best;
        if (l >= size)
            break;
        best = l;
        if (l + 1 < size &&
            cmp(base + l * element_size, base + (l + 1) * element_size) < 0)
            best = l + 1;
        if (cmp(cache, base + best * element_size) >= 0)
            break;
        lcmemcpy(base + i * element_size, base + best * element_size,
                 element_size);
        i = best;
    }
    if (start != i)
        lcmemcpy(base + i * element_size, cache, element_size);
}

static inline void lchsort_internal(void *restrict begin, size_t size,
                                    size_t element_size, lccmp cmp,
                                    void *restrict cache)
{
    for (size_t i = size / 2 - 1;; i--)
    {
        lchsort_siftdown(begin, size, element_size, cmp, i, cache);
        if (i == 0)
            break;
    }
    lcswap(begin, (char *)begin + (size - 1) * element_size, element_size,
           cache);
    for (size_t unsorted = size - 1; unsorted != 2; unsorted--)
    {
        lchsort_siftdown(begin, unsorted, element_size, cmp, 0, cache);
        lcswap(begin, (char *)begin + (unsorted - 1) * element_size,
               element_size, cache);
    }
    if (cmp(begin, (char *)begin + element_size) > 0)
        lcswap(begin, (char *)begin + element_size, element_size, cache);
}

/* ==================== public interface ==================== */

int lcmerge(void *left, size_t left_size, void *right, size_t right_size,
            size_t element_size, lccmp cmp, void *dst)
{
    assert(left != NULL && right != NULL && cmp != NULL && dst != NULL &&
           left_size && left_size != 0 && right_size != 0 &&
           element_size != 0 && left != right && right != dst && left != dst);
    if (left == NULL || right == NULL || cmp == NULL || dst == NULL ||
        left_size == 0 || right_size == 0 || element_size == 0 ||
        left == right || right == dst || left == dst)
        return LC_FAILURE;
    lcmerge_internal(left, left_size, right, right_size, element_size, cmp,
                     dst);
    return LC_SUCCESS;
}

int lcisort(void *begin, size_t size, size_t element_size, lccmp cmp)
{
    if (size == 0)
        return LC_SUCCESS;
    assert(begin != NULL && element_size != 0 && cmp != NULL);
    if (begin == NULL || element_size == 0 || cmp == NULL)
        return LC_FAILURE;
    void *cache = malloc(element_size);
    assert(cache != NULL);
    if (cache == NULL)
        return LC_FAILURE;
    lcisort_internal(begin, size, element_size, cmp, cache);
    free(cache);
    return LC_SUCCESS;
}

int lcssort(void *begin, size_t size, size_t element_size, lccmp cmp)
{
    if (size == 0)
        return LC_SUCCESS;
    assert(begin != NULL && element_size != 0 && cmp != NULL);
    if (begin == NULL || element_size == 0 || cmp == NULL)
        return LC_FAILURE;
    void *cache = malloc(element_size);
    assert(cache != NULL);
    if (cache == NULL)
        return LC_FAILURE;
    lcssort_internal(begin, size, element_size, cmp, cache);
    free(cache);
    return LC_SUCCESS;
}

int lchsort(void *begin, size_t size, size_t element_size, lccmp cmp)
{
    if (size == 0)
        return LC_SUCCESS;
    assert(begin != NULL && element_size != 0 && cmp != NULL);
    if (begin == NULL || element_size == 0 || cmp == NULL)
        return LC_FAILURE;
    if (size == 1)
        return LC_SUCCESS;
    void *cache = malloc(element_size);
    assert(cache != NULL);
    if (cache == NULL)
        return LC_FAILURE;
    if (size == 2)
    {
        if (cmp(begin, (char *)begin + element_size) > 0)
            lcswap(begin, (char *)begin + element_size, element_size, cache);
        free(cache);
        return LC_SUCCESS;
    }
    lchsort_internal(begin, size, element_size, cmp, cache);
    free(cache);
    return LC_SUCCESS;
}

int lcmsort(void *begin, size_t size, size_t element_size, lccmp cmp)
{
    if (size == 0)
        return LC_SUCCESS;
    assert(begin != NULL && element_size != 0 && cmp != NULL);
    if (begin == NULL || element_size == 0 || cmp == NULL)
        return LC_FAILURE;
    void *cache = malloc(size * element_size);
    assert(cache != NULL);
    if (cache == NULL)
        return LC_FAILURE;
    lcmsort_internal(begin, size, element_size, cmp, cache);
    free(cache);
    return LC_SUCCESS;
}

int lcmisort(void *begin, size_t size, size_t element_size, lccmp cmp)
{
    if (size == 0)
        return LC_SUCCESS;
    assert(begin != NULL && element_size != 0 && cmp != NULL);
    if (begin == NULL || element_size == 0 || cmp == NULL)
        return LC_FAILURE;
    if (size <= LCMISORT_CUTOFF)
        return lcisort(begin, size, element_size, cmp);
    void *cache = malloc(size * element_size);
    assert(cache != NULL);
    if (cache == NULL)
        return LC_FAILURE;
    lcmisort_internal(begin, size, element_size, cmp, LCMISORT_CUTOFF, cache);
    free(cache);
    return LC_SUCCESS;
}
