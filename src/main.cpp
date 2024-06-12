#include <Firebase_ESP_Client.h>
#include <ArduinoJson.h>
#include <HX711_ADC.h>
#include <WiFi.h>
#include <time.h>
#include <ctime>
#include <HTTPClient.h>

using namespace std;

// Definitions for WiFi
const char *WIFI_SSID = "Tabaranza_WIFI";
const char *WIFI_PASSWORD = "Gabby1713";

// Definitions for Firebase Database
const char *FIREBASE_HOST = "https://petness-92c55-default-rtdb.asia-southeast1.firebasedatabase.app/";
const char *FIREBASE_AUTH = "bhvzGLuvbjReHlQjk77UwWGtCVdBBUBABE3X4PQ2";

// Definitions for Firestore Database
const char *FIREBASE_FIRESTORE_HOST = "petness-92c55";

// Define the API Key
const char *API_KEY = "AIzaSyDPcMRU9x421wP0cS1sRHwEvi57W8NoLiE";

// Define the user Email and password that already registerd or added in your project
const char *USER_EMAIL = "petnessadmin@gmail.com";
const char *USER_PASSWORD = "petness";

// Definitions for NTP
const char *NTP_SERVER = "2.asia.pool.ntp.org";
const long GMT_OFFSET_SEC = 28800;
const int DAYLIGHT_OFFSET_SEC = 0;

// Definitions for HX711
const int HX711_dout = 13;
const int HX711_sck = 27;
HX711_ADC LoadCell(HX711_dout, HX711_sck);

// Define the struct to hold the data
typedef struct struct_message
{
  float amountToDispense;
} struct_message;

struct_message myData;

// Declare the FirebaseData objects at the global scope
FirebaseData fbdo1;
FirebaseData fbdo2;
FirebaseData fbdo3;
FirebaseConfig config;
FirebaseAuth auth;

// Declare the FirebaseJson object at the global scope
FirebaseJson content;

// Declare the paths at the global scope
String pathGetPetWeight = "/trigger/getPetWeight/status";
String pathAmountToDispense = "/petFeedingSchedule";

void petWeightTare()
{
  LoadCell.begin();
  long stabilizingtime = 2000;
  LoadCell.start(stabilizingtime);
  if (LoadCell.getTareTimeoutFlag())
  {
    Serial.println("Timeout, check MCU>HX711 wiring and pin designations");
    while (1)
      ;
  }
  else
  {
    LoadCell.setCalFactor(-20.84);
    Serial.println("Startup + tare is complete");
  }
}

// Function to send data via HTTP
void sendData(float amountToDispense)
{
  if (WiFi.status() == WL_CONNECTED)
  {
    HTTPClient http;
    String serverName = "http://192.168.1.20/receive"; // Replace with your receiver's IP address and endpoint
    http.begin(serverName);
    http.addHeader("Content-Type", "application/json");

    String jsonPayload = "{\"amountToDispense\": ";
    jsonPayload.concat(String(amountToDispense));
    jsonPayload.concat("}");

    int httpResponseCode = http.POST(jsonPayload);

    if (httpResponseCode > 0)
    {
      String response = http.getString();
      String httpResponse = "HTTP Response code: ";
      httpResponse.concat(String(httpResponseCode));
      Serial.println(httpResponse);

      String responseOutput = "Response: ";
      responseOutput.concat(response);
      Serial.println(responseOutput);
    }
    else
    {
      Serial.print("Error code: ");
      Serial.println(httpResponseCode);
    }

    http.end();
  }
  else
  {
    Serial.println("WiFi not connected");
  }
}

float samplesForGettingWeight()
{
  float totalWeight = 0;
  int sampleCount = 0;
  int numIgnoreSamples = 5;
  int numSamples = 10;

  for (int i = 0; i < numIgnoreSamples; i++)
  {
    bool newDataReady = false;

    while (!newDataReady)
    {
      if (LoadCell.update())
        newDataReady = true;
    }

    if (newDataReady)
    {
      float weightSample = LoadCell.getData();
      Serial.print("Ignoring sample: ");
      Serial.println(weightSample);
    }

    delay(200);
  }

  for (int i = 0; i < numSamples - numIgnoreSamples; i++)
  {
    bool newDataReady = false;

    while (!newDataReady)
    {
      if (LoadCell.update())
        newDataReady = true;
    }

    if (newDataReady)
    {
      float weightSample = LoadCell.getData();
      totalWeight += weightSample;
      sampleCount++;
      Serial.print("Load_cell output val: ");
      Serial.println(weightSample);
    }
    delay(500);
  }

  float petsWeight = totalWeight / sampleCount;
  Serial.print("PETS WEIGHT:  ");
  Serial.println(petsWeight);

  return petsWeight;
}

