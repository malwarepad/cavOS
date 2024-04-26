#include <fs_controller.h>
#include <malloc.h>
#include <pci.h>
#include <pci_id.h>
#include <string.h>

// https://pci-ids.ucw.cz/
// pci.ids file parser... Basically a database of PCI devices!
// Copyright (C) 2024 Panagiotis

PCI_ID_SESSION *PCI_IDstart() {
  OpenFile *file = fsKernelOpen(DEFAULT_PCI_ID_PATH, FS_MODE_READ, 0);
  if (!file) {
    debugf("[pci_id] No id file found at %s!\n", DEFAULT_PCI_ID_PATH);
    return 0;
  }

  uint32_t filesize = fsGetFilesize(file);

  uint8_t *buff = (uint8_t *)malloc(filesize);
  fsReadFullFile(file, buff);

  PCI_ID_SESSION *session = (PCI_ID_SESSION *)malloc(sizeof(PCI_ID_SESSION));

  session->buff = buff;
  session->size = filesize;
  return session;
}

void PCI_IDend(PCI_ID_SESSION *session) {
  free(session->buff);
  free(session);
}

#define CHAR_TAB 0x09
char *PCI_IDlookup(PCI_ID_SESSION *session, uint16_t vendor_id,
                   uint16_t device_id) {
  char *output = 0;
  bool  ignoreTabbed = true;

  bool stateCommented = false;
  bool stateTabbed = false;
  bool stateC = false;

  uint8_t *recording = 0;
  size_t   recordingPtr = 0;

  for (size_t curr = 0; curr <= session->size; curr++) {
    switch (session->buff[curr]) {
    case '#':
      stateCommented = true;
      break;
    case CHAR_TAB:
      stateTabbed = true;
      break;
    case 'C':
      if (session->buff[curr - 1] == '\n')
        stateC = true;
      break;

    default:
      break;
    }

    if (session->buff[curr] == '\n') {
      stateCommented = false;
      stateTabbed = false;
      stateC = false;

      recording = 0;
      recordingPtr = 0;
    }

    if (session->buff[curr] == '\n' || stateCommented || stateC ||
        (stateTabbed && ignoreTabbed))
      continue;

    if (session->buff[curr - 1] == '\n') {
      if (session->buff[curr] != CHAR_TAB)
        ignoreTabbed = true; // different entry!

      if (strtol((char *)(&session->buff[curr]), 0, 16) ==
          (ignoreTabbed ? vendor_id : device_id)) {
        if (!output) {
          output = malloc(PCI_ID_LIMIT_STR);
          memset(output, 0, PCI_ID_LIMIT_STR);
        }

        recording = (uint8_t *)(output + strlength(output));
        recordingPtr = 0;

        curr += 6;
        ignoreTabbed = false;
      }
    }

    if (recording && recordingPtr <= (PCI_ID_LIMIT - 1)) {
      recording[recordingPtr++] = session->buff[curr];
    }
  }

  return output;
}

void initiatePCI_ID() {
  PCI_ID_SESSION *session = PCI_IDstart();
  if (!session)
    return;

  PCI *browse = firstPCI;
  while (browse) {
    char *output = PCI_IDlookup(session, browse->vendor_id, browse->device_id);
    if (output)
      browse->name = output;
    browse = browse->next;
  }

  PCI_IDend(session);
}
