long display_needed_setup_time = 1; //ms
bool display_initialized;
char s_status[32];
char s_hzg_temp[5];
char s_hzg_temp_yesterday[5];

#include <ESP8266WiFiMulti.h>
ESP8266WiFiMulti wifiMulti;

#include "credentials.h"
#include <InfluxDbClient.h>
#include "myheaders.h"

#define INFLUXDB_URL "http://192.168.178.143:8086"
// InfluxDB 1.8+ (v2 compatibility API) use form user:password, eg. admin:adminpass
#define INFLUXDB_TOKEN "admin:admin"
// InfluxDB 1.8+ (v2 compatibility API) use any non empty string
#define INFLUXDB_ORG "org name/id"
#define INFLUXDB_BUCKET "homedb/autogen"

// Set timezone string according to https://www.gnu.org/software/libc/manual/html_node/TZ-Variable.html
/* #define TZ_INFO "CET-1CEST,M3.5.0,M10.5.0/3" */
#define TZ_INFO "CET-1CEST-2,M3.5.0/02:00:00,M10.5.0/03:00:00"

// InfluxDB client instance
InfluxDBClient client(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET, INFLUXDB_TOKEN);

void setup() {
  Serial.begin(115200);

  // Connect wifi while display is initializing
  long unsigned display_setup_start_time = millis();
  displaySetup(); //start setup and connect wifi in meantime

  // Setup wifi
  WiFi.mode(WIFI_STA);
  wifiMulti.addAP(WIFI_SSID, WIFI_PASSWD);

  Serial.print("Connecting to wifi");
  while (wifiMulti.run() != WL_CONNECTED && !display_initialized ) {
    Serial.print(WiFi.status()); //serialdebug
    if (millis() < display_setup_start_time + display_needed_setup_time) {
      display_initialized = true;
      snprintf(s_status, sizeof(s_status), "%s", "Connecting...");
      displayRedraw(s_hzg_temp, s_hzg_temp_yesterday, s_status);
    }
    delay(50);
  }

  // Accurate time is necessary for certificate validation
  // For the fastest time sync find NTP servers in your area: https://www.pool.ntp.org/zone/
  // Syncing progress and the time will be printed to Serial
  snprintf(s_status, sizeof(s_status), "%s", "Syncing time...");
  if (display_initialized) { displayRedraw(s_hzg_temp, s_hzg_temp_yesterday, s_status);}
  timeSync(TZ_INFO, "pool.ntp.org", "time.nis.gov");

  // Check server connection
  if (client.validateConnection()) {
    Serial.print("Connected to InfluxDB: ");
    Serial.println(client.getServerUrl());
    snprintf(s_status, sizeof(s_status), "%s", "Querying...");
    displayRedraw(s_hzg_temp, s_hzg_temp_yesterday, s_status);
  } else {
    Serial.print("InfluxDB connection failed: ");
    Serial.println(client.getLastErrorMessage());
    snprintf(s_status, sizeof(s_status), "%s", "DB conn. failed!");
    displayRedraw(s_hzg_temp, s_hzg_temp_yesterday, s_status);
  }
}


void loop() {
  double hzg_temp { query_hzg_temp() } ;
  double hzg_temp_yesterday { query_hzg_temp_yesterday() } ;

  /* Serial.println("Today: "); */
  /* Serial.println(hzg_temp); */
  /* Serial.println("Yesterday: "); */
  /* Serial.println(hzg_temp_yesterday); */

  snprintf(s_hzg_temp, sizeof(s_hzg_temp), "%f", hzg_temp);
  snprintf(s_hzg_temp_yesterday, sizeof(s_hzg_temp), "%f", hzg_temp_yesterday);
  displayRedraw(s_hzg_temp, s_hzg_temp_yesterday, s_status);

  Serial.println("Wait 3s");
  delay(3000);
}

