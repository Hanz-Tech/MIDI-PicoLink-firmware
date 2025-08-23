#ifndef WEB_SERIAL_CONFIG_H
#define WEB_SERIAL_CONFIG_H

// Call this regularly in the main loop to process config JSON commands from USB Serial
void processWebSerialConfig();

// Call this regularly in the main loop to handle delayed EEPROM saves
void handleDelayedEEPROMSave();

#endif // WEB_SERIAL_CONFIG_H
