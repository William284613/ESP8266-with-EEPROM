#include <EEPROM.h> // Include the EEPROM library for reading and writing to EEPROM memory
#include <ESP8266WiFi.h> // Include the WiFi library for ESP8266
#include <ESP8266WebServer.h> // Include the WebServer library for ESP8266

ESP8266WebServer server(80); // Create a web server object on port 80

String ssid, password, deviceID, outputStatus; // Strings to store SSID, password, device ID, and output status
String content; // String to store the HTML content to be sent to the client
const int RelayPin = 13; // Define input pin for relay

void setup() {
  Serial.begin(115200); // Start the serial communication at a baud rate of 115200
  EEPROM.begin(512); // Initialize EEPROM with 512 bytes of memory
  delay(100); // Wait for 100 milliseconds to ensure everything is initialized properly

  pinMode(RelayPin, OUTPUT); // Initialize the relay pin as an output
  readData(); // Read the SSID, password, device ID, and output status from EEPROM
  updateRelay(); // Set the relay status based on the stored output status

  if (testWiFi()) { // If successfully connected to WiFi
    launchWeb(0); // Launch the web server in STA (Station) mode
  } else { // If not connected to WiFi
    const char* ssidap = "NodeMCU-AP"; // Set the SSID for AP (Access Point) mode
    const char* passap = ""; // Set the password for AP mode (empty password)
    WiFi.mode(WIFI_AP); // Set WiFi mode to Access Point
    WiFi.softAP(ssidap, passap); // Start the Access Point with the specified SSID and password
    Serial.print("Proceed to AP mode \n  http://");
    Serial.println(WiFi.softAPIP()); // Print the IP address of the Access Point
    launchWeb(1); // Launch the web server in AP mode
  }
}

void launchWeb(int webtype) {
  createWebServer(webtype); // Create the web server based on the type (0 for STA mode, 1 for AP mode)
  server.begin(); // Start the web server
}

void loop() {
  server.handleClient(); // Handle client requests
}

