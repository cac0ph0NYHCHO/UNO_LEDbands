#include "WS_WIFI.h"

// The name and password of the WiFi access point
const char *ssid = STASSID;                
const char *password = STAPSK;               

char ipStr[16];
WebServer server(80);         
bool WIFI_Connection = 0;                                 

void handleRoot() {
  String myhtmlPage =
    String("") +
    "<html>"+
    "<head>"+
    "    <meta charset=\"utf-8\">"+
    "    <title>ESP32-S3-POE-ETH-8DI-8DO</title>"+
    "    <style>" +
    "        body {" +
    "            font-family: Arial, sans-serif;" +
    "            background-color: #f0f0f0;" +
    "            margin: 0;" +
    "            padding: 0;" +
    "        }" +
    "        .header {" +
    "            text-align: center;" +
    "            padding: 20px 0;" +
    "            background-color: #333;" +
    "            color: #fff;" +
    "            margin-bottom: 20px;" +
    "        }" +
    "        .container {" +
    "            max-width: 600px;" +
    "            margin: 0 auto;" +
    "            padding: 20px;" +
    "            background-color: #fff;" +
    "            border-radius: 5px;" +
    "            box-shadow: 0 0 5px rgba(0, 0, 0, 0.3);" +
    "        }" +
    "        .input-container {" +
    "            display: flex;" +
    "            align-items: center;" +
    "            margin-bottom: 10px;" +
    "        }" +
    "        .input-container label {" +
    "            width: 80px;" + 
    "            margin-right: 10px;" +
    "        }" +
    "        .input-container input[type=\"text\"] {" +
    "            flex: 1;" +
    "            padding: 5px;" +
    "            border: 1px solid #ccc;" +
    "            border-radius: 3px;" +
    "            margin-right: 10px; "+ 
    "        }" +
    "        .input-container button {" +
    "            padding: 5px 10px;" +
    "            background-color: #333;" +
    "            color: #fff;" +
    "            font-size: 14px;" +
    "            font-weight: bold;" +
    "            border: none;" +
    "            border-radius: 3px;" +
    "            text-transform: uppercase;" +
    "            cursor: pointer;" +
    "        }" +
    "        .button-container {" +
    "            margin-top: 20px;" +
    "            text-align: center;" +
    "        }" +
    "        .button-container button {" +
    "            margin: 0 5px;" +
    "            padding: 10px 15px;" +
    "            background-color: #333;" +
    "            color: #fff;" +
    "            font-size: 14px;" +
    "            font-weight: bold;" +
    "            border: none;" +
    "            border-radius: 3px;" +
    "            text-transform: uppercase;" +
    "            cursor: pointer;" +
    "        }" +
    "        .button-container button:hover {" +
    "            background-color: #555;" +
    "        }" +
    "    </style>" +
    "</head>"+
    "<body>"+
    "    <script defer=\"defer\">"+
    "        function ledSwitch(ledNumber) {"+
    "            var xhttp = new XMLHttpRequest();" +
    "            xhttp.onreadystatechange = function() {" +
    "                if (this.readyState == 4 && this.status == 200) {" +
    "                    console.log('LED ' + ledNumber + ' state changed');" +
    "                }" +
    "            };" +
    "            if (ledNumber < 9 && ledNumber > 0) {" +
    "             xhttp.open('GET', '/Switch' + ledNumber, true);" +
    "            }" +
    "            else if(ledNumber == 9){" +
    "            xhttp.open('GET', '/AllOn', true);" +
    "            }" +
    "            else if(ledNumber == 0){" +
    "            xhttp.open('GET', '/AllOff', true);" +
    "            }" +
    "            xhttp.send();" +
    "        }" +
    "        function updateData() {"
    "            var xhr = new XMLHttpRequest();"
    "            xhr.open('GET', '/getData', true);"
    "            xhr.onreadystatechange = function() {"
    "              if (xhr.readyState === 4 && xhr.status === 200) {"
    "                var dataArray = JSON.parse(xhr.responseText);"
    "                document.getElementById('ch1').value = dataArray[0];"
    "                document.getElementById('ch2').value = dataArray[1];"
    "                document.getElementById('ch3').value = dataArray[2];"
    "                document.getElementById('ch4').value = dataArray[3];"
    "                document.getElementById('ch5').value = dataArray[4];"
    "                document.getElementById('ch6').value = dataArray[5];"
    "                document.getElementById('ch7').value = dataArray[6];"
    "                document.getElementById('ch8').value = dataArray[7];"
    "                document.getElementById('btn1').removeAttribute('disabled');"+
    "                document.getElementById('btn2').removeAttribute('disabled');"+
    "                document.getElementById('btn3').removeAttribute('disabled');"+
    "                document.getElementById('btn4').removeAttribute('disabled');"+
    "                document.getElementById('btn5').removeAttribute('disabled');"+
    "                document.getElementById('btn6').removeAttribute('disabled');"+
    "                document.getElementById('btn7').removeAttribute('disabled');"+
    "                document.getElementById('btn8').removeAttribute('disabled');"+
    "                document.getElementById('btn9').removeAttribute('disabled');"+
    "                document.getElementById('btn0').removeAttribute('disabled');"+
    "              }"+
    "            };"+
    "            xhr.send();"+
    "        }"+
    "        var refreshInterval = 200;"+                                     
    "        setInterval(updateData, refreshInterval);"+       
    "    </script>" +
    "    <div class=\"header\">"+
    "        <h1>ESP32-S3-8DI-8DO</h1>"+
    "    </div>"+
    "    <div class=\"container\">"+
    "        <div class=\"input-container\" style=\"margin-left: 105px;\">"+
    "            <label for=\"input1\">CH1 Output</label>"+
    "            <input type=\"text\" id=\"ch1\" />"+
    "            <button value=\"Switch1\" id=\"btn1\" disabled onclick=\"ledSwitch(1)\">Flip Output</button>"+
    "        </div>"+
    "        <div class=\"input-container\" style=\"margin-left: 105px;\">"+
    "            <label for=\"input2\">CH2 Output</label>"+
    "            <input type=\"text\" id=\"ch2\" />"+
    "            <button value=\"Switch2\" id=\"btn2\" disabled onclick=\"ledSwitch(2)\">Flip Output</button>"+
    "        </div>"+
    "        <div class=\"input-container\" style=\"margin-left: 105px;\">"+
    "            <label for=\"input3\">CH3 Output</label>"+
    "            <input type=\"text\" id=\"ch3\" />"+
    "            <button value=\"Switch3\" id=\"btn3\" disabled onclick=\"ledSwitch(3)\">Flip Output</button>"+
    "        </div>"+
    "        <div class=\"input-container\" style=\"margin-left: 105px;\">"+
    "            <label for=\"input4\">CH4 Output</label>"+
    "            <input type=\"text\" id=\"ch4\" />"+
    "            <button value=\"Switch4\" id=\"btn4\" disabled onclick=\"ledSwitch(4)\">Flip Output</button>"+
    "        </div>"+
    "        <div class=\"input-container\" style=\"margin-left: 105px;\">"+
    "            <label for=\"input5\">CH5 Output</label>"+
    "            <input type=\"text\" id=\"ch5\" />"+
    "            <button value=\"Switch5\" id=\"btn5\" disabled onclick=\"ledSwitch(5)\">Flip Output</button>"+
    "        </div>"+
    "        <div class=\"input-container\" style=\"margin-left: 105px;\">"+
    "            <label for=\"input6\">CH6 Output</label>"+
    "            <input type=\"text\" id=\"ch6\" />"+
    "            <button value=\"Switch6\" id=\"btn6\" disabled onclick=\"ledSwitch(6)\">Flip Output</button>"+
    "        </div>"+
    "        <div class=\"input-container\" style=\"margin-left: 105px;\">"+
    "            <label for=\"input7\">CH7 Output</label>"+
    "            <input type=\"text\" id=\"ch7\" />"+
    "            <button value=\"Switch7\" id=\"btn7\" disabled onclick=\"ledSwitch(7)\">Flip Output</button>"+
    "        </div>"+
    "        <div class=\"input-container\" style=\"margin-left: 105px;\">"+
    "            <label for=\"input8\">CH8 Output</label>"+
    "            <input type=\"text\" id=\"ch8\" />"+
    "            <button value=\"Switch8\" id=\"btn8\" disabled onclick=\"ledSwitch(8)\">Flip Output</button>"+
    "        </div>"+
    "        <div class=\"button-container\">"+
    "            <button value=\"AllOn\" id=\"btn9\" disabled onclick=\"ledSwitch(9)\">All High</button>"+
    "            <button value=\"AllOff\" id=\"btn0\" disabled onclick=\"ledSwitch(0)\">All Low</button>"+
    "        </div>"+
    "    </div>"+
    "</body>"+
    "</html>";
    
  server.send(200, "text/html", myhtmlPage); 
  printf("The user visited the home page\r\n");
  
}

