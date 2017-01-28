#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>

/* Set these to your desired credentials. */
const char *ssid = "ESPHost";
const char *password = "";

int maxAPs = 20;

ESP8266WebServer server(80);

void handleConnect() {
  //Format passed would be 'ssid='+ssid+'&password='+password+'&device='+deviceName
  //No error reporting as the only request should come from ourselves which should already be formatted.
  String stringSsid;
  String stringPassword;
  String device;
  String returnData = "";

  for ( uint8_t i = 0; i < server.args(); i++ ) {
    if (server.argName(i) == "ssid") {
      stringSsid = server.arg(i);
    } else if (server.argName(i) == "password") {
      stringPassword = server.arg(i);
    } else if (server.argName(i) == "device") {
      device = server.arg (i);
    }
  }

  int ssidLength = stringSsid.length();
  int passLength = stringPassword.length();

  char routerSsid[ssidLength];
  char routerPassword[passLength];

  stringSsid.toCharArray(routerSsid, ssidLength + 1);
  stringPassword.toCharArray(routerPassword, passLength + 1);

  WiFi.hostname(device);
  Serial.println("SSID");
  Serial.println(routerSsid);
  Serial.println("Password");
  Serial.println(routerPassword);
  WiFi.begin(routerSsid, routerPassword);
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && WiFi.status() != WL_CONNECT_FAILED && WiFi.status() != WL_NO_SSID_AVAIL) {
    //Attempting connection
    delay(500);
    Serial.print(".");
    Serial.println(WiFi.status());
  }


  if (WiFi.status() == WL_CONNECTED) {
    returnData = "{\"success\": true,\"ip\": \"" + WiFi.localIP().toString() + "\"}";
  } else {
    //Some issue, possibly with password
    returnData = "{\"error\": \"Failed to connect to access point\"}";
  }

  server.send(200, "application/json", returnData);
  //Add delay so the server has time to send the response
  delay(500);
  WiFi.softAPdisconnect(true);
}

/* Just a little test message.  Go to http://192.168.4.1 in a web browser
   connected to this access point to see it.
*/
void handleRoot() {
  int signalStrengths[maxAPs][maxAPs];
  int n = WiFi.scanNetworks();
  Serial.println("scan done");
  String sendData = "<!DOCTYPE html><html lang='en'><head> <meta charset='UTF-8'> <title>Find Configurator</title> <style type='text/css'> html, body{padding:0px; margin:0px; background: #efefef; font-family:Helvetica;}h2{color: #1e7800; padding:0px; margin:0px; margin-bottom:10px;}select{background: white; width:100%; font-size:1em;}.spaced{margin-top:5px; margin-bottom:5px;}.modal{width:400px; margin-left:auto; margin-right:auto; background:white; padding:30px; margin-top:20px; border-radius:20px;}</style> <script type='application/javascript'> function createId(){var text=''; var characters='ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789'; for( var i=0; i < 5; i++ ) text +=characters.charAt(Math.floor(Math.random() * characters.length)); document.getElementById('deviceName').value='FIND-'+text;}function post(url, params, callback){var http=new XMLHttpRequest(); http.open('POST', url, true); http.setRequestHeader('Content-type', 'application/x-www-form-urlencoded'); http.onreadystatechange=function(){if(http.readyState==4 && http.status==200){alert(http.responseText); callback(http.responseText);}}; http.send(params);}function ssidConnect(){var ssid=document.getElementById('ssidSelect').value; var password=document.getElementById('wifiPassword').value; var deviceName=document.getElementById('deviceName').value; var errors=false; if (ssid==''){alert('Please select an SSID'); errors=true;}if (password==''){var check=confirm('No password has been added, assuming this is an insecure network'); if (!check){alert('Please add a password'); errors=true;}}if (ssid==''){alert('Please select an SSID'); errors=true;}if (!errors){post('/connect', 'ssid='+ssid+'&password='+password+'&device='+deviceName, function (responseData){var returnedObject=JSON.parse(responseData); if (returnedObject.error !==undefined){alert(returnedObject.error);}else if(returnedObject.success !==undefined){alert('AP should be disabled now, also new IP will be' + returnedObject.ip);}});}}</script></head><body onload='createId();'> <div class='modal'> <div class='stage-1'> <h2>Step 1</h2> Select your home wifi <select id='ssidSelect'>";
  if (n == 0)
    Serial.println("no networks found");
  else
  {
    //Strength code, curtosy of RajDarge http://www.esp8266.com/viewtopic.php?f=29&t=6295
    for (int i = 0; i < n; ++i)
    {
      if (i < maxAPs) {
        signalStrengths[1][i] = i;
        signalStrengths[0][i] = WiFi.RSSI(i);
        delay(10);
      }
    }

    for (int i = 1; i < n; ++i)
    { // insert sort into strongest signal
      int j = signalStrengths[0][i]; //holding value for signal strength
      int l = signalStrengths[1][i];
      int k;
      for (k = i - 1; (k >= 0) && (j > signalStrengths[0][k]); k--)
      {
        signalStrengths[0][k + 1] = signalStrengths[0][k];
        l = signalStrengths[1][k + 1];
        signalStrengths[1][k + 1] = signalStrengths[1][k];
        signalStrengths[1][k] = l; //swap index values here.
      }
      signalStrengths[0][k + 1] = j;
      signalStrengths[1][k + 1] = l; //swap index values here to re-order.
    }

    int j = 0;
    for (int j = 0; j < n; j++) {

      sendData += "<option value='" + WiFi.SSID(signalStrengths[1][j]) + "'>" + WiFi.SSID(signalStrengths[1][j]) + "</option>";
    }
  }
  sendData += "</select> <table> <tr> <td>Password</td><td><input type='password' id='wifiPassword'/></td></tr><tr> <td>Device Name</td><td><input type='text' id='deviceName'/></td></tr><tr> <td><input type='button' value='Submit' onclick='ssidConnect();'/></td></tr></table> </div></div></body></html>";

  server.send(200, "text/html", sendData);
}

void setup() {
  delay(1000);
  Serial.begin(115200);
  Serial.println();
  Serial.print("Configuring access point...");
  /* You can remove the password parameter if you want the AP to be open. */
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);

  //  SPIFFS.begin();

  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);
  server.on("/", handleRoot);
  server.on("/connect", handleConnect);
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();
}