void createWebServer(int webtype) {
  String style = "<style>"
                 "body { font-family: Arial, sans-serif; background-color: #cfe0f3; margin: 0; padding: 0; display: flex; justify-content: center; align-items: center; height: 100vh; }"
                 "h1, h2 { color: #b50202; text-align: center; }"
                 "h2 { font-size: 18px; }"
                 "h1 { font-size: 22px; }"
                 "p { font-size: 14px; color: #000; }"
                 ".container { background-color: #fff; padding: 20px; border-radius: 10px; box-shadow: 0 0 10px rgba(0,0,0,0.1); max-width: 600px; width: 100%; }"
                 ".section { border: 1px solid #000; padding: 10px; margin-bottom: 20px; }"
                 "form { display: flex; flex-direction: column; }"
                 "label { font-size: 14px; margin-bottom: 10px; font-weight: bold; text-align: left; }"
                 "input { font-size: 14px; margin-bottom: 10px; width: 100%; padding: 8px; box-sizing: border-box; }"
                 "input[type='submit'] { background-color: #007bff; color: white; padding: 10px 20px; border: none; border-radius: 5px; cursor: pointer; margin-top: 10px; }"
                 "input[type='submit']:hover { background-color: #0056b3; }"
                 "table { width: 100%; margin: 20px auto; border-collapse: collapse; }"
                 "table, th, td { border: 1px solid black; }"
                 "th, td { padding: 10px; text-align: left; }"
                 ".button { background-color: #007bff; color: white; padding: 10px 20px; border: none; border-radius: 5px; cursor: pointer; text-align: center; display: inline-block; }"
                 ".button:hover { background-color: #0056b3; }"
                 "</style>";

  if (webtype == 0) { // STA Mode
    server.on("/", [style]() {
      content = "<!DOCTYPE HTML>\r\n<html>" + style + "<div class='container'><h1>WiFi Mode</h1><br>";
      content += "Welcome to Your NodeMCU<h2>";
      content += "<div class='section'><table>";
      content += "<tr><th>SSID:</th><td>" + ssid + "</td></tr>"; // Display SSID
      content += "<tr><th>Password:</th><td>" + password + "</td></tr>"; // Display Password
      content += "<tr><th>Device ID:</th><td>" + deviceID + "</td></tr>"; // Display Device ID
      content += "<tr><th>Output Status:</th><td>" + outputStatus + "</td></tr>"; // Display Output Status
      content += "</table></div>";
      content += "<a class='button' href='/toggle'>Toggle Relay</a>"; // Button to toggle relay status
      content += "</div></html>";
      server.send(200, "text/html", content); // Send the HTML content to the client
    });

    server.on("/toggle", [style]() {
      // Toggle relay status
      outputStatus = (outputStatus == "ON") ? "OFF" : "ON"; // Toggle the output status
      writeData(ssid, password, deviceID, outputStatus); // Save the new status to EEPROM
      updateRelay(); // Update the relay status
      content = "<!DOCTYPE HTML>\r\n<html>" + style + "<div class='container'><h1>Relay Toggled</h1><br>";
      content += "<p>Relay is now " + outputStatus + ".</p>";
      content += "<a href='/'>Go Back</a></div>";
      content += "</html>";
      server.send(200, "text/html", content); // Send the confirmation page to the client
    });
  } else if (webtype == 1) { // AP Mode
    server.on("/", [style]() {
      content = "<!DOCTYPE HTML>\r\n<html>" + style + "<div class='container'><h1>Access Point Mode</h1><br>";
      content += "<h2>Welcome to NodeMCU WiFi configuration</h2>";
      content += "<p><b>Your current configuration</b></p>";
      content += "<div class='section'><table>";
      content += "<tr><th>SSID:</th><td>" + ssid + "</td></tr>"; // Display SSID
      content += "<tr><th>Password:</th><td>" + password + "</td></tr>"; // Display Password
      content += "<tr><th>Device ID:</th><td>" + deviceID + "</td></tr>"; // Display Device ID
      content += "<tr><th>Output Status:</th><td>" + outputStatus + "</td></tr>"; // Display Output Status
      content += "</table></div>";
      content += "<p><b>New configuration</b></p>";
      content += "<div class='section'><form method='get' action='setting'>"; // Create a form for new configuration
      content += "<label>SSID:</label><input name='ssid' length=32><br>"; // Input for SSID
      content += "<label>Password:</label><input name='password' length=32><br>"; // Input for Password
      content += "<label>Device ID:</label><input name='device_id' length=32><br>"; // Input for Device ID
      content += "<label>Output Status:</label><br>";
      content += "<input type='radio' name='output_status' value='ON'>ON<br>"; // Radio button for ON status
      content += "<input type='radio' name='output_status' value='OFF'>OFF<br>"; // Radio button for OFF status
      content += "<input type='submit' value='Submit'></form></div></div>"; // Submit button for the form
      content += "</html>";
      server.send(200, "text/html", content); // Send the HTML content to the client
    });

    server.on("/setting", [style]() {
      ssid = server.arg("ssid"); // Get the value of SSID from the form
      password = server.arg("password"); // Get the value of Password from the form
      deviceID = server.arg("device_id"); // Get the value of Device ID from the form
      outputStatus = server.arg("output_status"); // Get the value of Output Status from the form
      writeData(ssid, password, deviceID, outputStatus); // Write the new configuration to EEPROM
      updateRelay(); // Update the relay status based on the new configuration
      content = "<!DOCTYPE HTML>\r\n<html>" + style + "<div class='container'><h1>Configuration Successful</h1><br>";
      content += "Please reboot the device to apply the new settings.<br>";
      content += "<a href='/'>Go Back</a></div>";
      content += "</html>";
      server.send(200, "text/html", content); // Send the confirmation page to the client
    });
  }
}

boolean testWiFi() {
  WiFi.begin(ssid.c_str(), password.c_str()); // Start WiFi with the stored SSID and password
  int c = 0; // Counter for retry attempts
  while (c < 10) { // Retry 10 times
    if (WiFi.status() == WL_CONNECTED) { // If connected to WiFi
      Serial.println(String("\nWiFi Status: " + String(WiFi.status())) + ", WiFi Connected"); // Print WiFi status (3 for WL_CONNECTED)
      Serial.println("IP Address: " + WiFi.localIP().toString()); // Print local IP address
      return true; // Return true if connected
    }
    unsigned long elapsedTime = millis() / 1000; // Get elapsed time in seconds
    Serial.println("WiFi Connection Attempt: " + String(c) + ", Time: " + String(elapsedTime) + " seconds"); // Print attempt count and elapsed time
    delay(1000); // Wait for 1000 milliseconds (1 second) before the next retry
    c++;
  }
  Serial.println("\nConnection time out, No WiFi Connected"); // Print timeout message if not connected
  return false; // Return false if not connected
}

