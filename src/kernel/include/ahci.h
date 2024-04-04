#include "pci.h"
#include "types.h"

#ifndef AHCI_H
#define AHCI_H

/* Update from FreeBSD sources!
 * https://github.com/freebsd/freebsd-src/blob/main/sys/dev/ahci/ahci_pci.c */

#define AHCI_Q_NOFORCE 0x00000001
#define AHCI_Q_NOPMP 0x00000002
#define AHCI_Q_NONCQ 0x00000004
#define AHCI_Q_1CH 0x00000008
#define AHCI_Q_2CH 0x00000010
#define AHCI_Q_4CH 0x00000020
#define AHCI_Q_EDGEIS 0x00000040
#define AHCI_Q_SATA2 0x00000080
#define AHCI_Q_NOBSYRES 0x00000100
#define AHCI_Q_NOAA 0x00000200
#define AHCI_Q_NOCOUNT 0x00000400
#define AHCI_Q_ALTSIG 0x00000800
#define AHCI_Q_NOMSI 0x00001000
#define AHCI_Q_ATI_PMP_BUG 0x00002000
#define AHCI_Q_MAXIO_64K 0x00004000
#define AHCI_Q_SATA1_UNIT0 0x00008000 /* need better method for this */
#define AHCI_Q_ABAR0 0x00010000
#define AHCI_Q_1MSI 0x00020000
#define AHCI_Q_FORCE_PI 0x00040000
#define AHCI_Q_RESTORE_CAP 0x00080000
#define AHCI_Q_NOMSIX 0x00100000
#define AHCI_Q_MRVL_SR_DEL 0x00200000
#define AHCI_Q_NOCCS 0x00400000
#define AHCI_Q_NOAUX 0x00800000
#define AHCI_Q_IOMMU_BUSWIDE 0x01000000
#define AHCI_Q_SLOWDEV 0x02000000

typedef struct {
  uint32_t    id;
  uint8_t     rev;
  const char *name;
  int         quirks;
} AHCI_DEVICE;