void handleGetData() {
  String json = "[";
  for (int i = 0; i < sizeof(Dout_Flag) / sizeof(Dout_Flag[0]); i++) {
    json += String(Dout_Flag[i]);
    if (i < sizeof(Dout_Flag) / sizeof(Dout_Flag[0]) - 1) {
      json += ",";
    }
  }
  json += "]";
  server.send(200, "application/json", json);
}

void handleSwitch(uint8_t ledNumber) {
  uint8_t Data[1]={0};
  Data[0]=ledNumber+48;
  Dout_Analysis(Data,WIFI_Mode_Trigger);
  server.send(200, "text/plain", "OK");
}
void handleSwitch1() { handleSwitch(1); }
void handleSwitch2() { handleSwitch(2); }
void handleSwitch3() { handleSwitch(3); }
void handleSwitch4() { handleSwitch(4); }
void handleSwitch5() { handleSwitch(5); }
void handleSwitch6() { handleSwitch(6); }
void handleSwitch7() { handleSwitch(7); }
void handleSwitch8() { handleSwitch(8); }
void handleSwitch9() { handleSwitch(9); }
void handleSwitch0() { handleSwitch(0); }

void WIFI_Init()
{
  xTaskCreatePinnedToCore(
    WifiStaTask,    
    "WifiStaTask",   
    4096,                
    NULL,                 
    3,                   
    NULL,                 
    0                   
  );
}


