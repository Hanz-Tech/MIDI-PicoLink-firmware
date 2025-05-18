#include "serial_utils.h"
#include <stdarg.h>


bool debug = true;

// Print to both Serial and Serial2
void dualPrint(const String &message) {
    if (debug) {
        Serial.print(message);
        Serial2.print(message);
    }
}

void dualPrintln(const String &message) {
    if (debug) {
        Serial.println(message);
        Serial2.println(message);
    }
}

// Print formatted to both Serial and Serial2
void dualPrintf(const char *format, ...) {
    if (debug) {
        char buffer[256]; // Buffer for formatted string
        va_list args;
        
        // Format the string
        va_start(args, format);
        vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);
        
        // Print to both Serial and Serial2
        Serial.print(buffer);
        Serial2.print(buffer);
    }
}