static const struct {
  uint32_t    id;
  uint8_t     rev;
  const char *name;
  int         quirks;
} ahci_ids[] = {
    {0x43801002, 0x00, "AMD SB600",
     AHCI_Q_NOMSI | AHCI_Q_ATI_PMP_BUG | AHCI_Q_MAXIO_64K},
    {0x43901002, 0x00, "AMD SB7x0/SB8x0/SB9x0",
     AHCI_Q_ATI_PMP_BUG | AHCI_Q_1MSI},
    {0x43911002, 0x00, "AMD SB7x0/SB8x0/SB9x0",
     AHCI_Q_ATI_PMP_BUG | AHCI_Q_1MSI},
    {0x43921002, 0x00, "AMD SB7x0/SB8x0/SB9x0",
     AHCI_Q_ATI_PMP_BUG | AHCI_Q_1MSI},
    {0x43931002, 0x00, "AMD SB7x0/SB8x0/SB9x0",
     AHCI_Q_ATI_PMP_BUG | AHCI_Q_1MSI},
    {0x43941002, 0x00, "AMD SB7x0/SB8x0/SB9x0",
     AHCI_Q_ATI_PMP_BUG | AHCI_Q_1MSI},
    /* Not sure SB8x0/SB9x0 needs this quirk. Be conservative though */
    {0x43951002, 0x00, "AMD SB8x0/SB9x0", AHCI_Q_ATI_PMP_BUG},
    {0x43b61022, 0x00, "AMD X399", 0},
    {0x43b51022, 0x00, "AMD 300 Series", 0}, /* X370 */
    {0x43b71022, 0x00, "AMD 300 Series", 0}, /* B350 */
    {0x78001022, 0x00, "AMD Hudson-2", 0},
    {0x78011022, 0x00, "AMD Hudson-2", 0},
    {0x78021022, 0x00, "AMD Hudson-2", 0},
    {0x78031022, 0x00, "AMD Hudson-2", 0},
    {0x78041022, 0x00, "AMD Hudson-2", 0},
    {0x79001022, 0x00, "AMD KERNCZ", 0},
    {0x79011022, 0x00, "AMD KERNCZ", 0},
    {0x79021022, 0x00, "AMD KERNCZ", 0},
    {0x79031022, 0x00, "AMD KERNCZ", 0},
    {0x79041022, 0x00, "AMD KERNCZ", 0},
    {0x79161022, 0x00, "AMD KERNCZ (RAID)", 0},
    {0x06011b21, 0x00, "ASMedia ASM1060", AHCI_Q_NOCCS | AHCI_Q_NOAUX},
    {0x06021b21, 0x00, "ASMedia ASM1060", AHCI_Q_NOCCS | AHCI_Q_NOAUX},
    {0x06111b21, 0x00, "ASMedia ASM1061", AHCI_Q_NOCCS | AHCI_Q_NOAUX},
    {0x06121b21, 0x00, "ASMedia ASM1062", AHCI_Q_NOCCS | AHCI_Q_NOAUX},
    {0x06201b21, 0x00, "ASMedia ASM106x", AHCI_Q_NOCCS | AHCI_Q_NOAUX},
    {0x06211b21, 0x00, "ASMedia ASM106x", AHCI_Q_NOCCS | AHCI_Q_NOAUX},
    {0x06221b21, 0x00, "ASMedia ASM106x", AHCI_Q_NOCCS | AHCI_Q_NOAUX},
    {0x06241b21, 0x00, "ASMedia ASM106x", AHCI_Q_NOCCS | AHCI_Q_NOAUX},
    {0x06251b21, 0x00, "ASMedia ASM106x", AHCI_Q_NOCCS | AHCI_Q_NOAUX},
    {0x10621b21, 0x00, "ASMedia ASM116x", 0},
    {0x10641b21, 0x00, "ASMedia ASM116x", 0},
    {0x11641b21, 0x00, "ASMedia ASM116x", 0},
    {0x11651b21, 0x00, "ASMedia ASM116x", 0},
    {0x11661b21, 0x00, "ASMedia ASM116x", 0},
    {0x79011d94, 0x00, "Hygon KERNCZ", 0},
    {0x0f228086, 0x00, "Intel BayTrail", 0},
    {0x0f238086, 0x00, "Intel BayTrail", 0},
    {0x26528086, 0x00, "Intel ICH6", AHCI_Q_NOFORCE},
    {0x26538086, 0x00, "Intel ICH6M", AHCI_Q_NOFORCE},
    {0x26818086, 0x00, "Intel ESB2", 0},
    {0x26828086, 0x00, "Intel ESB2", 0},
    {0x26838086, 0x00, "Intel ESB2", 0},
    {0x27c18086, 0x00, "Intel ICH7", 0},
    {0x27c38086, 0x00, "Intel ICH7", 0},
    {0x27c58086, 0x00, "Intel ICH7M", 0},
    {0x27c68086, 0x00, "Intel ICH7M", 0},
    {0x28218086, 0x00, "Intel ICH8", 0},
    {0x28228086, 0x00, "Intel ICH8+ (RAID)", 0},
    {0x28248086, 0x00, "Intel ICH8", 0},
    {0x28298086, 0x00, "Intel ICH8M", 0},
    {0x282a8086, 0x00, "Intel ICH8M+ (RAID)", 0},
    {0x29228086, 0x00, "Intel ICH9", 0},
    {0x29238086, 0x00, "Intel ICH9", 0},
    {0x29248086, 0x00, "Intel ICH9", 0},
    {0x29258086, 0x00, "Intel ICH9", 0},
    {0x29278086, 0x00, "Intel ICH9", 0},
    {0x29298086, 0x00, "Intel ICH9M", 0},
    {0x292a8086, 0x00, "Intel ICH9M", 0},
    {0x292b8086, 0x00, "Intel ICH9M", 0},
    {0x292c8086, 0x00, "Intel ICH9M", 0},
    {0x292f8086, 0x00, "Intel ICH9M", 0},
    {0x294d8086, 0x00, "Intel ICH9", 0},
    {0x294e8086, 0x00, "Intel ICH9M", 0},
    {0x3a028086, 0x00, "Intel ICH10", 0},
    {0x3a058086, 0x00, "Intel ICH10 (RAID)", 0},
    {0x3a228086, 0x00, "Intel ICH10", 0},
    {0x3a258086, 0x00, "Intel ICH10 (RAID)", 0},
    {0x3b228086, 0x00, "Intel Ibex Peak", 0},
    {0x3b238086, 0x00, "Intel Ibex Peak", 0},
    {0x3b258086, 0x00, "Intel Ibex Peak (RAID)", 0},
    {0x3b298086, 0x00, "Intel Ibex Peak-M", 0},
    {0x3b2c8086, 0x00, "Intel Ibex Peak-M (RAID)", 0},
    {0x3b2f8086, 0x00, "Intel Ibex Peak-M", 0},
    {0x06d68086, 0x00, "Intel Comet Lake (RAID)", 0},
    {0x19b08086, 0x00, "Intel Denverton", 0},
    {0x19b18086, 0x00, "Intel Denverton", 0},
    {0x19b28086, 0x00, "Intel Denverton", 0},
    {0x19b38086, 0x00, "Intel Denverton", 0},
    {0x19b48086, 0x00, "Intel Denverton", 0},
    {0x19b58086, 0x00, "Intel Denverton", 0},
    {0x19b68086, 0x00, "Intel Denverton", 0},
    {0x19b78086, 0x00, "Intel Denverton", 0},
    {0x19be8086, 0x00, "Intel Denverton", 0},
    {0x19bf8086, 0x00, "Intel Denverton", 0},
    {0x19c08086, 0x00, "Intel Denverton", 0},
    {0x19c18086, 0x00, "Intel Denverton", 0},
    {0x19c28086, 0x00, "Intel Denverton", 0},
    {0x19c38086, 0x00, "Intel Denverton", 0},
    {0x19c48086, 0x00, "Intel Denverton", 0},
    {0x19c58086, 0x00, "Intel Denverton", 0},
    {0x19c68086, 0x00, "Intel Denverton", 0},
    {0x19c78086, 0x00, "Intel Denverton", 0},
    {0x19ce8086, 0x00, "Intel Denverton", 0},
    {0x19cf8086, 0x00, "Intel Denverton", 0},
    {0x1c028086, 0x00, "Intel Cougar Point", 0},
    {0x1c038086, 0x00, "Intel Cougar Point", 0},
    {0x1c048086, 0x00, "Intel Cougar Point (RAID)", 0},
    {0x1c058086, 0x00, "Intel Cougar Point (RAID)", 0},
    {0x1c068086, 0x00, "Intel Cougar Point (RAID)", 0},
    {0x1d028086, 0x00, "Intel Patsburg", 0},
    {0x1d048086, 0x00, "Intel Patsburg", 0},
    {0x1d068086, 0x00, "Intel Patsburg", 0},
    {0x28268086, 0x00, "Intel Patsburg+ (RAID)", 0},
    {0x1e028086, 0x00, "Intel Panther Point", 0},
    {0x1e038086, 0x00, "Intel Panther Point", 0},
    {0x1e048086, 0x00, "Intel Panther Point (RAID)", 0},
    {0x1e058086, 0x00, "Intel Panther Point (RAID)", 0},
    {0x1e068086, 0x00, "Intel Panther Point (RAID)", 0},
    {0x1e078086, 0x00, "Intel Panther Point (RAID)", 0},
    {0x1e0e8086, 0x00, "Intel Panther Point (RAID)", 0},
    {0x1e0f8086, 0x00, "Intel Panther Point (RAID)", 0},
    {0x1f228086, 0x00, "Intel Avoton", 0},
    {0x1f238086, 0x00, "Intel Avoton", 0},
    {0x1f248086, 0x00, "Intel Avoton (RAID)", 0},
    {0x1f258086, 0x00, "Intel Avoton (RAID)", 0},
    {0x1f268086, 0x00, "Intel Avoton (RAID)", 0},
    {0x1f278086, 0x00, "Intel Avoton (RAID)", 0},
    {0x1f2e8086, 0x00, "Intel Avoton (RAID)", 0},
    {0x1f2f8086, 0x00, "Intel Avoton (RAID)", 0},
    {0x1f328086, 0x00, "Intel Avoton", 0},
    {0x1f338086, 0x00, "Intel Avoton", 0},
    {0x1f348086, 0x00, "Intel Avoton (RAID)", 0},
    {0x1f358086, 0x00, "Intel Avoton (RAID)", 0},
    {0x1f368086, 0x00, "Intel Avoton (RAID)", 0},
    {0x1f378086, 0x00, "Intel Avoton (RAID)", 0},
    {0x1f3e8086, 0x00, "Intel Avoton (RAID)", 0},
    {0x1f3f8086, 0x00, "Intel Avoton (RAID)", 0},
    {0x23a38086, 0x00, "Intel Coleto Creek", 0},
    {0x31e38086, 0x00, "Intel Gemini Lake", 0},
    {0x5ae38086, 0x00, "Intel Apollo Lake", 0},
    {0x7ae28086, 0x00, "Intel Alder Lake", 0},
    {0x8c028086, 0x00, "Intel Lynx Point", 0},
    {0x8c038086, 0x00, "Intel Lynx Point", 0},
    {0x8c048086, 0x00, "Intel Lynx Point (RAID)", 0},
    {0x8c058086, 0x00, "Intel Lynx Point (RAID)", 0},
    {0x8c068086, 0x00, "Intel Lynx Point (RAID)", 0},
    {0x8c078086, 0x00, "Intel Lynx Point (RAID)", 0},
    {0x8c0e8086, 0x00, "Intel Lynx Point (RAID)", 0},
    {0x8c0f8086, 0x00, "Intel Lynx Point (RAID)", 0},
    {0x8c828086, 0x00, "Intel Wildcat Point", 0},
    {0x8c838086, 0x00, "Intel Wildcat Point", 0},
    {0x8c848086, 0x00, "Intel Wildcat Point (RAID)", 0},
    {0x8c858086, 0x00, "Intel Wildcat Point (RAID)", 0},
    {0x8c868086, 0x00, "Intel Wildcat Point (RAID)", 0},
    {0x8c878086, 0x00, "Intel Wildcat Point (RAID)", 0},
    {0x8c8e8086, 0x00, "Intel Wildcat Point (RAID)", 0},
    {0x8c8f8086, 0x00, "Intel Wildcat Point (RAID)", 0},
    {0x8d028086, 0x00, "Intel Wellsburg", 0},
    {0x8d048086, 0x00, "Intel Wellsburg (RAID)", 0},
    {0x8d068086, 0x00, "Intel Wellsburg (RAID)", 0},
    {0x8d628086, 0x00, "Intel Wellsburg", 0},
    {0x8d648086, 0x00, "Intel Wellsburg (RAID)", 0},
    {0x8d668086, 0x00, "Intel Wellsburg (RAID)", 0},
    {0x8d6e8086, 0x00, "Intel Wellsburg (RAID)", 0},
    {0x28238086, 0x00, "Intel Wellsburg+ (RAID)", 0},
    {0x28278086, 0x00, "Intel Wellsburg+ (RAID)", 0},
    {0x9c028086, 0x00, "Intel Lynx Point-LP", 0},
    {0x9c038086, 0x00, "Intel Lynx Point-LP", 0},
    {0x9c048086, 0x00, "Intel Lynx Point-LP (RAID)", 0},
    {0x9c058086, 0x00, "Intel Lynx Point-LP (RAID)", 0},
    {0x9c068086, 0x00, "Intel Lynx Point-LP (RAID)", 0},
    {0x9c078086, 0x00, "Intel Lynx Point-LP (RAID)", 0},
    {0x9c0e8086, 0x00, "Intel Lynx Point-LP (RAID)", 0},
    {0x9c0f8086, 0x00, "Intel Lynx Point-LP (RAID)", 0},
    {0x9c838086, 0x00, "Intel Wildcat Point-LP", 0},
    {0x9c858086, 0x00, "Intel Wildcat Point-LP (RAID)", 0},
    {0x9c878086, 0x00, "Intel Wildcat Point-LP (RAID)", 0},
    {0x9c8f8086, 0x00, "Intel Wildcat Point-LP (RAID)", 0},
    {0x9d038086, 0x00, "Intel Sunrise Point-LP", 0},
    {0x9d058086, 0x00, "Intel Sunrise Point-LP (RAID)", 0},
    {0x9d078086, 0x00, "Intel Sunrise Point-LP (RAID)", 0},
    {0xa1028086, 0x00, "Intel Sunrise Point", 0},
    {0xa1038086, 0x00, "Intel Sunrise Point", 0},
    {0xa1058086, 0x00, "Intel Sunrise Point (RAID)", 0},
    {0xa1068086, 0x00, "Intel Sunrise Point (RAID)", 0},
    {0xa1078086, 0x00, "Intel Sunrise Point (RAID)", 0},
    {0xa10f8086, 0x00, "Intel Sunrise Point (RAID)", 0},
    {0xa1828086, 0x00, "Intel Lewisburg", 0},
    {0xa1868086, 0x00, "Intel Lewisburg (RAID)", 0},
    {0xa1d28086, 0x00, "Intel Lewisburg", 0},
    {0xa1d68086, 0x00, "Intel Lewisburg (RAID)", 0},
    {0xa2028086, 0x00, "Intel Lewisburg", 0},
    {0xa2068086, 0x00, "Intel Lewisburg (RAID)", 0},
    {0xa2528086, 0x00, "Intel Lewisburg", 0},
    {0xa2568086, 0x00, "Intel Lewisburg (RAID)", 0},
    {0xa2828086, 0x00, "Intel Union Point", 0},
    {0xa2868086, 0x00, "Intel Union Point (RAID)", 0},
    {0xa28e8086, 0x00, "Intel Union Point (RAID)", 0},
    {0xa3528086, 0x00, "Intel Cannon Lake", 0},
    {0xa3538086, 0x00, "Intel Cannon Lake", 0},
    {0x23238086, 0x00, "Intel DH89xxCC", 0},
    {0x2360197b, 0x00, "JMicron JMB360", 0},
    {0x2361197b, 0x00, "JMicron JMB361", AHCI_Q_NOFORCE | AHCI_Q_1CH},
    {0x2362197b, 0x00, "JMicron JMB362", 0},
    {0x2363197b, 0x00, "JMicron JMB363", AHCI_Q_NOFORCE},
    {0x2365197b, 0x00, "JMicron JMB365", AHCI_Q_NOFORCE},
    {0x2366197b, 0x00, "JMicron JMB366", AHCI_Q_NOFORCE},
    {0x2368197b, 0x00, "JMicron JMB368", AHCI_Q_NOFORCE},
    {0x2392197b, 0x00, "JMicron JMB388", AHCI_Q_IOMMU_BUSWIDE},
    {0x0585197b, 0x00, "JMicron JMB58x", 0},
    {0x01221c28, 0x00, "Lite-On Plextor M6E (Marvell 88SS9183)",
     AHCI_Q_IOMMU_BUSWIDE},
    {0x611111ab, 0x00, "Marvell 88SE6111",
     AHCI_Q_NOFORCE | AHCI_Q_NOPMP | AHCI_Q_1CH | AHCI_Q_EDGEIS},
    {0x612111ab, 0x00, "Marvell 88SE6121",
     AHCI_Q_NOFORCE | AHCI_Q_NOPMP | AHCI_Q_2CH | AHCI_Q_EDGEIS | AHCI_Q_NONCQ |
         AHCI_Q_NOCOUNT},
    {0x614111ab, 0x00, "Marvell 88SE6141",
     AHCI_Q_NOFORCE | AHCI_Q_NOPMP | AHCI_Q_4CH | AHCI_Q_EDGEIS | AHCI_Q_NONCQ |
         AHCI_Q_NOCOUNT},
    {0x614511ab, 0x00, "Marvell 88SE6145",
     AHCI_Q_NOFORCE | AHCI_Q_NOPMP | AHCI_Q_4CH | AHCI_Q_EDGEIS | AHCI_Q_NONCQ |
         AHCI_Q_NOCOUNT},
    {0x91201b4b, 0x00, "Marvell 88SE912x",
     AHCI_Q_EDGEIS | AHCI_Q_IOMMU_BUSWIDE},
    {0x91231b4b, 0x11, "Marvell 88SE912x",
     AHCI_Q_ALTSIG | AHCI_Q_IOMMU_BUSWIDE},
    {0x91231b4b, 0x00, "Marvell 88SE912x",
     AHCI_Q_EDGEIS | AHCI_Q_SATA2 | AHCI_Q_IOMMU_BUSWIDE},
    {0x91251b4b, 0x00, "Marvell 88SE9125", 0},
    {0x91281b4b, 0x00, "Marvell 88SE9128",
     AHCI_Q_ALTSIG | AHCI_Q_IOMMU_BUSWIDE},
    {0x91301b4b, 0x00, "Marvell 88SE9130",
     AHCI_Q_ALTSIG | AHCI_Q_IOMMU_BUSWIDE},
    {0x91701b4b, 0x00, "Marvell 88SE9170", AHCI_Q_IOMMU_BUSWIDE},
    {0x91721b4b, 0x00, "Marvell 88SE9172", AHCI_Q_IOMMU_BUSWIDE},
    {0x917a1b4b, 0x00, "Marvell 88SE917A", AHCI_Q_IOMMU_BUSWIDE},
    {0x91821b4b, 0x00, "Marvell 88SE9182", AHCI_Q_IOMMU_BUSWIDE},
    {0x91831b4b, 0x00, "Marvell 88SS9183", AHCI_Q_IOMMU_BUSWIDE},
    {0x91a01b4b, 0x00, "Marvell 88SE91Ax", AHCI_Q_IOMMU_BUSWIDE},
    {0x92151b4b, 0x00, "Marvell 88SE9215", 0},
    {0x92201b4b, 0x00, "Marvell 88SE9220",
     AHCI_Q_ALTSIG | AHCI_Q_IOMMU_BUSWIDE},
    {0x92301b4b, 0x00, "Marvell 88SE9230",
     AHCI_Q_ALTSIG | AHCI_Q_IOMMU_BUSWIDE | AHCI_Q_SLOWDEV},
    {0x92351b4b, 0x00, "Marvell 88SE9235", 0},
    {0x06201103, 0x00, "HighPoint RocketRAID 620", 0},
    {0x06201b4b, 0x00, "HighPoint RocketRAID 620", 0},
    {0x06221103, 0x00, "HighPoint RocketRAID 622", 0},
    {0x06221b4b, 0x00, "HighPoint RocketRAID 622", 0},
    {0x06401103, 0x00, "HighPoint RocketRAID 640", 0},
    {0x06401b4b, 0x00, "HighPoint RocketRAID 640", 0},
    {0x06441103, 0x00, "HighPoint RocketRAID 644", 0},
    {0x06441b4b, 0x00, "HighPoint RocketRAID 644", 0},
    {0x06411103, 0x00, "HighPoint RocketRAID 640L", 0},
    {0x06421103, 0x00, "HighPoint RocketRAID 642L", AHCI_Q_IOMMU_BUSWIDE},
    {0x06451103, 0x00, "HighPoint RocketRAID 644L", AHCI_Q_IOMMU_BUSWIDE},
    {0x044c10de, 0x00, "NVIDIA MCP65", AHCI_Q_NOAA},
    {0x044d10de, 0x00, "NVIDIA MCP65", AHCI_Q_NOAA},
    {0x044e10de, 0x00, "NVIDIA MCP65", AHCI_Q_NOAA},
    {0x044f10de, 0x00, "NVIDIA MCP65", AHCI_Q_NOAA},
    {0x045c10de, 0x00, "NVIDIA MCP65", AHCI_Q_NOAA},
    {0x045d10de, 0x00, "NVIDIA MCP65", AHCI_Q_NOAA},
    {0x045e10de, 0x00, "NVIDIA MCP65", AHCI_Q_NOAA},
    {0x045f10de, 0x00, "NVIDIA MCP65", AHCI_Q_NOAA},
    {0x055010de, 0x00, "NVIDIA MCP67", AHCI_Q_NOAA},
    {0x055110de, 0x00, "NVIDIA MCP67", AHCI_Q_NOAA},
    {0x055210de, 0x00, "NVIDIA MCP67", AHCI_Q_NOAA},
    {0x055310de, 0x00, "NVIDIA MCP67", AHCI_Q_NOAA},
    {0x055410de, 0x00, "NVIDIA MCP67", AHCI_Q_NOAA},
    {0x055510de, 0x00, "NVIDIA MCP67", AHCI_Q_NOAA},
    {0x055610de, 0x00, "NVIDIA MCP67", AHCI_Q_NOAA},
    {0x055710de, 0x00, "NVIDIA MCP67", AHCI_Q_NOAA},
    {0x055810de, 0x00, "NVIDIA MCP67", AHCI_Q_NOAA},
    {0x055910de, 0x00, "NVIDIA MCP67", AHCI_Q_NOAA},
    {0x055A10de, 0x00, "NVIDIA MCP67", AHCI_Q_NOAA},
    {0x055B10de, 0x00, "NVIDIA MCP67", AHCI_Q_NOAA},
    {0x058410de, 0x00, "NVIDIA MCP67", AHCI_Q_NOAA},
    {0x07f010de, 0x00, "NVIDIA MCP73", AHCI_Q_NOAA},
    {0x07f110de, 0x00, "NVIDIA MCP73", AHCI_Q_NOAA},
    {0x07f210de, 0x00, "NVIDIA MCP73", AHCI_Q_NOAA},
    {0x07f310de, 0x00, "NVIDIA MCP73", AHCI_Q_NOAA},
    {0x07f410de, 0x00, "NVIDIA MCP73", AHCI_Q_NOAA},
    {0x07f510de, 0x00, "NVIDIA MCP73", AHCI_Q_NOAA},
    {0x07f610de, 0x00, "NVIDIA MCP73", AHCI_Q_NOAA},
    {0x07f710de, 0x00, "NVIDIA MCP73", AHCI_Q_NOAA},
    {0x07f810de, 0x00, "NVIDIA MCP73", AHCI_Q_NOAA},
    {0x07f910de, 0x00, "NVIDIA MCP73", AHCI_Q_NOAA},
    {0x07fa10de, 0x00, "NVIDIA MCP73", AHCI_Q_NOAA},
    {0x07fb10de, 0x00, "NVIDIA MCP73", AHCI_Q_NOAA},
    {0x0ad010de, 0x00, "NVIDIA MCP77", AHCI_Q_NOAA},
    {0x0ad110de, 0x00, "NVIDIA MCP77", AHCI_Q_NOAA},
    {0x0ad210de, 0x00, "NVIDIA MCP77", AHCI_Q_NOAA},
    {0x0ad310de, 0x00, "NVIDIA MCP77", AHCI_Q_NOAA},
    {0x0ad410de, 0x00, "NVIDIA MCP77", AHCI_Q_NOAA},
    {0x0ad510de, 0x00, "NVIDIA MCP77", AHCI_Q_NOAA},
    {0x0ad610de, 0x00, "NVIDIA MCP77", AHCI_Q_NOAA},
    {0x0ad710de, 0x00, "NVIDIA MCP77", AHCI_Q_NOAA},
    {0x0ad810de, 0x00, "NVIDIA MCP77", AHCI_Q_NOAA},
    {0x0ad910de, 0x00, "NVIDIA MCP77", AHCI_Q_NOAA},
    {0x0ada10de, 0x00, "NVIDIA MCP77", AHCI_Q_NOAA},
    {0x0adb10de, 0x00, "NVIDIA MCP77", AHCI_Q_NOAA},
    {0x0ab410de, 0x00, "NVIDIA MCP79", AHCI_Q_NOAA},
    {0x0ab510de, 0x00, "NVIDIA MCP79", AHCI_Q_NOAA},
    {0x0ab610de, 0x00, "NVIDIA MCP79", AHCI_Q_NOAA},
    {0x0ab710de, 0x00, "NVIDIA MCP79", AHCI_Q_NOAA},
    {0x0ab810de, 0x00, "NVIDIA MCP79", AHCI_Q_NOAA},
    {0x0ab910de, 0x00, "NVIDIA MCP79", AHCI_Q_NOAA},
    {0x0aba10de, 0x00, "NVIDIA MCP79", AHCI_Q_NOAA},
    {0x0abb10de, 0x00, "NVIDIA MCP79", AHCI_Q_NOAA},
    {0x0abc10de, 0x00, "NVIDIA MCP79", AHCI_Q_NOAA},
    {0x0abd10de, 0x00, "NVIDIA MCP79", AHCI_Q_NOAA},
    {0x0abe10de, 0x00, "NVIDIA MCP79", AHCI_Q_NOAA},
    {0x0abf10de, 0x00, "NVIDIA MCP79", AHCI_Q_NOAA},
    {0x0d8410de, 0x00, "NVIDIA MCP89", AHCI_Q_NOAA},
    {0x0d8510de, 0x00, "NVIDIA MCP89", AHCI_Q_NOFORCE | AHCI_Q_NOAA},
    {0x0d8610de, 0x00, "NVIDIA MCP89", AHCI_Q_NOAA},
    {0x0d8710de, 0x00, "NVIDIA MCP89", AHCI_Q_NOAA},
    {0x0d8810de, 0x00, "NVIDIA MCP89", AHCI_Q_NOAA},
    {0x0d8910de, 0x00, "NVIDIA MCP89", AHCI_Q_NOAA},
    {0x0d8a10de, 0x00, "NVIDIA MCP89", AHCI_Q_NOAA},
    {0x0d8b10de, 0x00, "NVIDIA MCP89", AHCI_Q_NOAA},
    {0x0d8c10de, 0x00, "NVIDIA MCP89", AHCI_Q_NOAA},
    {0x0d8d10de, 0x00, "NVIDIA MCP89", AHCI_Q_NOAA},
    {0x0d8e10de, 0x00, "NVIDIA MCP89", AHCI_Q_NOAA},
    {0x0d8f10de, 0x00, "NVIDIA MCP89", AHCI_Q_NOAA},
    {0x3781105a, 0x00, "Promise TX8660", 0},
    {0x33491106, 0x00, "VIA VT8251", AHCI_Q_NOPMP | AHCI_Q_NONCQ},
    {0x62871106, 0x00, "VIA VT8251", AHCI_Q_NOPMP | AHCI_Q_NONCQ},
    {0x11841039, 0x00, "SiS 966", 0},
    {0x11851039, 0x00, "SiS 968", 0},
    {0x01861039, 0x00, "SiS 968", 0},
    {0xa01c177d, 0x00, "ThunderX", AHCI_Q_ABAR0 | AHCI_Q_1MSI},
    {0x00311c36, 0x00, "Annapurna",
     AHCI_Q_FORCE_PI | AHCI_Q_RESTORE_CAP | AHCI_Q_NOMSIX},
    {0x1600144d, 0x00, "Samsung", AHCI_Q_NOMSI},
    {0x07e015ad, 0x00, "VMware", 0},
    {0x00000000, 0x00, NULL, 0}};

