#include "ThermManager.h"

ThermManager::ThermManager() {
  //webSocketBegin = false;
  //wifiRegisterCount = 0;
  //checkUpdateCount = 0;
	postData.reserve(2000);
}

void ThermManager::setHomeConnection(HomeConnection * __hc, RTCZero * __rtc) {
  this->hc = __hc;
  this->rtc = __rtc;
  logger.setRTC(this->rtc);
}



bool ThermManager::checkWiFiConnection() {
  if (hc->getConnectionStatus() != WL_CONNECTED) {
    logger.printlnLog("Connection lost ..");
    boolean rc = hc->connect();
    if (rc) {
      char lcdIP[50];
      char lcdMAC[50];
      hc->getLocalIp(lcdIP);
      hc->getMacAddress(lcdMAC);
      logger.printlnLog(
        "Connection successful - IP Address : %s , MAC Address : %s",
        lcdIP, lcdMAC);
    } else {
      logger.printlnLog("Not able to connect : status %d",
                        hc->getConnectionStatus());
    }
  }
  return hc->getConnectionStatus() == WL_CONNECTED;
}

unsigned long ThermManager::getWiFiTime() {
  return rtc->getEpoch();
}

void ThermManager::formatDate(time_t t, char* buffer) {
  struct tm * timeinfo;
  timeinfo = localtime(&t);
  strftime(buffer, 80, "%d-%m-%Y %H:%M:%S", timeinfo);
}

int ThermManager::checkRestError(JsonObject *root) {
  JsonObject& error = root->get<JsonObject>("error");  // ->["error"];
  int rc = error["code"];
  String message = error["message"];
  logger.printlnLog("Return code %d - %s", rc, message.c_str());
  return rc;
}

unsigned long ThermManager::convertTime(double t) {
  unsigned long l = t / 1000;
  return l;
}

/**
  Check fo server connection. If non available try to connect
*/
bool ThermManager::checkThermConfiguration(CONFIG *conf) {
  if (conf->key == 0) {
    logger.printlnLog("No configuration available.. call WiFiRegister");
    wiFiRegister(conf);
  }
  return conf->key != 0;
}

unsigned long ThermManager::wiFiRegister(CONFIG *config)
{
  long ret = 0;
#ifdef WEBSOCKET
  logger.printlnLog("START -> wiFiRegisterWS");
  webSocketConnect();
  if (webSocketBegin)
    _wiFiRegisterWS(config);
#else
  logger.printlnLog("START -> wiFiRegister");
  ret = _wiFiRegister(config);
#endif
  //webSocketBegin = false;
  logger.printlnLog("END -> wiFiRegister");
  return ret;
}

/*
  unsigned long ThermManager::wiFiRegisterWS(CONFIG *config)
  {
  logger.printlnLog("START -> wiFiRegisterWS");
  long ret = _wiFiRegister(config);
  webSocketBegin = false;
  logger.printlnLog("END -> wiFiRegisterWS");
  return ret;
  }
*/
void ThermManager::getCurrentData(TEMPDATA *outdata)
{
  logger.printlnLog("START -> getCurrentData");
  _getCurrentData(outdata);
  //webSocketBegin = false;
  logger.printlnLog("END -> getCurrentData");
}

void ThermManager::checkUpdate(bool first, CONFIG *config)
{
  logger.printlnLog("START -> checkUpdate");
  _checkUpdate(first, config);
  //webSocketBegin = false;
  logger.printlnLog("END -> checkUpdate");
}

void ThermManager::sendMonitorData(CONFIG *config, SENSORDATA *sensorData)
{
  logger.printlnLog("START -> sendMonitorData");
  _sendMonitorData(config, sensorData);
  //webSocketBegin = false;
  logger.printlnLog("END -> sendMonitorData");
}

