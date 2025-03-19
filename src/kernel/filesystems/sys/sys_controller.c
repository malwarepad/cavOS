#include <malloc.h>
#include <pci.h>
#include <sys.h>
#include <util.h>

#include <fb.h>
#include <syscalls.h>

Fakefs rootSys = {0};

typedef struct PciConf {
  uint16_t bus;
  uint8_t  slot;
  uint8_t  function;
} PciConf;

size_t pciConfigRead(OpenFile *fd, uint8_t *out, size_t limit) {
  FakefsFile *file = (FakefsFile *)fd->fakefs;
  PciConf    *conf = (PciConf *)file->extra;

  if (fd->pointer >= 4096)
    return 0;
  int toCopy = 4096 - fd->pointer;
  if (toCopy > limit)
    toCopy = limit;

  for (int i = 0; i < toCopy; i++) {
    uint16_t word =
        ConfigReadWord(conf->bus, conf->slot, conf->function, fd->pointer++);
    out[i] = EXPORT_BYTE(word, true);
  }

  return toCopy;
}

VfsHandlers handlePciConfig = {.read = pciConfigRead,
                               .write = 0,
                               .stat = fakefsFstat,
                               .duplicate = 0,
                               .ioctl = 0,
                               .mmap = 0,
                               .getdents64 = 0,
                               .seek = fsSimpleSeek};

void sysSetupPci(FakefsFile *devices) {
  PCIdevice        *device = (PCIdevice *)malloc(sizeof(PCIdevice));
  PCIgeneralDevice *out = (PCIgeneralDevice *)malloc(sizeof(PCIgeneralDevice));

  for (uint16_t bus = 0; bus < PCI_MAX_BUSES; bus++) {
    for (uint8_t slot = 0; slot < PCI_MAX_DEVICES; slot++) {
      for (uint8_t function = 0; function < PCI_MAX_FUNCTIONS; function++) {
        if (!FilterDevice(bus, slot, function))
          continue;

        GetDevice(device, bus, slot, function);
        GetGeneralDevice(device, out);

        char *dirname = (char *)malloc(128);
        sprintf(dirname, "0000:%02d:%02d.%d", bus, slot, function);

        PciConf *pciconf = (PciConf *)malloc(sizeof(PciConf));
        pciconf->bus = bus;
        pciconf->slot = slot;
        pciconf->function = function;

        FakefsFile *dir =
            fakefsAddFile(&rootSys, devices, dirname, 0,
                          S_IFDIR | S_IRUSR | S_IWUSR, &fakefsRootHandlers);

        // [..]/config
        FakefsFile *confFile =
            fakefsAddFile(&rootSys, dir, "config", 0,
                          S_IFREG | S_IRUSR | S_IWUSR, &handlePciConfig);
        fakefsAttachFile(confFile, pciconf, 4096);

        // [..]/vendor
        char *vendorStr = (char *)malloc(8);
        sprintf(vendorStr, "0x%04x\n", device->vendor_id);
        FakefsFile *vendorFile = fakefsAddFile(&rootSys, dir, "vendor", 0,
                                               S_IFREG | S_IRUSR | S_IWUSR,
                                               &fakefsSimpleReadHandlers);
        fakefsAttachFile(vendorFile, vendorStr, 4096);

        // [..]/irq
        char *irqStr = (char *)malloc(8);
        sprintf(irqStr, "%d\n", out->interruptLine);
        FakefsFile *irqFile =
            fakefsAddFile(&rootSys, dir, "irq", 0, S_IFREG | S_IRUSR | S_IWUSR,
                          &fakefsSimpleReadHandlers);
        fakefsAttachFile(irqFile, irqStr, 4096);

        // [..]/revision
        char *revisionStr = (char *)malloc(8);
        sprintf(revisionStr, "0x%02x\n", device->revision);
        FakefsFile *revisionFile = fakefsAddFile(&rootSys, dir, "revision", 0,
                                                 S_IFREG | S_IRUSR | S_IWUSR,
                                                 &fakefsSimpleReadHandlers);
        fakefsAttachFile(revisionFile, revisionStr, 4096);

        // [..]/device
        char *deviceStr = (char *)malloc(8);
        sprintf(deviceStr, "0x%04x\n", device->device_id);
        FakefsFile *deviceFile = fakefsAddFile(&rootSys, dir, "device", 0,
                                               S_IFREG | S_IRUSR | S_IWUSR,
                                               &fakefsSimpleReadHandlers);
        fakefsAttachFile(deviceFile, deviceStr, 4096);

        // [..]/class
        uint32_t class_code = ConfigReadWord(device->bus, device->slot,
                                             device->function, PCI_SUBCLASS)
                              << 8;
        class_code |= ((uint32_t)device->progIF) << 8;
        char *classStr = (char *)malloc(16);
        sprintf(classStr, "0x%x\n", class_code);
        FakefsFile *class = fakefsAddFile(&rootSys, dir, "class", 0,
                                          S_IFREG | S_IRUSR | S_IWUSR,
                                          &fakefsSimpleReadHandlers);
        fakefsAttachFile(class, classStr, 4096);
      }
    }
  }

  free(device);
  free(out);
}

void sysSetup() {
  FakefsFile *bus =
      fakefsAddFile(&rootSys, rootSys.rootFile, "bus", 0,
                    S_IFDIR | S_IRUSR | S_IWUSR, &fakefsRootHandlers);
  FakefsFile *pci =
      fakefsAddFile(&rootSys, bus, "pci", 0, S_IFDIR | S_IRUSR | S_IWUSR,
                    &fakefsRootHandlers);
  FakefsFile *devices =
      fakefsAddFile(&rootSys, pci, "devices", 0, S_IFDIR | S_IRUSR | S_IWUSR,
                    &fakefsRootHandlers);

  FakefsFile *class =
      fakefsAddFile(&rootSys, rootSys.rootFile, "class", 0,
                    S_IFDIR | S_IRUSR | S_IWUSR, &fakefsRootHandlers);
  FakefsFile *graphics =
      fakefsAddFile(&rootSys, class, "graphics", 0, S_IFDIR | S_IRUSR | S_IWUSR,
                    &fakefsRootHandlers);
  FakefsFile *fb0 =
      fakefsAddFile(&rootSys, graphics, "fb0", 0, S_IFDIR | S_IRUSR | S_IWUSR,
                    &fakefsRootHandlers);
  FakefsFile *device =
      fakefsAddFile(&rootSys, fb0, "device", 0, S_IFDIR | S_IRUSR | S_IWUSR,
                    &fakefsRootHandlers);
  fakefsAddFile(&rootSys, device, "subsystem", "/dev/null",
                S_IFLNK | S_IRUSR | S_IWUSR, &fakefsNoHandlers);

  sysSetupPci(devices);
}

bool sysMount(MountPoint *mount) {
  // install handlers
  mount->handlers = &fakefsHandlers;
  mount->stat = fakefsStat;
  mount->lstat = fakefsLstat;
  mount->readlink = fakefsReadlink;

  mount->fsInfo = malloc(sizeof(FakefsOverlay));
  memset(mount->fsInfo, 0, sizeof(FakefsOverlay));
  FakefsOverlay *sys = (FakefsOverlay *)mount->fsInfo;

  sys->fakefs = &rootSys;
  if (!rootSys.rootFile) {
    fakefsSetupRoot(&rootSys.rootFile);
    sysSetup();
  }

  return true;
}