#define ATA_DEV_BUSY 0x80
#define ATA_DEV_DRQ 0x08
#define ATA_CMD_READ_DMA_EX 0x25

#define HBA_PxCMD_ST 0x0001
#define HBA_PxCMD_FRE 0x0010
#define HBA_PxCMD_FR 0x4000
#define HBA_PxCMD_CR 0x8000

#define SATA_SIG_ATA 0x00000101   // SATA drive
#define SATA_SIG_ATAPI 0xEB140101 // SATAPI drive
#define SATA_SIG_SEMB 0xC33C0101  // Enclosure management bridge
#define SATA_SIG_PM 0x96690101    // Port multiplier

#define AHCI_BIOS_BUSY (1 << 4)
#define AHCI_BIOS_OWNED (1 << 0)
#define AHCI_OS_OWNED (1 << 1)

#define AHCI_DEV_NULL 0
#define AHCI_DEV_SATA 1
#define AHCI_DEV_SEMB 2
#define AHCI_DEV_PM 3
#define AHCI_DEV_SATAPI 4

#define HBA_PORT_IPM_ACTIVE 1
#define HBA_PORT_DET_PRESENT 3

#define HBA_PxIS_TFES (1 << 30)

typedef volatile struct tagHBA_PORT {
  uint32_t clb;       // 0x00, command list base address, 1K-byte aligned
  uint32_t clbu;      // 0x04, command list base address upper 32 bits
  uint32_t fb;        // 0x08, FIS base address, 256-byte aligned
  uint32_t fbu;       // 0x0C, FIS base address upper 32 bits
  uint32_t is;        // 0x10, interrupt status
  uint32_t ie;        // 0x14, interrupt enable
  uint32_t cmd;       // 0x18, command and status
  uint32_t rsv0;      // 0x1C, Reserved
  uint32_t tfd;       // 0x20, task file data
  uint32_t sig;       // 0x24, signature
  uint32_t ssts;      // 0x28, SATA status (SCR0:SStatus)
  uint32_t sctl;      // 0x2C, SATA control (SCR2:SControl)
  uint32_t serr;      // 0x30, SATA error (SCR1:SError)
  uint32_t sact;      // 0x34, SATA active (SCR3:SActive)
  uint32_t ci;        // 0x38, command issue
  uint32_t sntf;      // 0x3C, SATA notification (SCR4:SNotification)
  uint32_t fbs;       // 0x40, FIS-based switch control
  uint32_t rsv1[11];  // 0x44 ~ 0x6F, Reserved
  uint32_t vendor[4]; // 0x70 ~ 0x7F, vendor specific
} HBA_PORT;

