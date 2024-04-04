#include "util.h"

#ifndef PCI_ID_H
#define PCI_ID_H

#define DEFAULT_PCI_ID_PATH "/usr/share/hwdata/pci.ids"
#define PCI_ID_LIMIT 64
#define PCI_ID_LIMIT_STR (PCI_ID_LIMIT * 2 + 1)
// MANUFACTURER - MODEL - (NEWLINE)

typedef struct PCI_ID_SESSION {
  uint8_t *buff;
  uint32_t size;
} PCI_ID_SESSION;

PCI_ID_SESSION *PCI_IDstart();

void initiatePCI_ID();

void PCI_IDend(PCI_ID_SESSION *session);

#endif
