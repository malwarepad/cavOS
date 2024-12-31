#include "task.h"
#include "types.h"

#ifndef KERNEL_HELPER_H
#define KERNEL_HELPER_H

Task *netHelperTask;
void  kernelHelpEntry();

void initiateKernelThreads();

#endif