typedef volatile struct tagHBA_MEM {
  // 0x00 - 0x2B, Generic Host Control
  uint32_t cap;     // 0x00, Host capability
  uint32_t ghc;     // 0x04, Global host control
  uint32_t is;      // 0x08, Interrupt status
  uint32_t pi;      // 0x0C, Port implemented
  uint32_t vs;      // 0x10, Version
  uint32_t ccc_ctl; // 0x14, Command completion coalescing control
  uint32_t ccc_pts; // 0x18, Command completion coalescing ports
  uint32_t em_loc;  // 0x1C, Enclosure management location
  uint32_t em_ctl;  // 0x20, Enclosure management control
  uint32_t cap2;    // 0x24, Host capabilities extended
  uint32_t bohc;    // 0x28, BIOS/OS handoff control and status

  // 0x2C - 0x9F, Reserved
  uint8_t rsv[0xA0 - 0x2C];

  // 0xA0 - 0xFF, Vendor specific registers
  uint8_t vendor[0x100 - 0xA0];

  // 0x100 - 0x10FF, Port control registers
  HBA_PORT ports[32]; // 1 ~ 32
} HBA_MEM;

typedef struct tagHBA_CMD_HEADER {
  // DW0
  uint8_t cfl : 5; // Command FIS length in DWORDS, 2 ~ 16
  uint8_t a : 1;   // ATAPI
  uint8_t w : 1;   // Write, 1: H2D, 0: D2H
  uint8_t p : 1;   // Prefetchable

  uint8_t r : 1;    // Reset
  uint8_t b : 1;    // BIST
  uint8_t c : 1;    // Clear busy upon R_OK
  uint8_t rsv0 : 1; // Reserved
  uint8_t pmp : 4;  // Port multiplier port

  uint16_t prdtl; // Physical region descriptor table length in entries

  // DW1
  volatile uint32_t prdbc; // Physical region descriptor byte count transferred

  // DW2, 3
  uint32_t ctba;  // Command table descriptor base address
  uint32_t ctbau; // Command table descriptor base address upper 32 bits

  // DW4 - 7
  uint32_t rsv1[4]; // Reserved
} HBA_CMD_HEADER;

