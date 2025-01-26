#include "types.h"

#include "uacpi/acpi.h"
#include "uacpi/resources.h"
#include "uacpi/tables.h"
#include "uacpi/uacpi.h"
#include "uacpi/utilities.h"

#ifndef ACPI_H
#define ACPI_H

struct acpi_madt *madt;

void initiateACPI();

#endif