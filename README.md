# PVZero

With an ESP32, the power grid data of a **Shelly 3EM** is queried, processed and used for current limiting with a
**DPM86xx** power supply unit, so that a zero-current feed-in of a PV system can be realized.

## System setup

The system for which the PVZero application is being realised looks as follows in its full configuration:

![processing](./docs/images/system_setup.drawio.svg)

The settings for the maximum power to be fed in and how many strings are controlled are made via the Web GUI.

## Processing

The limitation of the **Feed-in target DC Current** is determined in the following steps:

![processing](./docs/images/processing.drawio.svg)

The consumption value is first filtered and then converted into the target value to be fed in.
The conversion is carried out according to **y = mx+b**, where **x** corresponds to the **Consumption Power** and **y**
to the **Feed-in target DC Current**. The values for **m** and **b** are determined by the provided
**Feed-in values for current and voltage**.

It should be noted that the **Feed-in actual Power** influences the **Consumption Power** and must therefore be added to
the total consumption value before each calculation in order to avoid oscillation or overshooting of the system.

## Arduino Framework

The project is based on [Arduino Framework](https://www.arduino.cc/reference/en) and is developed in
[VS Code](https://code.visualstudio.com/) using the [PlatformIO IDE](https://platformio.org/) extension.
