#include "utilities.h"

void *memcpy(void *dest, const void *src, int n)
{
    for (int i = 0; i < n; i++)
    {
        ((char *)dest)[i] = ((char *)src)[i];
    }
}