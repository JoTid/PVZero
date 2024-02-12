/**************************************************************

This file is a part of PVZero
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
#include <Arduino.h>
#include <ewcInterface.h>
#include "pvzero_class.h"

using namespace EWC;
using namespace PVZERO;

const char* FRIMWARE_VERSION = "1.0.0";
uint32_t TIMEOUT_WIFI = 60000;
uint32_t msStart = 0;
PVZeroClass pvzero;

// const String formatBytes(size_t const& bytes) {            // lesbare Anzeige der Speichergrößen
//   return bytes < 1024 ? static_cast<String>(bytes) + " Byte" : bytes < 1048576 ? static_cast<String>(bytes / 1024.0) + " KB" : static_cast<String>(bytes / 1048576.0) + " MB";
// }

// bool freeSpace(uint16_t const& printsize) {            // Funktion um beim speichern in Logdateien zu prüfen ob noch genügend freier Platz verfügbar ist.
//     FSInfo fs_info;
//     SPIFFS.info(fs_info);              // Füllt FSInfo Struktur mit Informationen über das Dateisystem
//     Serial.printf("Funktion: %s meldet in Zeile: %d FreeSpace: %s\n",__PRETTY_FUNCTION__,__LINE__, formatBytes(fs_info.totalBytes - (fs_info.usedBytes * 1.05)).c_str());
//     Dir dir = SPIFFS.openDir("/");
//     // or Dir dir = LittleFS.openDir("/data");
//     while (dir.next()) {
//         Serial.print(dir.fileName());
//         if(dir.fileSize()) {
//             File f = dir.openFile("r");
//             Serial.println(f.size());
//         }
//     }
//     return (fs_info.totalBytes - (fs_info.usedBytes * 1.05) > printsize) ? true : false;
// }

bool onceAfterConnect = false;

void setup() {
    Serial.begin(115200);
    Serial.println();
    Serial.print("ESP heap: ");
    Serial.println(ESP.getFreeHeap());
    EWC::I::get().server().setBrand("PVZero", FRIMWARE_VERSION);
    pvzero.setup();
    Serial.print("ESP heap: ");
    Serial.println(ESP.getFreeHeap());
    Serial.println("acconfig");
    if (I::get().config().paramWifiDisabled) {
        PZI::get().deviceState().setState(DeviceState::State::STANDALONE);
    } else {
        PZI::get().deviceState().setState(DeviceState::State::INIT, TIMEOUT_WIFI);
    }
    Serial.println("initialized");
}

void loop() {
    EWC::I::get().server().loop();
    if (WiFi.status() == WL_CONNECTED) {
        if (!onceAfterConnect) {
            I::get().logger() << "connected, " << PZI::get().time().str() << endl;
            onceAfterConnect = true;
            PZI::get().deviceState().setState(DeviceState::State::INIT);
        }
    }
    pvzero.loop();
}
