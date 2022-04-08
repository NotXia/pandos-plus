#include <exceptions.h>

void interruptHandler() {
    unsigned int ip = PREV_PROCESSOR_STATE->cause & CAUSE_IP_MASK;

    if (ip & 0b10 != 0) { // PLT

    }
    else if (ip & 0b100 != 0) { // IT

    }
    else { // Devices
        unsigned int shift = 0b1000;
        for (int i=0; i<4; i++) {
            if (ip & shift != 0) {

            }
            shift = shift << 1;
        }

        if (ip & 0b1000000 != 0) { // Terminali

        }
    }

}
