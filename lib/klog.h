#ifndef PANDOS_KLOG_H_INCLUDED
#define PANDOS_KLOG_H_INCLUDED


// Print str to klog
void klog_print(char *str);

// Princ a number in hexadecimal format (best for addresses)
void klog_print_hex(unsigned int num);


#endif