typedef struct tagFIS_REG_H2D {
  // DWORD 0
  uint8_t fis_type; // FIS_TYPE_REG_H2D

  uint8_t pmport : 4; // Port multiplier
  uint8_t rsv0 : 3;   // Reserved
  uint8_t c : 1;      // 1: Command, 0: Control

  uint8_t command;  // Command register
  uint8_t featurel; // Feature register, 7:0

  // DWORD 1
  uint8_t lba0;   // LBA low register, 7:0
  uint8_t lba1;   // LBA mid register, 15:8
  uint8_t lba2;   // LBA high register, 23:16
  uint8_t device; // Device register

  // DWORD 2
  uint8_t lba3;     // LBA register, 31:24
  uint8_t lba4;     // LBA register, 39:32
  uint8_t lba5;     // LBA register, 47:40
  uint8_t featureh; // Feature register, 15:8

  // DWORD 3
  uint8_t countl;  // Count register, 7:0
  uint8_t counth;  // Count register, 15:8
  uint8_t icc;     // Isochronous command completion
  uint8_t control; // Control register

  // DWORD 4
  uint8_t rsv1[4]; // Reserved
} FIS_REG_H2D;

typedef struct tagHBA_PRDT_ENTRY {
  uint32_t dba;  // Data base address
  uint32_t dbau; // Data base address upper 32 bits
  uint32_t rsv0; // Reserved

  // DW3
  uint32_t dbc : 22; // Byte count, 4M max
  uint32_t rsv1 : 9; // Reserved
  uint32_t i : 1;    // Interrupt on completion
} HBA_PRDT_ENTRY;

