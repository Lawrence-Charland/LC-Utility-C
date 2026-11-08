/**
 * @file        main.c
 * @brief       Scratch driver for manually poking at lcsorts.
 * @note        This is only a temporary draft: it is not a test suite, not
 *              part of the library API, and not a deliverable. It may be
 *              rewritten or deleted at any time. The real checks live in
 *              testsorts.c; this file is kept around only for quick manual
 *              checks.
 */

#include <lcsorts.h>
// #include <stdio.h>
#include <lccmp.h>

int main(void)
{
    int arr[10] = {1, 0, 2, 9, 3, 8, 4, 7, 5, 6};
    lcisort(arr, 10, sizeof(int), lccmp_int);
    return 0;
}