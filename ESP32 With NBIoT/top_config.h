#include "sensors_config.h"
// NB-IOT Module interface (UART)
#define RXD 17 // PH
#define TXD 16 // PH
#define PWR 5 // Nbmod wakeup pin

#define SW_DEBUG 1
HardwareSerial Nbmod(2);

bool nbModStatus = false;
bool MQTTStatus = false;

uint8_t comm_buff[512];      // buffer array for data receive over serial port
unsigned int rx_len = 0;    // counter for buffer array   
unsigned long prev_time;
uint8_t atResponse;

enum at_resp_enum {
  AT_RESP_NONE = -1,  // nothing received
  AT_RESP_ERR = 0,  // response_string is different from the response
  AT_RESP_OK = 1,   // response_string was included in the response
};

// Thingsboard (TB or tb) stuffs
String TB_DEVICE_KEY = ""; //Token
#define TB_DURATION 10000 

String tbServer = "demo.thingsboard.io";
String tbPort = "1883";
String data_payload = "";

unsigned long tb_send_timming;
bool tb_data_valid = false;