/**
  void ThermManager::preparaCheckUpdateRequest(CONFIG *config, JsonObject& root)
  {

  root["id"] = config->key;
  if (config->progLoaded)
    root["lastUpdate"] = config->lastUpdate;
  }
**/
/*
void ThermManager::analizzaCheckUpdateResponse(CONFIG *conf, JsonObject& root)
{

  if (checkRestError(&root) == REST_RET_OK) {
    JsonObject& data = root["data"];
    int needUpdate = data["needUpdate"];
    logger.printlnLog("CheckUpdate : Need Update %d",
                      needUpdate);
    if (needUpdate == 1) {
      // Update configuration and programming
      JsonObject& configuration = data["configuration"];
      conf->tempMeasure = configuration.get<int>("tempMeasure");
      conf->serverStatus = configuration.get<int>("status");
      conf->lastUpdate = convertTime(configuration.get<double>("lastUpdate"));
      //conf->manualMode
      logger.printlnLog(
        "CheckUpdate : TempMeasure %d , Status %d , LastUpdate : %lu",
        conf->tempMeasure, conf->serverStatus,
        conf->lastUpdate);

      //TODO manage missing configuration
      JsonObject& currentTempProgram =
        data["currentTempProgram"];

      conf->minTemp = currentTempProgram.get<float>(
                        "minTemp");
      conf->minTempManual = currentTempProgram.get<float>(
                              "minTempManual");

      JsonArray& dayProgramming =
        currentTempProgram["dayProgramming"];
      int numDay = dayProgramming.size();
      for (int i = 0; i < numDay; i++) {
        JsonVariant _dayP = dayProgramming[i];
        JsonObject& dayP = _dayP;
        int idDay = dayP.get<int>("idDay");
        if (idDay >= 0 && idDay < MAX_DAY) {
          conf->day[idDay].day = idDay;
          JsonArray& prog = dayP["prog"];
          int numProg = prog.size();
          if (numProg > MAX_PROGDAY)
            numProg = MAX_PROGDAY;
          conf->day[idDay].numProg = numProg;
          logger.printlnLog(
            "CheckUpdate : Day %d - DefTemp %f - NumProg %d",
            idDay, conf->minTemp,
            conf->day[idDay].numProg);
          for (int j = 0; j < numProg; j++) {
            JsonVariant _proge = prog[j];

            JsonObject& proge = _proge;
            conf->day[idDay].prog[j].minTemp =
              proge.get<float>("minTemp");
            conf->day[idDay].prog[j].timeStart =
              proge.get<int>("timeStart");
            conf->day[idDay].prog[j].timeEnd =
              proge.get<int>("timeEnd");
            int pri = proge.get<int>("priorityDisp");
            if (pri == 0)
              pri = conf->key;
            conf->day[idDay].prog[j].priorityDisp = pri;
            logger.printlnLog(
              "CheckUpdate : Prog %d - Temp %f - Start %d - End %d - Priority %d",
              j, conf->day[idDay].prog[j].minTemp,
              conf->day[idDay].prog[j].timeStart,
              conf->day[idDay].prog[j].timeEnd,
              conf->day[idDay].prog[j].priorityDisp);
          }
        }
      }
      conf->progLoaded = true;
      logger.printlnLog("CheckUpdate : Configuration updated");
    } else
    {
      logger.printlnLog("CheckUpdate : Configuration not updated");
    }
  }

}
****/
/****

unsigned long ThermManager::analizzaWiFiRegisterResponse(CONFIG *config, JsonObject* root)
{
  if (checkRestError(root) == REST_RET_OK) {
    JsonObject& data = root->get<JsonObject>("data");
    config->lastUpdate = convertTime(
                           data.get<double>("lastUpdate"));
    config->lastAccess = convertTime(
                           data.get<double>("lastAccess"));
    config->key = data.get<int>("$loki");
    config->serverStatus = data.get<int>("status");
    config->progLoaded = false;
    config->tempMeasure = data.get<int>("tempMeasure");
    logger.printlnLog("Key %d - ServerStatus %d - Last %d", config->key,
                      config->serverStatus, config->lastUpdate);
    unsigned long now = hc->getTime();
    if (now == 0)
      now = config->lastAccess;
    int tzo = data.get<int>("timeZoneOffset");
    now -= tzo * 60;
    rtc->setEpoch(now);
    return now;
  }
  return 0;
}
***/

/***
void ThermManager::preparaWiFiRegisterRequest(CONFIG *config, JsonObject& root)
{
  //String PostData;
  //PostData.reserve(1000);
  //DynamicJsonBuffer jsonBuffer(GET_JSON_BUFFER);
  //JsonObject& root = jsonBuffer.createObject();
  char macAddress[50];
  hc->getMacAddress(macAddress);
  char ipAddress[50];
  hc->getLocalIp(ipAddress);

  root["flagLcd"] = config->flagLcd;
  root["flagLightSensor"] = config->flagLightSensor;
  root["flagMotionSensor"] = config->flagMotionSensor;
  root["flagReleTemp"] = config->flagReleTemp;
  root["flagReleLight"] = config->flagReleLight;
  root["macAddress"] = macAddress;
  root["ipAddress"] = ipAddress;
  //root.printTo(request);
  //return request;
}
***/

