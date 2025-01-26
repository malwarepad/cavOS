#include <apic.h>
#include <isr.h>
#include <rtc.h>
#include <schedule.h>
#include <system.h>
#include <timer.h>

void initiatePitTimer(uint32_t reload_value) {
  // outportb(0x43, 0b00110100);
  // outportb(0x40, 1193 & 0xFF); // low-byte
  // outportb(0x40, 1193 >> 8);   // high-byte

  timerFrequency = TIMER_ACCURANCY / reload_value;

  outportb(0x43, 0b00110100);

  uint8_t l = (uint8_t)(timerFrequency & 0xFF);
  uint8_t h = (uint8_t)(timerFrequency >> 8 & 0xFF);

  outportb(0x40, l);
  outportb(0x40, h);

  RTC rtc = {0};
  readFromCMOS(&rtc);
  timerTicks = 0;

  timerBootUnix = rtcToUnix(&rtc);
  debugf("[timer] Ready to fire: frequency{%dMHz}\n", timerFrequency);
}

void timerTick(uint64_t rsp) {
  timerTicks++;
  schedule(rsp);
}

uint32_t sleep(uint32_t time) {
  uint64_t target = timerTicks + (time);
  while (target > timerTicks)
    handControl();

  return 0;
}

void initiateApicTimer() {
  size_t waitfor = 10; // in ms
  initiatePitTimer(1000);
  uint8_t ioapicInt = ioApicRedirect(0, false);
  registerIRQhandler(ioapicInt, timerTick);

  apicWrite(APIC_REGISTER_TIMER_DIV, 0x3); // div 16
  apicWrite(APIC_REGISTER_TIMER_INITCNT, 0xFFFFFFFF);

  // wait using the old pit
  uint64_t target = timerTicks + waitfor;
  while (target > timerTicks)
    ;

  // mask apic timer and get back time passed
  apicWrite(APIC_REGISTER_LVT_TIMER, 0x10000);
  uint32_t ticksInXms = 0xFFFFFFFF - apicRead(APIC_REGISTER_TIMER_CURRCNT);

  uint32_t lapicId = 0;
  uint8_t  targIrq = irqPerCoreAllocate(0, &lapicId);
  apicFreq = ticksInXms / waitfor;

  // finally configure the it
  apicWrite(APIC_REGISTER_LVT_TIMER, targIrq | APIC_LVT_TIMER_MODE_PERIODIC);
  apicWrite(APIC_REGISTER_TIMER_DIV, 0x3);
  apicWrite(APIC_REGISTER_TIMER_INITCNT, apicFreq);
  ioapicInt = ioApicRedirect(0, true); // mask the old pit
  registerIRQhandler(targIrq, timerTick);
}
