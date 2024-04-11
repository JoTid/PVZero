#include <Arduino.h>
#include <esp_core_dump.h>
#include <ewcInterface.h>
#include "pvzero_class.h"

using namespace EWC;
using namespace PVZ;

/*!
 * @brief PIN number of green LED
 */
#define GREEN_LED_PIN 13

/*!
 * @brief PIN number of red LED
 */
#define RED_LED_PIN 12

uint32_t TIMEOUT_WIFI = 60000;
PVZeroClass pvz;

/*--------------------------------------------------------------------------------------------------------------------*\
** Global modules                                                                                                     **
**                                                                                                                    **
\*--------------------------------------------------------------------------------------------------------------------*/

bool onceAfterConnect = false;

String readDumpSummery()
{
  esp_core_dump_init();
  esp_core_dump_summary_t summary;
  esp_err_t err = esp_core_dump_get_summary(&summary);
  String result;
  if (err == ESP_OK)
  {
    result = "Coredump info:\n";
    result += "  Program Counter[hex]:  " + String(summary.exc_pc, HEX) + "\n";
    result += "  Coredump Version[hex]: " + String(summary.core_dump_version, HEX) + "\n";
    result += "  Ex-info vaddr:         " + String(summary.ex_info.exc_vaddr) + "\n";
    result += "  Ex-info cause:         " + String(summary.ex_info.exc_cause) + "\n";
    result += "  Bt task:               " + String(summary.exc_task) + "\n";
    result += "  Bt corrupted:          " + String(summary.exc_bt_info.corrupted) + "\n";
    result += "  ELF file SHA256:       " + String(summary.app_elf_sha256, HEX) + "\n";
    EWC::I::get().logger() << "Backtrace corrupted: " << String(summary.exc_bt_info.corrupted) << endl;

    String backtracePc;
    for (int i = 0; i < summary.exc_bt_info.depth; i++)
    {
      uintptr_t pc = summary.exc_bt_info.bt[i]; // Program Counter (PC)
      if (i > 0)
      {
        backtracePc += " ";
      }
      backtracePc += String(pc, HEX);
    }
    EWC::I::get().logger() << "Backtrace[" << summary.exc_bt_info.depth << "]: " << backtracePc << endl;
    result += "  Backtrace [" + String(summary.exc_bt_info.depth) + "]: " + backtracePc + "\n";
    result += "\n";
    result += "Decode stacktrace with:\n";
    result += "C:\\Users\\{USER}\\.platformio\\packages\\toolchain-xtensa\\bin\\xtensa-lx106-elf-addr2line.exe -aipfC -e C:\\{PROJECT_PATH}\\.pio\\build\\az-delivery-devkit-v4\\firmware.elf " + backtracePc + "\n";
  }
  else
  {
    result = "Getting core dump summary not ok. Error: " + (int)err;
    result += "\nProbably no coredump present yet.";
    EWC::I::get().logger() << "[GET COREDUMP]: Getting core dump summary not ok. Error: " << (int)err << endl;
    EWC::I::get().logger() << "[GET COREDUMP]: Probably no coredump present yet." << endl;
    EWC::I::get().logger() << "[GET COREDUMP]: esp_core_dump_image_check(): " << esp_core_dump_image_check() << endl;
  }
  return result;
}


//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//                                                                                                                    //
//--------------------------------------------------------------------------------------------------------------------//
void setup()
{
  //---------------------------------------------------------------------------------------------------
  // initialise EspWebConfig
  //
  EWC::I::get().logger().setLogging(true);
  EWC::I::get().led().init(true, HIGH, GREEN_LED_PIN, RED_LED_PIN);
  EWC::I::get().config().paramDeviceName = String("pvz-") + EWC::I::get().config().getChipId();
  EWC::I::get().config().paramAPName = String("pvz-") + EWC::I::get().config().getChipId();
  EWC::I::get().config().paramHostname = String("pvz-") + EWC::I::get().config().getChipId();
  EWC::I::get().server().setBrand("PVZero", FIRMWARE_VERSION);

  //---------------------------------------------------------------------------------------------------
  // initialise PVZero
  //
  pvz.setup();
  if (I::get().config().paramWifiDisabled)
  {
    I::get().logger() << F("STANDALONE") << endl;
  }
  I::get().logger() << F("initialized") << endl;
}

//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//                                                                                                                    //
//--------------------------------------------------------------------------------------------------------------------//
void loop()
{
  EWC::I::get().server().loop();
  if (WiFi.status() == WL_CONNECTED)
  {
    if (!onceAfterConnect)
    {
      I::get().logger() << "connected, " << I::get().time().str() << endl;
      onceAfterConnect = true;
      I::get().logger() << "Send email about reboot" << endl;
      PZI::get().mail().sendChange("PVZ rebooted", readDumpSummery().c_str());
    }
  }
  else
  {
    // TODO: test if we should reinit after reconnect?
    onceAfterConnect = false;
  }

  pvz.loop();
}
