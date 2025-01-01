#include <system.h>
#include <uacpi/uacpi.h>

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

  /*
   * Initialize the namespace. This calls all necessary _STA/_INI AML methods,
   * as well as _REG for registered operation region handlers.
   */
  ret = uacpi_namespace_initialize();
  if (uacpi_unlikely_error(ret)) {
    debugf("uacpi_namespace_initialize error: %s", uacpi_status_to_string(ret));
    panic();
  }
}
