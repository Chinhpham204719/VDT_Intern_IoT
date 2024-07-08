#include "top_config.h"

void setup() {
    Serial.begin(115200);
    pinMode(LED_PIN,OUTPUT);
    init_system();
}

void loop () {
  update_data_payload();
  // COM_monitor();
  thingsboard_communication();
  delay(2000);
}

void update_data_payload(){
	if(read_sensors() == true) {
    if(tb_data_valid == true) return;
  
    // Prepare a JSON payload  
  if(data_payload.charAt(0) != '{')
    data_payload = "{";

  data_payload += "\\\"temperature\\\":"; data_payload += String(temp); data_payload += ",";
  data_payload += "\\\"humidity\\\":"; data_payload += String(hum); 
  data_payload += ",";
  data_payload += "\\\"luminance\\\":"; data_payload += String(lux);
  data_payload += "}";
  tb_data_valid = true;
  }
}

void thingsboard_communication(){
  if(tb_data_valid == false) return;
  
  if(millis() - tb_send_timming > TB_DURATION){        
    send_data_to_tb("v1/devices/me/telemetry", 5000);
    request_control("getLightStatus", 5000);  //request control signal to TB
    tb_data_valid = false;
    tb_send_timming = millis();
  }
}

void init_system() {
    // init sensors
    init_sensors();

    // init nbiot module
    Serial.println("init NB-IOT");
    Nbmod.begin(115200, SERIAL_8N1, RXD, TXD);
    delay(5000);
    if (init_nbmod() != false) init_mqtt();
}

void COM_monitor(){  
  while (Nbmod.available()) {
    Serial.write(Nbmod.read());//Forward what Software Serial received to Serial Port
  }
}