unsigned long ThermManager::_wiFiRegister(CONFIG *config) {

  unsigned long now = 0;
  postData = "";
  //String postData;
  //postData.reserve(1000);
  //DynamicJsonBuffer jsonBuffer(GET_JSON_BUFFER);
  StaticJsonBuffer<GET_JSON_BUFFER> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();

  char macAddress[50];
  hc->getMacAddress(macAddress);
  char ipAddress[50];
  hc->getLocalIp(ipAddress);

  root["flagLcd"] = config->flagLcd;
  root["flagLightSensor"] = config->flagLightSensor;
  root["flagMotionSensor"] = config->flagMotionSensor;
  root["flagReleTemp"] = config->flagReleTemp;
  root["flagReleLight"] = config->flagReleLight;
  root["macAddress"] = macAddress;
  root["ipAddress"] = ipAddress;
  root.printTo(postData);
  if (hc->httpPostMethod(&client, POST_REGISTER, postData)) {
    //DynamicJsonBuffer jsonBuffer(GET_JSON_BUFFER);
	StaticJsonBuffer<GET_JSON_BUFFER> jsonBufferOut;
    JsonObject& root = jsonBufferOut.parseObject(client);
    if (!root.success()) {
      logger.printlnLog("parseObject() failed");
    } else {
      //JsonObject& data = root["data"];
      //int d = data["$loki"];
      //logger.printlnLog("KEY %d", d);
      //return analizzaWiFiRegisterResponse(config, &root);
      if (checkRestError(&root) == REST_RET_OK) {
        //JsonObject& data = root.get<JsonObject>("data");
        JsonObject& data = root["data"];
        config->lastUpdate = convertTime(
                               data.get<double>("lastUpdate"));
        config->lastAccess = convertTime(
                               data.get<double>("lastAccess"));
        config->key = data.get<int>("$loki");
        config->serverStatus = data.get<int>("status");
        config->progLoaded = false;
        config->tempMeasure = data.get<int>("tempMeasure");
        logger.printlnLog("Key %d - ServerStatus %d - Last %d", config->key,
                          config->serverStatus, config->lastUpdate);
        //unsigned long
        now = hc->getTime();
        if (now == 0)
          now = config->lastAccess;
        int tzo = data.get<int>("timeZoneOffset");
        now -= tzo * 60;
        rtc->setEpoch(now);
        //return now;
      }
    }
  }
  return now;
}

/**
   GetCurretDtata
*/
void ThermManager::_getCurrentData(TEMPDATA *outdata) {
  postData = "";
  if (hc->httpGetMethod(&client, GET_CURRENTDATA)) {
    //DynamicJsonBuffer jsonBuffer(GET_JSON_BUFFER);
    StaticJsonBuffer<GET_JSON_BUFFER> jsonBuffer;
    JsonObject& root = jsonBuffer.parseObject(client);
    if (!root.success()) {
      logger.printlnLog("parseObject() failed");
    } else {
      if (checkRestError(&root) == REST_RET_OK) {
        JsonObject& data = root["data"];
        outdata->now = convertTime(data.get<double>("now"));
        outdata->tempMeasure = data["tempMeasure"];
        JsonArray& t = data["temp"];
        outdata->num = t.size();
        time_t tim = getWiFiTime(); //hc.getTime();
        char buffer[80];
        formatDate(tim, buffer);
        logger.printlnLog("tempMeasure %d - now %lu(%s) - size %d ",
                          outdata->tempMeasure, outdata->now, buffer,
                          outdata->num);
        //myprintln(printBuffer);
        for (int i = 0; i < outdata->num; i++) {
          JsonVariant r = t[i];
          JsonObject& rr = r;
          outdata->data[i].idDisp = rr["idDisp"];
          outdata->data[i].temperature = rr["temperature"];
          outdata->data[i].tmsUpd = convertTime(
                                      rr.get<double>("tmsUpd"));
          logger.printlnLog("idDisp %d - Temp %f",
                            outdata->data[0].idDisp,
                            outdata->data[0].temperature);
          //logger.printlnLog(printBuffer);
        }
      }
    }
  }

}