void setPetWeight()
{
  float petsWeight = samplesForGettingWeight();

  String path = "/getWeight/loadCell/weight";
  if (Firebase.RTDB.setFloat(&fbdo1, path.c_str(), petsWeight))
  {
    Serial.println("Weight data updated successfully.");
  }
  else
  {
    Serial.print("Failed to update weight data, reason: ");
    Serial.println(fbdo1.errorReason());
  }

  path = "/trigger/getPetWeight/status";
  if (Firebase.RTDB.setBool(&fbdo1, path.c_str(), false))
  {
    Serial.println("Trigger status updated successfully.");
  }
  else
  {
    Serial.print("Failed to update trigger status, reason: ");
    Serial.println(fbdo1.errorReason());
  }
}

String getCurrentDate()
{
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
  {
    Serial.println("Failed to obtain Current Date");
    return "";
  }
  char dateStr[11];
  strftime(dateStr, sizeof(dateStr), "%Y-%m-%d", &timeinfo);
  return String(dateStr);
}

String getCurrentTime()
{
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
  {
    Serial.println("Failed to obtain Hour & Minute time");
    return "";
  }
  char timeStr[6];
  strftime(timeStr, sizeof(timeStr), "%H:%M", &timeinfo);
  return String(timeStr);
}

int getCurrentHour()
{
  struct tm timeinfo;
  int retries = 10;
  while (!getLocalTime(&timeinfo))
  {
    Serial.println("Failed to obtain time, retrying...");
    delay(1000);
    retries--;
    if (retries == 0)
    {
      Serial.println("Failed to obtain time");
      return -1;
    }
  }
  return timeinfo.tm_hour;
}

int getCurrentMin()
{
  struct tm timeinfo;
  int retries = 10;
  while (!getLocalTime(&timeinfo))
  {
    Serial.println("Failed to obtain time, retrying...");
    delay(1000);
    retries--;
    if (retries == 0)
    {
      Serial.println("Failed to obtain time");
      return -1;
    }
  }
  return timeinfo.tm_min;
}

String generateUniqueId()
{
  String uniqueId = "";
  for (int i = 0; i < 20; i++)
  {
    uniqueId += String(random(0, 10));
  }
  return uniqueId;
}

void recordFeedingDataToFirestore(const String &mode, const String &userName, float amount, const String &scheduleDate, const String &scheduledTime)
{
  content.clear(); // Clear previous content to avoid conflicts
  content.set("fields/mode/stringValue", mode);
  content.set("fields/userName/stringValue", userName);
  content.set("fields/amount/doubleValue", amount);
  content.set("fields/date/stringValue", scheduleDate);
  content.set("fields/time/stringValue", scheduledTime);

  String recordId = generateUniqueId();
  String documentPath = "/pets/";
  documentPath.concat(userName);
  documentPath.concat("/records/");
  documentPath.concat("/");

  Serial.print("Document path: ");
  Serial.println(documentPath);

  if (Firebase.Firestore.createDocument(&fbdo3, FIREBASE_FIRESTORE_HOST, "", documentPath.c_str(), content.raw()))
  {
    Serial.println("Feeding record added successfully.");
    Serial.printf("ok\n%s\n\n", fbdo3.payload().c_str());
  }
  else
  {
    Serial.print("Failed to add feeding record, reason: ");
    Serial.println(fbdo3.errorReason());
  }
}

void deleteScheduledFeeding(String userName, String scheduledDate, String scheduledTime)
{
  String path = pathAmountToDispense;
  path += "/";
  path += userName;
  path += "/scheduledFeeding/";

  Serial.println(path);

  if (Firebase.RTDB.deleteNode(&fbdo2, path.c_str()))
  {
    Serial.println("Scheduled feeding deleted successfully.");
  }
  else
  {
    Serial.print("Failed to delete scheduled feeding, reason: ");
    Serial.println(fbdo2.errorReason());
  }
}

