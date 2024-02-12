/**************************************************************

This file is a part of
https://github.com/JoTid/PVZero

Copyright [2020] Alexander Tiderko

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-3.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

**************************************************************/
#include <ewcConfigServer.h>
#include "generated/pvzeroSetupHTML.h"
#include "generated/webIndexHTML.h"
#include "generated/webLanguagesJSON.h"
#include "generated/webShelly_em3_connectorJS.h"
#include "pvzero_class.h"
#include "pvzero_interface.h"

using namespace EWC;
using namespace PVZERO;

#if defined(ESP8266)
#define MY_SHELLY_PIN D6
#else
#define MY_SHELLY_PIN 6
#endif

PVZeroClass::PVZeroClass()
:   
    _shellyEm3Connector(MY_SHELLY_PIN) // pinPot=D6
{
    PZI::get()._pvzero = this;
    PZI::get()._time = &_ewcTime;
    PZI::get()._config = &_config;
    PZI::get()._ewcServer = &_ewcServer;
    PZI::get()._deviceState = &_deviceState;
    PZI::get()._shellyEm3Connector = &_shellyEm3Connector;
    PZI::get()._lcd = &_lcd;
    _timePrinted = false;
}

PVZeroClass::~PVZeroClass()
{
}

void PVZeroClass::setup()
{
    EWC::I::get().configFS().addConfig(_ewcUpdater);
    EWC::I::get().configFS().addConfig(_ewcTime);
    EWC::I::get().configFS().addConfig(_config);
    // EWC::I::get().led().enable(true, LED_BUILTIN, LOW);
    EWC::I::get().server().enableConfigUri();
    EWC::I::get().server().setup();
    EWC::I::get().config().paramLanguage = "de";
    EWC::I::get().server().insertMenuG("Device", "/pvzero/setup", "menu_device", FPSTR(PROGMEM_CONFIG_TEXT_HTML), HTML_PVZERO_SETUP_GZIP, sizeof(HTML_PVZERO_SETUP_GZIP), true, 0);
    WebServer* ws = &EWC::I::get().server().webserver();
    EWC::I::get().logger() << F("Setup Webserver") << endl;
    EWC::I::get().server().webserver().on(HOME_URI, std::bind(&ConfigServer::sendContentG, &EWC::I::get().server(), ws, FPSTR(PROGMEM_CONFIG_TEXT_HTML), HTML_WEB_INDEX_GZIP, sizeof(HTML_WEB_INDEX_GZIP)));
    EWC::I::get().server().webserver().on("/languages.json", std::bind(&ConfigServer::sendContentG, &EWC::I::get().server(), ws, FPSTR(PROGMEM_CONFIG_APPLICATION_JSON), JSON_WEB_LANGUAGES_GZIP, sizeof(JSON_WEB_LANGUAGES_GZIP)));
    EWC::I::get().server().webserver().on("/pvzero/config.json", std::bind(&PVZeroClass::_onPVZeroConfig, this, ws));
    // EWC::I::get().server().webserver().on("/bbs/cycle.svg", std::bind(&ConfigServer::sendContentG, &EWC::I::get().server(), ws, "image/svg+xml", SVG_BBS_CYCLE_GZIP, sizeof(SVG_BBS_CYCLE_GZIP)));
    EWC::I::get().server().webserver().on("/pvzero/config/save", std::bind(&PVZeroClass::_onPVZeroSave, this, ws));
    EWC::I::get().server().webserver().on("/pvzero/state.json", std::bind(&PVZeroClass::_onPVZeroState, this, ws));
    EWC::I::get().server().webserver().on("/check", std::bind(&PVZeroClass::_onPVZeroCheck, this, ws));
    // EWC::I::get().server().webserver().on("/cycle1/pump", std::bind(&PVZeroClass::_onBbsPump1, this, ws));
    // EWC::I::get().server().webserver().on("/cycle2/pump", std::bind(&PVZeroClass::_onBbsPump2, this, ws));
    EWC::I::get().server().webserver().on("/js/shelly_em3_connector.js", std::bind(&ConfigServer::sendContentG, &EWC::I::get().server(), ws, FPSTR(PROGMEM_CONFIG_APPLICATION_JS), JS_WEB_SHELLY_EM3_CONNECTOR_GZIP, sizeof(JS_WEB_SHELLY_EM3_CONNECTOR_GZIP)));
    _shellyEm3Connector.setup(EWC::I::get().configFS().resetDetected());
    _taster.setup(EWC::I::get().configFS().resetDetected());
    // _mqttHelper.setup(_ewcMqtt);
    _tsMeasLoopStart = millis();
    _shellyEm3Connector.setCallbackState(std::bind(&PVZeroClass::_onPotState, this, std::placeholders::_1, std::placeholders::_2));
    EWC::I::get().logger() << F("Setup ok") << endl;
}

