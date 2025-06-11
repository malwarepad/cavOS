#include "nic_controller.h"
#include "pci.h"
#include "types.h"

#ifndef E1000_H
#define E1000_H

#define E1000_RX_PAGE_COUNT 1
#define E1000_RX_LIST_ENTRIES                                                  \
  ((E1000_RX_PAGE_COUNT * PAGE_SIZE) / sizeof(struct E1000RX))

#define E1000_TX_PAGE_COUNT 1
#define E1000_TX_LIST_ENTRIES                                                  \
  ((E1000_TX_PAGE_COUNT * PAGE_SIZE) / sizeof(struct E1000TX))

typedef struct E1000RX {
  uint64_t addr;
  uint16_t length;
  uint16_t checksum; // or resv
  uint8_t  status;
  uint8_t  errors;
  uint16_t special; // or resv
} E1000RX;

typedef struct E1000TX {
  uint64_t addr;
  uint16_t length;
  uint8_t  checksumOffset;
  uint8_t  command;
  uint8_t  status;
  uint8_t  checksumStart;
  uint16_t special;
} E1000TX;

#define E1000RX_STATUS_DONE (1 << 0)
#define E1000RX_STATUS_END_OF_PACKET (1 << 1)
#define E1000RX_STATUS_IGNORE_CSUM (1 << 2)
#define E1000RX_STATUS_IS_8021Q (1 << 3)
#define E1000RX_STATUS_TCP_CSUM (1 << 5)
#define E1000RX_STATUS_IP_CSUM (1 << 6)
#define E1000RX_STATUS_FILTER (1 << 7)

typedef struct E1000_interface {
  size_t iobase;
  size_t membasePhys;
  size_t membase;

  uint16_t deviceId;
  NIC     *nic;

  E1000RX *rxList;
  uint32_t rxHead;

  E1000TX *txList;
  uint32_t txHead;

  bool eeprom;
} E1000_interface;

// Some EEPROM registers
#define REG_EECD 0x0010
#define REG_EEPROM 0x0014

// Receive configuration
#define REG_RX_CONTROL 0x0100
#define RCTL_ENABLE (1 << 1)
#define RCTL_STORE_BAD_PACKETS (1 << 2)
#define RCTL_UNICAST_PROMISCUOUS_ENABLED (1 << 3)
#define RCTL_MULTICAST_PROMISCUOUS_ENABLED (1 << 4)
#define RCTL_LONG_PACKET_RECEPTION_ENABLE (1 << 5)
#define RCTL_LOOPBACK_MODE_MASK (0b11 << 6)
#define RCTL_LOOPBACK_MODE_OFF (0b00 << 6)
#define RCTL_LOOPBACK_MODE_ON (0b11 << 6)
#define RCTL_DESC_MIN_THRESHOLD_SIZE_MASK (0b11 << 8)
#define RCTL_DESC_MIN_THRESHOLD_SIZE_HALF (0b00 << 8)
#define RCTL_DESC_MIN_THRESHOLD_SIZE_FOURTH (0b01 << 8)
#define RCTL_DESC_MIN_THRESHOLD_SIZE_EIGHTH (0b10 << 8)
#define RCTL_MULTICAST_OFFSET_MASK (0b11 << 12)
#define RCTL_MULTICAST_OFFSET_47_36 (0b00 << 12)
#define RCTL_MULTICAST_OFFSET_46_35 (0b01 << 12)
#define RCTL_MULTICAST_OFFSET_45_34 (0b10 << 12)
#define RCTL_MULTICAST_OFFSET_43_32 (0b11 << 12)
#define RCTL_BROADCAST_ACCEPT_MODE (1 << 15)
#define RCTL_BUFFER_SIZE_MASK (0b11 << 16)
#define RCTL_BUFFER_SIZE_2048 (0b00 << 16)
#define RCTL_BUFFER_SIZE_1024 (0b01 << 16)
#define RCTL_BUFFER_SIZE_512 (0b10 << 16)
#define RCTL_BUFFER_SIZE_256 (0b11 << 16)
#define RCTL_BUFFER_SIZE_16384 ((0b01 << 16) & (1 << 25))
#define RCTL_BUFFER_SIZE_8192 ((0b10 << 16) & (1 << 25))
#define RCTL_BUFFER_SIZE_4096 ((0b11 << 16) & (1 << 25))
#define RCTL_VLAN_FILTER_ENABLE (1 << 18)
#define RCTL_CANONICAL_FORM_INDICATOR_ENABLE (1 << 19)
#define RCTL_CANONICAL_FORM_INDICATOR_VALUE (1 << 20)
#define RCTL_DISCARD_PAUSE_FRAMES (1 << 22)
#define RCTL_PASS_MAC_CONTROL_FRAMES (1 << 23)
#define RCTL_BUFFER_SIZE_EXTEND (1 << 25)
#define RCTL_STRIP_ETHERNET_CRC (1 << 26)

// Receive buffer
#define REG_RXDESCLO 0x2800
#define REG_RXDESCHI 0x2804
#define REG_RXDESCLEN 0x2808
#define REG_RXDESCHEAD 0x2810
#define REG_RXDESCTAIL 0x2818

// Transport buffer
#define REG_TXDESCLO 0x3800
#define REG_TXDESCHI 0x3804
#define REG_TXDESCLEN 0x3808
#define REG_TXDESCHEAD 0x3810
#define REG_TXDESCTAIL 0x3818