void scheduledDispenseFood(String userName, float amountToDispense, String scheduledDate, String scheduledTime)
{
  static bool hasRun = false;
  static bool recordHasRun = false;
  static String previousScheduledDate = "";
  static String previousScheduledTime = "";

  Serial.println("scheduledDispenseFood function called");

  float amount = amountToDispense;
  String currentDateStr = getCurrentDate();
  String currentTimeStr = getCurrentTime();

  struct tm scheduledTime_tm = {};
  struct tm currentTime_tm = {};

  strptime(scheduledDate.c_str(), "%Y-%m-%d", &scheduledTime_tm);
  strptime(scheduledTime.c_str(), "%H:%M", &scheduledTime_tm);

  int scheduledHour = scheduledTime_tm.tm_hour;
  int scheduledMin = scheduledTime_tm.tm_min;

  int currentHour = getCurrentHour();
  int currentMin = getCurrentMin();

  Serial.print("Current Date: ");
  Serial.println(currentDateStr);
  Serial.print("Scheduled Date: ");
  Serial.println(scheduledDate);
  Serial.print("Current Time: ");
  Serial.println(currentTimeStr);
  Serial.print("Scheduled Time: ");
  Serial.println(scheduledTime);

  if (currentHour == -1 || scheduledHour == -1)
  {
    Serial.println("Failed to obtain current or scheduled time");
    return;
  }

  // Reset hasRun flag if the date or time changes
  if (previousScheduledDate != scheduledDate || previousScheduledTime != scheduledTime)
  {
    hasRun = false;
    recordHasRun = false; // Reset recordHasRun flag
    previousScheduledDate = scheduledDate;
    previousScheduledTime = scheduledTime;
  }

  if (currentDateStr == scheduledDate && currentHour == scheduledHour && currentMin == scheduledMin)
  {
    if (!hasRun)
    {
      Serial.println("Scheduled time reached, dispensing food");
      delay(1000); // 1 second delay

      // Add your code here to dispense the food
      // Send the data to the dispensing ESP32
      // sendData(userName.c_str(), amountToDispense, scheduledDate.c_str(), scheduledTime.c_str());
      Serial.println("Food is being dispensed");
      sendData(amountToDispense);
      Serial.println("Food dispensed successfully");

      // Record the feeding data to Firestore
      recordFeedingDataToFirestore("scheduledRecord", userName, amount, scheduledDate, scheduledTime);

      hasRun = true;
      recordHasRun = true; // Set recordHasRun flag
    }
  }
  else if ((currentDateStr > scheduledDate) && currentHour > scheduledHour || currentMin > scheduledMin)
  {
    hasRun = false;
    recordHasRun = false;
    Serial.println("Schedule has passed");
    // Delete the scheduled feeding from the database
    deleteScheduledFeeding(userName, scheduledDate, scheduledTime);
  }
  else
  {
    Serial.println("Current time has not reached the scheduled time yet");
  }
}

void smartDispenseFood(String userName, float totalAmount, int servings)
{
  static int lastDispensedHour = -1;
  static bool hasRun = false;

  Serial.println("smartDispenseFood function called");

  float amountToDispense = totalAmount / servings;
  int currentHour = getCurrentHour();
  String scheduledDate = getCurrentDate();
  String scheduledTime = getCurrentTime();

  bool shouldDispense = false;

  // Determine if food should be dispensed based on the current hour and number of servings
  switch (servings)
  {
  case 1:
    shouldDispense = (currentHour == 5);
    break;
  case 2:
    shouldDispense = (currentHour == 12 || currentHour == 18);
    break;
  case 3:
    shouldDispense = (currentHour == 8 || currentHour == 15 || currentHour == 20);
    break;
  default:
    Serial.println("Invalid number of servings");
    return;
  }

  // Check if food should be dispensed and if it hasn't been dispensed in this hour yet
  if (shouldDispense && currentHour != lastDispensedHour)
  {
    Serial.print("Dispensing ");
    Serial.print(amountToDispense);
    Serial.print(" at ");
    Serial.print(currentHour);
    Serial.println(":00");

    // Send the data to the dispensing ESP32
    // sendData(userName.c_str(), amountToDispense, scheduledDate.c_str(), scheduledTime.c_str());
    sendData(amountToDispense);

    // Record the feeding data to Firestore
    recordFeedingDataToFirestore("smartRecord", userName, amountToDispense, scheduledDate, scheduledTime);

    lastDispensedHour = currentHour;
    hasRun = true;
  }
  else if (hasRun && currentHour != lastDispensedHour)
  {
    hasRun = false;
  }
  else
  {
    Serial.print("Current Hour: ");
    Serial.println(currentHour);
  }
}