void WifiStaTask(void *parameter) {
  uint8_t Count = 0;
  WiFi.mode(WIFI_STA);                                   
  WiFi.setSleep(true);      
  WiFi.begin(ssid, password);                         // Connect to the specified Wi-Fi network
  while(1){
        if(WiFi.status() != WL_CONNECTED)
    {
      WIFI_Connection = 0;
      Count++;
      if(Count >= 30){
        Count = 0;
        WiFi.disconnect();
        vTaskDelay(pdMS_TO_TICKS(100));
        WiFi.mode(WIFI_OFF);
        vTaskDelay(pdMS_TO_TICKS(100));
        WiFi.mode(WIFI_STA);
        vTaskDelay(pdMS_TO_TICKS(100));
        WiFi.begin(ssid, password);
      }
    }
    else{
      WIFI_Connection = 1;
      IPAddress myIP = WiFi.localIP();
      printf("IP Address: ");
      sprintf(ipStr, "%d.%d.%d.%d", myIP[0], myIP[1], myIP[2], myIP[3]);
      printf("%s\r\n", ipStr);
      RGB_Open_Time(0, 50, 0, 1000, 0); 

      server.on("/", handleRoot);            // Dout Control page
      server.on("/getData", handleGetData);
      server.on("/Switch1", handleSwitch1);
      server.on("/Switch2", handleSwitch2);
      server.on("/Switch3", handleSwitch3);
      server.on("/Switch4", handleSwitch4);
      server.on("/Switch5", handleSwitch5);
      server.on("/Switch6", handleSwitch6);
      server.on("/Switch7", handleSwitch7);
      server.on("/Switch8", handleSwitch8);
      server.on("/AllOn"  , handleSwitch9);
      server.on("/AllOff" , handleSwitch0);
      
      server.begin(); 
      printf("Web server started\r\n"); 

      while (WiFi.status() == WL_CONNECTED){
        server.handleClient(); // Processing requests from clients
        vTaskDelay(pdMS_TO_TICKS(10));
      }
    }
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
  vTaskDelete(NULL);
}