// Generic configuration register
#define REG_CTRL 0x0000
#define CTRL_FULL_DUPLEX (1 << 0)
#define CTRL_LINK_RESET (1 << 3)
#define CTRL_AUTO_SPEED_DETECTION_ENABLE (1 << 5)
#define CTRL_SET_LINK_UP (1 << 6)
#define CTRL_INVERT_LOSS_OF_SIGNAL (1 << 7)
#define CTRL_FORCE_SPEED_10MBS (0b00 << 8)
#define CTRL_FORCE_SPEED_100MBS (0b11 << 8)
#define CTRL_FORCE_SPEED_1000MBS (0b10 << 8)
#define CTRL_FORCE_SPEED (1 << 11)
#define CTRL_FORCE_DUPLEX (1 << 12)
#define CTRL_SDP0_DATA (1 << 18)
#define CTRL_SDP1_DATA (1 << 19)
#define CTRL_D3COLD_WAKEUP_CAPABILITY_ADVERTISEMENT_ENABLE (1 << 20)
#define CTRL_PHY_POWER_MANAGEMENT_ENABLE (1 << 21)
#define CTRL_SDP0_PIN_DIRECTIONALITY (1 << 22)
#define CTRL_SDP1_PIN_DIRECTIONALITY (1 << 23)
#define CTRL_DEVICE_RESET (1 << 26)
#define CTRL_RECEIVE_FLOW_CONTROL_ENABLE (1 << 27)
#define CTRL_TRANSMIT_FLOW_CONTROL_ENABLE (1 << 28)
#define CTRL_VLAN_MODE_ENABLE (1 << 30)
#define CTRL_PHY_RESET (1 << 31)

// What MAC address do we want to assosiate w/the interface?
#define REG_RAL_BEGIN 0x5400
#define REG_RAL_END 0x5478

// EEPROM operations
#define EERD_START (1 << 0)
#define EERD_DONE (1 << 4)
#define EERD_DONE_EXTRA (1 << 1)

#define EEPROM_ETHERNET_ADDRESS_BYTES0 0x0
#define EEPROM_ETHERNET_ADDRESS_BYTES1 0x1
#define EEPROM_ETHERNET_ADDRESS_BYTES2 0x2

#define EECD_FLASH_WRITE_DISABLED (0b01 << 4)
#define EECD_FLASH_WRITE_ENABLED (0b10 << 4)
#define EECD_FLASH_WRITE_MASK (0b11 << 4)
#define EECD_EEPROM_REQUEST (1 << 6)
#define EECD_EEPROM_GRANT (1 << 7)
#define EECD_EEPROM_PRESENT (1 << 8)
#define EECD_EEPROM_SIZE (1 << 9)
#define EECD_EEPROM_TYPE (1 << 13)

// Interrupt status register
#define REG_ICR 0x00c0
#define ICR_TX_DESC_WRITTEN_BACK (1 << 0)
#define ICR_TX_QUEUE_EMPTY (1 << 1)
#define ICR_LINK_STATUS_CHANGE (1 << 2)
#define ICR_RX_SEQUENCE_ERROR (1 << 3)
#define ICR_RX_DESC_MIN_THRESHOLD_HIT (1 << 4)
#define ICR_RX_OVERRUN (1 << 6)
#define ICR_RX_TIMER_INTERRUPT (1 << 7)
#define ICR_MDIO_ACCESS_COMLETE (1 << 9)
#define ICR_RX_CONFIGURATION (1 << 10)
#define ICR_TX_DESC_MIN_THRESHOLD_HIT (1 << 15)
#define ICR_SMALL_RECEIVE_PACKET (1 << 16)

// Interrupt configuration mask
#define REG_IMASK_CLEAR 0x00d8
#define REG_IMASK 0x00d0
#define IMASK_TX_DESC_WRITTEN_BACK (1 << 0)
#define IMASK_TX_QUEUE_EMPTY (1 << 1)
#define IMASK_LINK_STATUS_CHANGE (1 << 2)
#define IMASK_RX_SEQUENCE_ERROR (1 << 3)
#define IMASK_RX_DESC_MIN_THRESHOLD_HIT (1 << 4)
#define IMASK_RX_OVERRUN (1 << 6)
#define IMASK_RX_TIMER_INTERRUPT (1 << 7)
#define IMASK_MDIO_ACCESS_COMPLETE (1 << 9)
#define IMASK_RX_C_ORDERED_SETS (1 << 10)
#define IMASK_PHY_INTERRUPT (1 << 12)
#define IMASK_TX_DESC_MIN_THRESHOLD_HIT (1 << 15)
#define IMASK_RX_SMALL_PACKET_DETECTION (1 << 16)

// Transmission commands
#define CMD_EOP (1 << 0)
#define CMD_IFCS (1 << 1)
#define CMD_IC (1 << 2)
#define CMD_RS (1 << 3)
#define CMD_RPS (1 << 4)
#define CMD_VLE (1 << 6)
#define CMD_IDE (1 << 7)

// Transmission configuration
#define REG_TCTL 0x0400
#define TCTL_EN (1 << 1)

bool initiateE1000(PCIdevice *device);
void sendE1000(NIC *nic, void *packet, uint32_t packetSize);

#endif