void PVZeroClass::loop()
{
    unsigned long ts_now = millis();
    _deviceState.loop();
    _taster.loop();
    _ewcUpdater.loop();
    // we perform the measurement only once per second
    if (ts_now - _tsMeasLoopStart < 1000) {
        return;
    }
    _tsMeasLoopStart = ts_now;
    switch(_deviceState.currentState()) {
        case DeviceState::State::SETUP:
            EWC::I::get().logger() << F("Enter SETUP") << endl;
            break;
        case DeviceState::State::STANDALONE:
            EWC::I::get().logger() << F("Switch to standalone mode.") << endl;
            _deviceState.setState(DeviceState::State::INIT);
            break;
        case DeviceState::State::INIT:
            EWC::I::get().logger() << F("Enter INIT") << endl;
            _deviceState.setState(DeviceState::State::RUNNING);
            break;
        case DeviceState::State::RUNNING:
            break;
        case DeviceState::State::SOFTSLEEP:
            break;
    }
    if (! _deviceState.ready()) {
        // device initialization...
        return;
    }
    if (PZI::get().ewcServer().isConnected()) {
        if (!_timePrinted)
        {
            // TODO update ntp time not only at start
            I::get().logger() << "Check for NTP..." << endl;
            _ewcTime.setupTime();
            if (_ewcTime.timeAvailable())
            {
                _timePrinted = true;
                // print current time
                I::get().logger() << "Current time:" << _ewcTime.str() << endl;
                // or current time in seconds
                I::get().logger() << "  as seconds:" << _ewcTime.currentTime() << endl;
            }
        }
        _shellyEm3Connector.loop();
    } else {
        // TODO: check how to wake up
        EWC::I::get().logger() << F("Switch to SOFTSLEEP") << endl;
        _deviceState.setState(DeviceState::State::SOFTSLEEP);
    }
}

void PVZeroClass::_onPVZeroConfig(WebServer* webserver)
{
    I::get().logger() << F("[PVZERO] config request") << endl;
    if (!I::get().server().isAuthenticated(webserver)) {
        I::get().logger() << F("[PVZERO] not sufficient authentication") << endl;
        return webserver->requestAuthentication();
    }
    DynamicJsonDocument jsonDoc(512);
    _config.fillJson(jsonDoc);
    String output;
    serializeJson(jsonDoc, output);
    webserver->send(200, FPSTR(PROGMEM_CONFIG_APPLICATION_JSON), output);
}

void PVZeroClass::_onPVZeroSave(WebServer* webserver)
{
    I::get().logger() << F("[PVZERO] save config request") << endl;
    if (!I::get().server().isAuthenticated(webserver)) {
        I::get().logger() << F("[PVZERO] not sufficient authentication") << endl;
        return webserver->requestAuthentication();
    }
    DynamicJsonDocument config(512);
    if (webserver->hasArg("check_interval")) {
        long val = webserver->arg("check_interval").toInt();
        if (val > 0) {
           config["pvzero"]["check_interval"] = val;
        }
    }
    if (webserver->hasArg("taster_func")) {
        long val = webserver->arg("taster_func").toInt();
        if (val >= 0) {
            config["pvzero"]["taster_func"] = val;
        }
    }
    if (webserver->hasArg("shellyEm3Uri")) {
        String val = webserver->arg("shellyEm3Uri");
        if (val.length() >= 0) {
            config["pvzero"]["shellyEm3Uri"] = val;
        }
    }
    if (webserver->hasArg("voltage")) {
        long val = webserver->arg("voltage").toInt();
        if (val >= 0) {
            config["pvzero"]["voltage"] = val;
        }
    }
    if (webserver->hasArg("voltage")) {
        long val = webserver->arg("max_amperage").toInt();
        if (val >= 0) {
            config["pvzero"]["max_amperage"] = val;
        }
    }
    
    _config.fromJson(config);
    I::get().configFS().save();
    String details;
    serializeJsonPretty(config["pvzero"], details);
    I::get().server().sendPageSuccess(webserver, "PVZERO Config save", "Save successful!", "/pvzero/setup", "<pre id=\"json\">" + details + "</pre>");
}

void PVZeroClass::_onPVZeroState(WebServer* webserver)
{
    I::get().logger() << F("[PVZERO] state request") << endl;
    if (!I::get().server().isAuthenticated(webserver)) {
        I::get().logger() << F("[PVZERO] not sufficient authentication") << endl;
        return webserver->requestAuthentication();
    }
    DynamicJsonDocument jsonDoc(1024);
    JsonObject json = jsonDoc.to<JsonObject>();
    json["name"] = I::get().server().brand();
    json["version"] = I::get().server().version();
    json["current_excess"] = _shellyEm3Connector.currentExcess();
    json["current_current"] = _shellyEm3Connector.currentCurrent();
    json["check_info"] = _shellyEm3Connector.info();
    json["next_check"] = _shellyEm3Connector.infoSleepUntil();
    String output;
    serializeJson(json, output);
    webserver->send(200, FPSTR(PROGMEM_CONFIG_APPLICATION_JSON), output);
}

void PVZeroClass::_onPVZeroCheck(WebServer* webserver)
{
    I::get().logger() << F("[PVZERO] check request") << endl;
    if (!I::get().server().isAuthenticated(webserver)) {
        I::get().logger() << F("[PVZERO] not sufficient authentication") << endl;
        return webserver->requestAuthentication();
    }
    // _shellyEm3Connector.sleeper().wakeup();
    // I::get().server().sendRedirect(webserver, HOME_URI);
    webserver->send(200, FPSTR(PROGMEM_CONFIG_APPLICATION_JSON), "{\"success\": true}");
}

void PVZeroClass::_onPotState(bool state, int duration)
{
    // _mqttHelper.publishC1Pump();
}

