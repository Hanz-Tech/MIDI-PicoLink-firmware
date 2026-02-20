#include <cstdint>  // Fix for uint8_t in pio_usb_configuration.h
#include "pio_usb_configuration.h"
#include <Arduino.h>
#include <MIDI.h>
#include "usb_host_wrapper.h"
#include "usb_host_midi_handlers.h"
#include "usb_device_midi_handlers.h"
#include "pio_usb.h"
#include "led_utils.h"
#include "midi_instances.h"
#include "midi_router.h"

#include "serial_midi_handler.h"
#include "midi_filters.h"
#include "serial_utils.h"
#include "web_serial_config.h"
#include "config.h"
#include "imu_handler.h"
#include "pin_config.h"
extern volatile uint8_t midi_dev_addr;
extern volatile bool midi_host_mounted;

volatile bool isConnectedToComputer = false;

Adafruit_USBH_Host USBHost;

volatile bool core1_booting = true;
uint32_t timeout = 2000; // 2 seconds timeout

void setup() {
  TinyUSBDevice.setID(0x239A, 0x8122);  // Use Adafruit's official VID/PID for MIDI
  TinyUSBDevice.setManufacturerDescriptor("HanzTech");
  TinyUSBDevice.setProductDescriptor("MIDI PicoLink");
  TinyUSBDevice.setSerialDescriptor("PicoLink-001");
  
  Serial.begin(115200);
  Serial2.setRX(DEBUG_UART_RX_PIN);
  Serial2.setTX(DEBUG_UART_TX_PIN);
  Serial2.begin(115200);

  dualPrintln("DEBUG: Entered setup()");
  Serial2.println("DEBUG: Core0 start Serial2");
  pinMode(LED_IN_PIN, OUTPUT);
  pinMode(LED_OUT_PIN, OUTPUT);
  digitalWrite(LED_IN_PIN, LOW);
  digitalWrite(LED_OUT_PIN, LOW);
  initLEDs();

  usb_midi.setStringDescriptor("MIDI PicoLink");
  usb_midi.begin();

  USB_D.begin(MIDI_CHANNEL_OMNI);
  setupUsbDeviceHandlers();
  setupUsbHostHandlers();
  uint32_t startTime = millis();
  
  while(!TinyUSBDevice.mounted()) {
    delay(1);
    if(millis() - startTime > timeout) {
      dualPrintln("USB device not mounted after timeout - likely connected to power bank");
      break;
    }
  }
  isConnectedToComputer = TinyUSBDevice.mounted();
  
  dualPrintln("RP2040 USB MIDI Router - Main Sketch");
  if(isConnectedToComputer) {
    dualPrintln("Connected to computer - USB Device mode active");
  } else {
    dualPrintln("Connected to power source only - Running in standalone mode");
  }

  setupMidiFilters();
  enableAllChannels();
  loadConfigFromEEPROM();
  
  if (setupIMU()) {
    dualPrintln("IMU initialized successfully");
  } else {
    dualPrintln("IMU initialization failed or not enabled");
  }
  
  setupSerialMidi();

  rp2040.fifo.push(0);
  while(rp2040.fifo.pop() != 1){};
  USB_D.turnThruOff();
  dualPrintln("Core0 setup complete");
  dualPrintln("");
  blinkBothLEDs(4, 100);
}

void loop() {
  if (isConnectedToComputer) {
    USB_D.read();
  }
  loopSerialMidi(); 
  processWebSerialConfig();
  handleDelayedEEPROMSave();
  loopIMU();
  handleLEDs();
}

void setup1() {
  while(rp2040.fifo.pop() != 0){};
  if (!isConnectedToComputer) {
    dualPrintln("Core1 setup in standalone mode");
  } else {
    uint32_t startTime = millis();
    while(!Serial && (millis() - startTime < timeout)); // 2 second timeout
  }
  uint32_t cpu_hz = clock_get_hz(clk_sys);
  if (cpu_hz != 120000000UL && cpu_hz != 240000000UL) {
    delay(2000);   // wait for native usb
    dualPrintf("Error: CPU Clock = %u, PIO USB require CPU clock must be multiple of 120 Mhz\r\n", cpu_hz);
    dualPrintf("Change your CPU Clock to either 120 or 240 Mhz in Menu->CPU Speed\r\n");
    while(1) delay(1);
  }
  pio_usb_configuration_t pio_cfg = PIO_USB_DEFAULT_CONFIG;
  pio_cfg.pin_dp = HOST_PIN_DP;

  #if defined(ARDUINO_RASPBERRY_PI_PICO_W)
    /* Need to swap PIOs so PIO code from CYW43 PIO SPI driver will fit */
    pio_cfg.pio_rx_num = 0;
    pio_cfg.pio_tx_num = 1;
  #endif

  dualPrintf("Core1: Configuring PIO USB with DP pin %u\r\n", HOST_PIN_DP);
  USBHost.configure_pio_usb(1, &pio_cfg);

  dualPrintf("Core1: Starting USB Host...\r\n");
  bool host_init_success = USBHost.begin(1);
  if (host_init_success) {
    dualPrintf("Core1: USB Host initialized successfully\r\n");
  } else {
    dualPrintf("Core1: USB Host initialization FAILED!\r\n");
  }
  
  delay(100);
  
  rp2040.fifo.push(1);
  dualPrintln("Core1 setup to run TinyUSB host with pio-usb");
  dualPrintln("");
}

void loop1() {
  usb_host_wrapper_task();
}
