#ifndef MessageParser_h
#define MessageParser_h

#define ARDUINOJSON_USE_LONG_LONG 1

#include <ArduinoJson.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include "Logging.h"

class MessageParser
{
public:
  MessageParser(Logging *_logger);
  void preparaWiFiRegisterRequest(CONFIG &config, DynamicJsonDocument &root);
  bool preparaMonitorDataRequest(CONFIG &config, SENSORDATA &sensorData, DynamicJsonDocument &root);
  void updateConfigurationResponse(CONFIG &config, char *message);

private:
  int checkRestError(DynamicJsonDocument &doc);
  bool deserialize(DynamicJsonDocument &doc, char *message);
  Logging *logger;
};
#endif
