# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog],
and this project adheres to [Semantic Versioning].

## [Unreleased]

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
