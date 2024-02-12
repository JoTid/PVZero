/**************************************************************

This file is a part of Solar EinspeseRegelungsSystem mit Shelly Em3
https://github.com/JoTid/PVZero

Copyright [2020] Alexander Tiderko

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

**************************************************************/
#include <Arduino.h>
#include <ewcInterface.h>
#include <ewcConfigServer.h>
#include "config.h"
#include "taster.h"


using namespace PVZERO;

Config::Config() : EWC::ConfigInterface("pvzero")
{
}

Config::~Config()
{
}

void Config::setup(JsonDocument& config, bool resetConfig)
{
    _initParameter();
    fromJson(config);
}

void Config::fillJson(JsonDocument& config)
{
    config["pvzero"]["name"] = _name;
    config["pvzero"]["version"] = EWC::I::get().server().version();
    config["pvzero"]["check_interval"] = checkInterval;
    config["pvzero"]["taster_func"] = tasterFunc;
    config["pvzero"]["shellyEm3Uri"] = shellyEm3Uri;
    config["pvzero"]["voltage"] = voltage;
    config["pvzero"]["max_amperage"] = maxAmperage;
}

void Config::fromJson(JsonDocument& config)
{
    JsonVariant jv = config["pvzero"]["check_interval"];
    if (!jv.isNull()) {
        checkInterval = jv.as<int>();
    }
    jv = config["pvzero"]["taster_func"];
    if (!jv.isNull()) {
        tasterFunc = jv.as<unsigned char>();
    }
    jv = config["pvzero"]["shellyEm3Uri"];
    if (!jv.isNull()) {
        shellyEm3Uri = jv.as<String>();
    }
    jv = config["pvzero"]["voltage"];
    if (!jv.isNull()) {
        voltage = jv.as<int>();
    }
    jv = config["pvzero"]["max_amperage"];
    if (!jv.isNull()) {
        maxAmperage = jv.as<int>();
    }
}

void Config::_initParameter()
{
    _name = "pvzero-" + EWC::I::get().config().getChipId();
    checkInterval = 1;
    //deep_sleep("deep-sleep", "Deep sleep"),
    shellyEm3Uri = "http://192.168.1.131";
    // taster
    tasterFunc = TASTER_CHECK_NOW;
    voltage = 38;
    maxAmperage = 10;
}
