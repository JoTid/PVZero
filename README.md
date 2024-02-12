# PVZero

With an ESP32, the power grid data of a Shelly 3EM is queried, processed and used for current limiting with a Juntek
power supply unit, so that a zero-current feed-in of a PV system can be realised.

## Processing

Essentially, the limitation of the electricity feed-in is determined from the currently measured consumption in the
following steps:
![processing](./docs/images/processing.drawio.svg)

The consumption value is first filtered and then converted into the target value to be fed in.
The conversion is carried out according to **y = mx+b**, where **x** corresponds to the **consumption power** and **y**
to the **feed-in power**. The values for **m** and **b** are determined by the specified
**consumption and feed-in values**.

It should be noted that the feed-in power influences the consumption value and must therefore be subtracted from the
consumption value before each calculation in order to avoid oscillation or overshooting of the system.

## Arduino Framework

The project is based on [Arduino Framework](https://www.arduino.cc/reference/en) and is developed in
[VS Code](https://code.visualstudio.com/) using the [PlatformIO IDE](https://platformio.org/) extension.