/**
  Check if any update of configuration has been done
*/
void ThermManager::_checkUpdate(bool first, CONFIG *conf) {
  postData = "";
  char GET[100];
  if (checkThermConfiguration(conf)) {

    if (first || conf->progLoaded == false)
      sprintf(GET, "%s/%d", GET_CHECK, conf->key);
    else
      sprintf(GET, "%s/%d?lastUpdate=%lu000",
              GET_CHECK, conf->key, conf->lastUpdate);
    if (hc->httpGetMethod(&client, GET)) {
      DynamicJsonBuffer jsonBuffer(GET_JSON_BUFFER);
      JsonObject& root = jsonBuffer.parseObject(client);
      if (!root.success()) {
        logger.printlnLog("parseObject() failed");
      } else {
        if (checkRestError(&root) == REST_RET_OK) {
          JsonObject& data = root["data"];
          int needUpdate = data["needUpdate"];
          logger.printlnLog("CheckUpdate : Need Update %d",
                            needUpdate);
          if (needUpdate == 1) {
            // Update configuration and programming
            JsonObject& configuration = data["configuration"];
            conf->tempMeasure = configuration.get<int>("tempMeasure");
            conf->serverStatus = configuration.get<int>("status");
            conf->lastUpdate = convertTime(configuration.get<double>("lastUpdate"));
            //conf->manualMode
            logger.printlnLog(
              "CheckUpdate : TempMeasure %d , Status %d , LastUpdate : %lu",
              conf->tempMeasure, conf->serverStatus,
              conf->lastUpdate);

            //TODO manage missing configuration
            JsonObject& currentTempProgram =
              data["currentTempProgram"];

            conf->minTemp = currentTempProgram.get<float>(
                              "minTemp");
            conf->minTempManual = currentTempProgram.get<float>(
                                    "minTempManual");

            JsonArray& dayProgramming =
              currentTempProgram["dayProgramming"];
            int numDay = dayProgramming.size();
            for (int i = 0; i < numDay; i++) {
              JsonVariant _dayP = dayProgramming[i];
              JsonObject& dayP = _dayP;
              int idDay = dayP.get<int>("idDay");
              if (idDay >= 0 && idDay < MAX_DAY) {
                conf->day[idDay].day = idDay;
                JsonArray& prog = dayP["prog"];
                int numProg = prog.size();
                if (numProg > MAX_PROGDAY)
                  numProg = MAX_PROGDAY;
                conf->day[idDay].numProg = numProg;
                logger.printlnLog(
                  "CheckUpdate : Day %d - DefTemp %f - NumProg %d",
                  idDay, conf->minTemp,
                  conf->day[idDay].numProg);
                //myprintln(printBuffer);
                for (int j = 0; j < numProg; j++) {
                  JsonVariant _proge = prog[j];

                  JsonObject& proge = _proge;
                  conf->day[idDay].prog[j].minTemp =
                    proge.get<float>("minTemp");
                  conf->day[idDay].prog[j].timeStart =
                    proge.get<int>("timeStart");
                  conf->day[idDay].prog[j].timeEnd =
                    proge.get<int>("timeEnd");
                  int pri = proge.get<int>("priorityDisp");
                  if (pri == 0)
                    pri = conf->key;
                  conf->day[idDay].prog[j].priorityDisp = pri;
                  logger.printlnLog(
                    "CheckUpdate : Prog %d - Temp %f - Start %d - End %d - Priority %d",
                    j, conf->day[idDay].prog[j].minTemp,
                    conf->day[idDay].prog[j].timeStart,
                    conf->day[idDay].prog[j].timeEnd,
                    conf->day[idDay].prog[j].priorityDisp);
                }
              }
            }
            conf->progLoaded = true;
            logger.printlnLog("CheckUpdate : Configuration updated");
          } else
          {
            logger.printlnLog("CheckUpdate : Configuration not updated");
          }
        }

      }
    }
  }
}

/**
  Call Monitor to update server with current sensor values
*/
void ThermManager::_sendMonitorData(CONFIG *conf, SENSORDATA *sensorData) {
  postData = "";
  if (checkThermConfiguration(conf)) {
    if (sensorData->numItem == 0) {
      logger.printlnLog("Monitor... nothing to send");
    } else {
      //String PostData;
      //PostData.reserve(1000);
      StaticJsonBuffer<GET_JSON_BUFFER> jsonBuffer;// (GET_JSON_BUFFER);
      JsonObject& root = jsonBuffer.createObject();
      root["temperature"] = sensorData->totalTemperature
                            / sensorData->numItem;
      root["pressure"] = sensorData->totalPressure / sensorData->numItem
                         / 100.0;
      root["light"] = sensorData->totalLight / sensorData->numItem;
      root["humidity"] = sensorData->humidity / sensorData->numItem;
      root["status"] = conf->clientStatus;
      root["numSurveys"] = sensorData->numItem;
      //root.printTo(PostData);
      int len = root.measureLength();
      root.printTo(postData);
      char POST[100];
      sprintf(POST, "%s/%d", POST_MONITOR, conf->key);
      if (hc->httpPostMethod(&client, POST, postData)) {
        //DynamicJsonBuffer jsonBuffer(GET_JSON_BUFFER);
    	StaticJsonBuffer<GET_JSON_BUFFER> jsonBufferOut;
        JsonObject& rootOut = jsonBufferOut.parseObject(client);
        if (!rootOut.success()) {
          logger.printlnLog("parseObject() failed");
        } else checkRestError(&rootOut);
      }
    }
  }
}
