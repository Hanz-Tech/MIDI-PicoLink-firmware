#ifndef SERIAL_UTILS_H
#define SERIAL_UTILS_H

#include <Arduino.h>

// Print to both Serial and Serial2
void dualPrint(const String &message);
void dualPrintln(const String &message);

// Print formatted to both Serial and Serial2
void dualPrintf(const char *format, ...);

#endif // SERIAL_UTILS_H