double query_hzg_temp() {
  String query = "from(bucket: \"" INFLUXDB_BUCKET "\") |> range(start: -1h) |> filter(fn: (r) => r._measurement == \"heizung/pufferspeicher/temp\" and r._field == \"value\") |> top(n:1)";
  Serial.println("querying with:");
  Serial.println(query);
  FluxQueryResult result = client.query(query);
  result.next();
  double value = result.getValueByName("_value").getDouble();
// Get converted value for the _time column
      FluxDateTime time = result.getValueByName("_time").getDateTime();

      // Format date-time for printing
      // Format string according to http://www.cplusplus.com/reference/ctime/strftime/
      String timeStr = time.format("%F %T");

      Serial.print(value);
      Serial.print(" now:  ");
      Serial.println(timeStr);

  // Check if there was an error
  if(result.getError() != "") {
    Serial.print("Query result error: ");
    Serial.println(result.getError());
  }

  result.close();
  return value;
}

double query_hzg_temp_yesterday() {
  String query = "from(bucket: \"" INFLUXDB_BUCKET "\") |> range(start: -15m) |> filter(fn: (r) => r._measurement == \"heizung/pufferspeicher/temp\" and r._field == \"value\") |> top(n:1)";
  FluxQueryResult result = client.query(query);
  result.next();
  double value = result.getValueByName("_value").getDouble();
	// Get converted value for the _time column
      FluxDateTime time = result.getValueByName("_time").getDateTime();

      // Format date-time for printing
      // Format string according to http://www.cplusplus.com/reference/ctime/strftime/
      String timeStr = time.format("%F %T");

      Serial.print(value);
      Serial.print(" yesterday at ");
      Serial.println(timeStr);

  // Check if there was an error
  if(result.getError() != "") {
    Serial.print("Query result error: ");
    Serial.println(result.getError());
  }

  result.close();
  return value;
}

void testquery() {

  String query = "from(bucket: \"" INFLUXDB_BUCKET "\") |> range(start: -1h) |> filter(fn: (r) => r._measurement == \"heizung/pufferspeicher/temp\" and r._field == \"value\") |> bottom(n:1)";

  // Print composed query
  Serial.print("Querying with: ");
  Serial.println(query);

  // Send query to the server and get result
  FluxQueryResult result = client.query(query);


  while (result.next()) {
    double value = result.getValueByName("_value").getDouble();
    Serial.print(value);

      // Get converted value for the _time column
      FluxDateTime time = result.getValueByName("_time").getDateTime();

      // Format date-time for printing
      // Format string according to http://www.cplusplus.com/reference/ctime/strftime/
      String timeStr = time.format("%F %T");

      Serial.print(" at ");
      Serial.println(timeStr);
    }

  // Check if there was an error
  if(result.getError() != "") {
    Serial.print("Query result error: ");
    Serial.println(result.getError());
  }

  // Close the result
  result.close();
}
void printAgregateResult(String selectorFunction) {
  // Construct a Flux query
  // Query will find RSSI for last hour for each connected WiFi network with this device computed by given selector function
  String query = "from(bucket: \"" INFLUXDB_BUCKET "\") |> range(start: -1h) |> filter(fn: (r) => r._measurement == \"heizung/pufferspeicher/temp\")";
  query += "|> " + selectorFunction + "()";
  // Print ouput header
  Serial.print("==== ");
  Serial.print(selectorFunction);
  Serial.println(" ====");

  // Print composed query
  Serial.print("Querying with: ");
  Serial.println(query);

  // Send query to the server and get result
  FluxQueryResult result = client.query(query);

  // Iterate over rows. Even there is just one row, next() must be called at least once.
  while (result.next()) {
    // Get converted value for flux result column 'SSID'
    String ssid = result.getValueByName("SSID").getString();
    Serial.print("SSID '");
    Serial.print(ssid);

    Serial.print("' with RSSI ");
    // Get converted value for flux result column '_value' where there is RSSI value
    // RSSI is integer value and so on min and max selected results,
    // whereas mean is computed and the result type is double.
    if(selectorFunction == "mean") {
      double value = result.getValueByName("_value").getDouble();
      Serial.print(value, 1);
      // computed value has not got a _time column, so omitting getting time here
    } else {
      long value = result.getValueByName("_value").getLong();
      Serial.print(value);

      // Get converted value for the _time column
      FluxDateTime time = result.getValueByName("_time").getDateTime();

      // Format date-time for printing
      // Format string according to http://www.cplusplus.com/reference/ctime/strftime/
      String timeStr = time.format("%F %T");

      Serial.print(" at ");
      Serial.print(timeStr);
    }


    Serial.println();
  }

}
