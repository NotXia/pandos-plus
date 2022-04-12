#ifndef PANDOS_UTILITIES_H_INCLUDED
#define PANDOS_UTILITIES_H_INCLUDED

void memcpy(void *dest, const void *src, unsigned int n);

void klog_print(char *str);
void klog_print_hex(unsigned int num);

#endif