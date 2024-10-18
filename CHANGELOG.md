# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog],
and this project adheres to [Semantic Versioning].

## [Unreleased]

- Read additional parameter from MPPT loader: Panel Voltage and Panel Power
- Update API of the MQTT Property method to provide additional required configuration variables.
  So there are no errors or warnings at Home Assistant server
- Set QoS of MQTT to 0 (fire and forget), as that values are send cyclic each second and no acknowledge is required

## [0.81.10] - 2024-04-30

- Adjust values for battery guard
- Add expected yield for today, that is derived from MPPT yield value
- Increase performance of the USART communication with the PSUs

## [0.81.09] - 2024-04-12

- Send dump per e-mail after a crash
- Fix access to a null pointer while number conversion in mppt module
- Adjust values for battery guard
- Provide additional value via MQTT

## [0.81.08] - 2024-04-05

- Add MPPT state to the GUI
- Update State machine of battery guard, introduce new states and define optimize conditions for transitions
- Additional MQTT entities has been introduced
- Increase performance at communication with MPPT charger
- Update communication handling with DPM86xx, especially the return value evaluation has been improved
- Improve UART communication with MPPT and DPM86xx
- Improve MQTT implementation
- Bug fixes

## [0.81.07] - 2024-03-29

- Remove LED initialisation from main setup, as LED will lead to ESP reboot

- Enable CRC Check for the Text frames of MPPT

## [0.81.06] - 2024-03-28

- The communication method with the power supply units. So values are more stable and do not ha such value outliers.
  In addition, the status is read out and the corresponding values are only accepted depending on this.
  Furthermore filter now that voltage and current values.

- Instead of setting a constant value for the PSU voltage, it is now set 1.0V below the battery voltage.
  This seems to make the PSUs more stable.

- Fix issue where incorrect values were supplied to the CA and thus the consumption power was not taken into account,
  with the result that the feed-in was higher than the consumption

- Introduce new state with unlimited feed-in while charging and adjust voltage limits for transition between the states.

## [0.81.05] - 2024-03-24

- Version which was used during commissioning and works with a certain degree of success.
  This version is the basis of the firmware in operation; all subsequent changes should be documented and justified.

- This version has not been extensively tested and should only be used under constant monitoring.

<!-- Links -->

[keep a changelog]: https://keepachangelog.com/en/1.0.0/
[semantic versioning]: https://semver.org/spec/v2.0.0.html

<!-- Versions -->