void feedingDataPerModePerValue()
{
  if (Firebase.RTDB.get(&fbdo2, pathAmountToDispense.c_str()))
  {
    if (fbdo2.dataType() == "json")
    {
      FirebaseJson *json = fbdo2.jsonObjectPtr();
      String jsonString;
      json->toString(jsonString);

      DynamicJsonDocument doc(2048);
      deserializeJson(doc, jsonString);

      for (JsonPair kv : doc.as<JsonObject>())
      {
        String userName = kv.key().c_str();
        JsonObject userObject = kv.value().as<JsonObject>();

        if (userObject.containsKey("smartFeeding"))
        {
          JsonObject smartFeedingObject = userObject["smartFeeding"].as<JsonObject>();
          bool smartFeedingStatus = smartFeedingObject["smartFeedingStatus"].as<bool>();

          if (smartFeedingStatus)
          {
            for (JsonPair kv2 : smartFeedingObject)
            {
              if (kv2.value().is<JsonObject>())
              {
                JsonObject arrayObject = kv2.value().as<JsonObject>();
                String amountToDispense = arrayObject["amountToDispensePerServingPerDay"].as<String>();
                float totalAmount = amountToDispense.toFloat();
                int servings = arrayObject["servings"].as<int>();

                smartDispenseFood(userName, totalAmount, servings);
              }
            }
          }
        }

        if (userObject.containsKey("scheduledFeeding"))
        {
          JsonObject scheduledFeedingObject = userObject["scheduledFeeding"].as<JsonObject>();
          bool scheduledFeedingStatus = scheduledFeedingObject["scheduledFeedingStatus"].as<bool>();

          if (scheduledFeedingStatus)
          {
            for (JsonPair kv2 : scheduledFeedingObject)
            {
              if (kv2.value().is<JsonObject>())
              {
                JsonObject arrayObject = kv2.value().as<JsonObject>();
                float amountToFeed = arrayObject["amountToFeed"].as<float>();
                String scheduledDate = arrayObject["scheduledDate"].as<String>();
                String scheduledTime = arrayObject["scheduledTime"].as<String>();

                scheduledDispenseFood(userName, amountToFeed, scheduledDate, scheduledTime);
              }
            }
          }
        }
      }
    }
  }
  else
  {
    Serial.print("Failed to get feeding data, reason: ");
    Serial.println(fbdo2.errorReason());
  }
}

void petWeightStream()
{
  if (Firebase.RTDB.readStream(&fbdo1))
  {
    if (fbdo1.streamTimeout())
    {
      Serial.println("Stream timeout, no data received from Firebase");
    }
    else if (fbdo1.dataType() == "boolean")
    {
      bool status = fbdo1.boolData();
      Serial.print("Status of Weight Stream: ");
      Serial.println(String(status).c_str());
      if (status)
      {
        setPetWeight();
      }
    }
  }
  else
  {
    Serial.print("Failed to read stream, reason: ");
    Serial.println(fbdo1.errorReason());
  }
}

void setup()
{
  Serial.begin(115200);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(300);
  }
  Serial.println("Connected to WiFi");
  Firebase.reconnectWiFi(true);

  config.host = FIREBASE_HOST;
  config.signer.tokens.legacy_token = FIREBASE_AUTH;
  /* Assign the api key (required) */
  config.api_key = API_KEY;
  /* Assign the user sign in credentials */
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  Firebase.begin(&config, &auth);

  configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, NTP_SERVER);

  petWeightTare();

  if (Firebase.RTDB.beginStream(&fbdo1, pathGetPetWeight.c_str()))
  {
    Serial.println("Stream getPetWeight begin, success");
  }
  else
  {
    Serial.print("Stream begin failed, reason: ");
    Serial.println(fbdo1.errorReason());
  }
}

unsigned long lastPetWeightStreamTime = 0;
unsigned long lastFeedingDataFetchTime = 0;

void loop()
{
  unsigned long currentMillis = millis();

  if (currentMillis - lastPetWeightStreamTime >= 1000)
  {
    petWeightStream();
    lastPetWeightStreamTime = currentMillis;
  }

  if (currentMillis - lastFeedingDataFetchTime >= 3000)
  {
    feedingDataPerModePerValue();
    lastFeedingDataFetchTime = currentMillis;
  }
}
