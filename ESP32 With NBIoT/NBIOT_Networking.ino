#include <ArduinoJson.h>

char send_atcmd (const char* ATcmd, const char* expectResp, unsigned int timeout) {
    digitalWrite(PWR, LOW);
    delay(300);
    digitalWrite(PWR, HIGH);
    
    atResponse = AT_RESP_NONE;
    rx_len = 0;
    comm_buff[rx_len] = 0;

    if(ATcmd != ""){
        while (Nbmod.available() > 0) Nbmod.read();
        Nbmod.println(ATcmd);// gui luon
    }

    prev_time = millis();
    do {
        if (Nbmod.available() != 0) {
            atResponse = AT_RESP_ERR;

            // read buffer for answer
            comm_buff[rx_len] = Nbmod.read();

            rx_len++;
            comm_buff[rx_len] = 0;// lúc in ra sẽ không bị các ký tự khác nhảy vào

            // check for desired response in buffer
            if (expectResp != "") {
                if (strstr((char *)comm_buff, expectResp) != NULL) {
                    atResponse = AT_RESP_OK;
                    break;
                }
            }
            else atResponse = AT_RESP_OK;
        }
    }
    while ((millis() - prev_time) < timeout);

#if (SW_DEBUG)
  if (ATcmd != "");
    if (atResponse == AT_RESP_OK){
    for(int i =0; i<rx_len; i++)
      Serial.printf("%c", comm_buff[i]);
    Serial.println("");}
#endif
    return atResponse;
}




bool init_nbmod() {
    pinMode(PWR, OUTPUT);
    digitalWrite(PWR, HIGH);
    char respond_code;
    int i; // generic index name
    nbModStatus = false;

    // check module status. max tries = 5
    for (i = 0; i < 5; i++) {
        respond_code = send_atcmd("AT", "OK", 2000);
        if (respond_code != AT_RESP_OK) {
            delay(1000);
            continue;
        }
        else break;
    }
    if (respond_code != AT_RESP_OK) {
        Serial.println("Module unavailable");
        nbModStatus = false;
        return false;
    }

    // Report signal quality
  for (i = 0; i < 5; i++) {
    respond_code = send_atcmd("AT+CSQ", "OK", 2000);
    if(respond_code != AT_RESP_OK){
      delay(1000);
      continue;
    }
    else break;      
  }  
  if(respond_code != AT_RESP_OK){
    Serial.println("Cmd AT_CSQ error");
    return false;
  }

  int start_time = millis();
  do {
    respond_code = send_atcmd("AT+CEREG?", "CEREG: 0,1", 2000);
  }
  while(respond_code != AT_RESP_OK);

  if(respond_code != AT_RESP_OK){
    Serial.println("Cmd AT_CEREG error");
    return false;
  }

  return true;
  // add more command below here i forgor
}

bool init_mqtt() {
    char respond_code;
    int i; // generic index name
    nbModStatus = false;

    // New mqtt server
    String at_newmqtt = "AT+CMQNEW=\"" + tbServer + "\"," + "\"" + tbPort + "\"," + "10000,1024";
    // Serial.println(at_newmqtt);
    for (i = 0; i < 5; i++) {
      respond_code = send_atcmd(at_newmqtt.c_str(), "OK", 2000);
      if (respond_code != AT_RESP_OK) {
          delay(5000);
          continue;
      }
      else break;
    }
    if (respond_code != AT_RESP_OK) {
      Serial.println("Failed to connect to Thingsboard!");
      return false;
    }  
    
    //Form the connection 
    String at_conmqtt = "AT+CMQCON=0,3,\"espnbclient\",600,1,0,\"" + TB_DEVICE_KEY + "\",NULL";
    // Serial.println(at_conmqtt);
    for (i = 0; i < 5; i++) {
      respond_code = send_atcmd(at_conmqtt.c_str(), "OK", 2000);
      if (respond_code != AT_RESP_OK) {
          delay(5000);
          continue;
      }
      else break;
    }
    if (respond_code != AT_RESP_OK) {
      Serial.println("Failed to establish MQTT connection!");
      return false;
    }


    //Subcribe to MQTT
    String at_submqtt = "AT+CMQSUB=0," + String("\"v1/devices/me/rpc/response/+\",1") ;
    for (i = 0; i<5; i++){
      respond_code = send_atcmd(at_submqtt.c_str(),"OK",2000);
      if (respond_code != AT_RESP_OK) {
          Serial.println("Msg received!");
          delay(2000);
          continue;
      }
      else break;
      }
    if (respond_code != AT_RESP_OK) {
      Serial.println("Failed to establish MQTT subscription!");
      return false;
    }
    return true;
}

void send_data_to_tb(char* topic, int time_out){
  // check for connection
  char respond_code = AT_RESP_ERR;
  respond_code = send_atcmd("AT+CMQCON?", tbServer.c_str(), 2000);
  if (respond_code != AT_RESP_OK) {
        delay(1000);
        init_mqtt();
    }

  // send
  String at_pubmsg = "AT+CMQPUB=0,\"" + String(topic) + "\",1,0,0," + String(data_payload.length()-6) + ",\"" + data_payload + "\"";
  // Serial.println(at_pubmsg);
  for (int i = 0; i < 5; i++) {
    respond_code = send_atcmd(at_pubmsg.c_str(), "OK", 2000);
    if (respond_code != AT_RESP_OK) {
        delay(5000);
        continue;
    }
    else break;
  }
  data_payload = "";
  if (respond_code != AT_RESP_OK) {
    Serial.println("Failed to send msg!");
  }
  else 
    Serial.println("Msg sent!");
}


void switch_state_led(unsigned int timeout){
  prev_time = millis();
  do
  {
    if(Nbmod.available()){
    String switch_state;
    String payload = Nbmod.readStringUntil('}') + "}";
    payload = payload.substring(payload.indexOf('{'));
    unsigned int length  = payload.length(); 
    Serial.print("Check:");
    Serial.println(payload);
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, payload);

    if(doc.containsKey("method")){
      Serial.print("Nhận được bản tin:");
      switch_state = String(doc["params"]);
      if(switch_state.indexOf("false") != -1){
        Serial.println(switch_state);
        digitalWrite(LED_PIN, LOW);
      }
      else if(switch_state.indexOf("true") != -1)
        Serial.println(switch_state);
        digitalWrite(LED_PIN, HIGH);
    }
    }
    delay(1000);
  } while (millis() - prev_time < timeout); 
}

void request_control(char * method, unsigned int timeout){
    char respond_code;
    unsigned long lasttime = millis();
    String method_request = "{\\\"method\\\":\\\""+ String(method)+ "\\\"" + "}";
    String at_pubmqtt = "AT+CMQPUB=0," + String("\"v1/devices/me/rpc/request/1\",1,0,0,") + String(method_request.length()-4)+ ",\"" + method_request + "\"";
    do{
      for (int i = 0; i<5; i++){
        respond_code = send_atcmd(at_pubmqtt.c_str(),"OK", 2000);
        if (respond_code != AT_RESP_OK) {
            delay(2000);
        }
        else {
          switch_state_led(2000);
          break;
        }
        }
        if (respond_code != AT_RESP_OK) {
          Serial.println("Failed to establish MQTT connection!");
      }
    }while(millis() - lasttime<timeout);
}