void writeData(String ssid, String password, String deviceID, String outputStatus) {
  Serial.println("Writing to EEPROM"); // Print a message to the serial monitor indicating the start of the write process

  // Write SSID to EEPROM
  for (int i = 0; i < 32; i++) { // Loop through the first 32 bytes of EEPROM
    if (i < ssid.length()) { // If the current index is less than the length of the SSID string
      EEPROM.write(i, ssid[i]); // Write the character from the SSID string to the current EEPROM address
    } else { // If the current index is greater than or equal to the length of the SSID string
      EEPROM.write(i, 0); // Write a null character (0) to the current EEPROM address
    }
  }

  // Write Password to EEPROM
  for (int i = 32; i < 64; i++) { // Loop through the next 32 bytes of EEPROM (from address 32 to 63)
    if (i - 32 < password.length()) { // If the current index minus 32 is less than the length of the Password string
      EEPROM.write(i, password[i - 32]); // Write the character from the Password string to the current EEPROM address
    } else { // If the current index minus 32 is greater than or equal to the length of the Password string
      EEPROM.write(i, 0); // Write a null character (0) to the current EEPROM address
    }
  }

  // Write Device ID to EEPROM
  for (int i = 64; i < 96; i++) { // Loop through the next 32 bytes of EEPROM (from address 64 to 95)
    if (i - 64 < deviceID.length()) { // If the current index minus 64 is less than the length of the Device ID string
      EEPROM.write(i, deviceID[i - 64]); // Write the character from the Device ID string to the current EEPROM address
    } else { // If the current index minus 64 is greater than or equal to the length of the Device ID string
      EEPROM.write(i, 0); // Write a null character (0) to the current EEPROM address
    }
  }

  // Write Output Status to EEPROM
  EEPROM.write(96, (outputStatus == "ON") ? '1' : '0'); // Write '1' if the Output Status is "ON" and '0' if it is "OFF" to EEPROM address 96

  EEPROM.commit(); // Commit the changes to EEPROM to ensure they are saved
  Serial.println("Write successful"); // Print a message to the serial monitor indicating the successful completion of the write process
}

void readData() {
  Serial.println("Reading from EEPROM....");

  ssid = ""; // Initialize SSID to an empty string
  password = ""; // Initialize password to an empty string
  deviceID = ""; // Initialize deviceID to an empty string
  outputStatus = ""; // Initialize outputStatus to an empty string

  // Read SSID from EEPROM
  for (int i = 0; i < 32; i++) {
    char c = char(EEPROM.read(i)); // Read a character from EEPROM
    if (c != 0) ssid += c; // Append the character to the SSID string if it's not null
  }

  // Read Password from EEPROM
  for (int i = 32; i < 64; i++) {
    char c = char(EEPROM.read(i)); // Read a character from EEPROM
    if (c != 0) password += c; // Append the character to the Password string if it's not null
  }

  // Read Device ID from EEPROM
  for (int i = 64; i < 96; i++) {
    char c = char(EEPROM.read(i)); // Read a character from EEPROM
    if (c != 0) deviceID += c; // Append the character to the Device ID string if it's not null
  }

  // Read Output Status from EEPROM
  char statusChar = char(EEPROM.read(96)); // Read the Output Status character from EEPROM
  outputStatus = (statusChar == '1') ? "ON" : "OFF"; // Set the Output Status based on the character value

  Serial.println("WiFi SSID from EEPROM: " + ssid); // Print SSID
  Serial.println("WiFi Password from EEPROM: " + password); // Print Password
  Serial.println("Device ID from EEPROM: " + deviceID); // Print Device ID
  Serial.println("Output Status from EEPROM: " + outputStatus); // Print Output Status
  Serial.println("Reading successful.....");
}

void updateRelay() {
  if (outputStatus == "ON") {
    digitalWrite(RelayPin, LOW); // Turn the relay on (assuming LOW triggers the relay)
  } else {
    digitalWrite(RelayPin, HIGH); // Turn the relay off
  }
}
