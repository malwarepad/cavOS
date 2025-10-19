#include <acpi.h>
#include <linux.h>
#include <rtc.h>
#include <system.h>
#include <uacpi/uacpi.h>

#include <uacpi/sleep.h>

// uACPI entry point & utilities
// Copyright (C) 2024 Panagiotis

void initiateACPI() {
  uacpi_status ret = uacpi_initialize(0);
  if (uacpi_unlikely_error(ret)) {
    debugf("uacpi_initialize error: %s", uacpi_status_to_string(ret));
    panic();
  }

  /*
   * Load the AML namespace. This feeds DSDT and all SSDTs to the interpreter
   * for execution.
   */
  ret = uacpi_namespace_load();
  if (uacpi_unlikely_error(ret)) {
    debugf("uacpi_namespace_load error: %s", uacpi_status_to_string(ret));
    panic();
  }

  // set the interrupt model
  uacpi_set_interrupt_model(UACPI_INTERRUPT_MODEL_IOAPIC);

  /*
   * Initialize the namespace. This calls all necessary _STA/_INI AML methods,
   * as well as _REG for registered operation region handlers.
   */
  ret = uacpi_namespace_initialize();
  if (uacpi_unlikely_error(ret)) {
    debugf("uacpi_namespace_initialize error: %s", uacpi_status_to_string(ret));
    panic();
  }

  // use the fadt for getting the century register (if it exists)
  struct acpi_fadt *fadt = 0;
  if (!uacpi_unlikely_error(uacpi_table_fadt(&fadt))) {
    century_register = fadt->century;
    if (century_register)
      debugf("[acpi::info] Found century register for RTC: register{0x%lx}\n",
             century_register);
  }
}

size_t acpiPoweroff() {
  uacpi_status ret = uacpi_prepare_for_sleep_state(UACPI_SLEEP_STATE_S5);
  if (uacpi_unlikely_error(ret)) {
    debugf("[acpi] Couldn't prepare for poweroff: %s\n",
           uacpi_status_to_string(ret));
    return ERR(EIO);
  }

  asm volatile("cli");
  uacpi_status retPoweroff = uacpi_enter_sleep_state(UACPI_SLEEP_STATE_S5);
  if (uacpi_unlikely_error(retPoweroff)) {
    asm volatile("sti");
    debugf("[acpi] Couldn't power off the system: %s\n",
           uacpi_status_to_string(retPoweroff));
    return ERR(EIO);
  }

  debugf("[acpi] Shouldn't be reached after power off!\n");
  panic();
  return 0;
}

size_t acpiReboot() {
  uacpi_prepare_for_sleep_state(UACPI_SLEEP_STATE_S5);

  uacpi_status ret = uacpi_reboot();
  if (uacpi_unlikely_error(ret)) {
    debugf("[acpi] Couldn't restart the system: %s\n",
           uacpi_status_to_string(ret));
    return ERR(EIO);
  }

  debugf("[acpi] Shouldn't be reached after reboot!\n");
  panic();
  return 0;
}