typedef struct tagHBA_CMD_TBL {
  // 0x00
  uint8_t cfis[64]; // Command FIS

  // 0x40
  uint8_t acmd[16]; // ATAPI command, 12 or 16 bytes

  // 0x50
  uint8_t rsv[48]; // Reserved

  // 0x80
  HBA_PRDT_ENTRY
  prdt_entry[1]; // Physical region descriptor table entries, 0 ~ 65535
} HBA_CMD_TBL;

typedef enum {
  FIS_TYPE_REG_H2D = 0x27,   // Register FIS - host to device
  FIS_TYPE_REG_D2H = 0x34,   // Register FIS - device to host
  FIS_TYPE_DMA_ACT = 0x39,   // DMA activate FIS - device to host
  FIS_TYPE_DMA_SETUP = 0x41, // DMA setup FIS - bidirectional
  FIS_TYPE_DATA = 0x46,      // Data FIS - bidirectional
  FIS_TYPE_BIST = 0x58,      // BIST activate FIS - bidirectional
  FIS_TYPE_PIO_SETUP = 0x5F, // PIO setup FIS - device to host
  FIS_TYPE_DEV_BITS = 0xA1,  // Set device bits FIS - device to host
} FIS_TYPE;

typedef struct ahci ahci;

struct ahci {
  void        *clbVirt[32];
  void        *ctbaVirt[32];
  uint32_t     sata; // bitmap (32 ports -> 32 bits)
  AHCI_DEVICE *bsdInfo;
  HBA_MEM     *mem;
};

bool initiateAHCI(PCIdevice *device);
bool ahciRead(ahci *ahciPtr, uint32_t portId, HBA_PORT *port, uint32_t startl,
              uint32_t starth, uint32_t count, uint16_t *buf);

#endif
