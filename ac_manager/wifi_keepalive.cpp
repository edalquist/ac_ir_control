#include "application.h"
#include "wifi_keepalive.h"

int checkInterval;
int resetInterval;
IPAddress pingDest;

int lastCheck;
int lastResponse;

void setupConnectionCheck() {
  checkInterval = 30;
  resetInterval = 90;
  IPAddress pd(192, 168, 0, 1);
  pingDest = pd;

  lastCheck = Time.now();
  lastResponse = lastCheck;

  Spark.variable("lastCheck", &lastCheck, INT);
  Spark.variable("lastResponse", &lastResponse, INT);
}

void checkConnection() {
  int now = Time.now();
  if ((now - lastCheck) >= checkInterval) {
    lastCheck = now;
    Serial.println(pingDest);
    int successes = WiFi.ping(pingDest, 8);

    if (successes > 0) {
      Spark.publish("PING", "SUCCESS");
      lastResponse = now;
    } else {
      Spark.publish("PING", "FAIL");
    }
  }
  if ((now - lastResponse) > resetInterval) {
    System.reset();
  }
}
