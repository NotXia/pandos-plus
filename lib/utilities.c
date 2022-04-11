/*
    Fonte: https://stackoverflow.com/questions/66809657/undefined-reference-to-memcpy-error-caused-by-ld
*/

#include "utilities.h"

void memcpy(void *dest, const void *src, unsigned int n) {
    for (unsigned int i=0; i<n; i++) {
        ((char *)dest)[i] = ((char *)src)[i];
    }
}