# PVZero

With an ESP32, the power grid data of a **Shelly 3EM** is queried, processed and used for current limiting with a
**DPM86xx** power supply unit, so that a zero-current feed-in of a PV system can be realized.

## Arduino Framework

The project is based on [Arduino Framework](https://www.arduino.cc/reference/en) and is developed in
[VS Code](https://code.visualstudio.com/) using the [PlatformIO IDE](https://platformio.org/) extension.

### Favicon.ico

Upload a **favicon.ico** with `pio run --target uploadfs`

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

## Battery guard

Der Batteriewächter soll sicher stellen, dass die Batterie

1. während des Tages nicht entladen wird
1. niemals vollständig entladen wird
1. 1x in zwei Wochen vollständig geladen wird

Um diese Aufgaben zu meisten, werden 3 Eingangsgrößen herangezogen:

1. **Datum und Uhrzeit**: Bereitgestellt aus dem Internet, werden diese jedes Mal bei einer Vollladung gespeichert.
   Damit ermittelt der Batteriewächter den nächsten Zeitpunkt für das Vollladen der Batterie.

1. **Charge Voltage**: Es gibt zwei Quellen für diesen Wert

   - Messung der Versorgungsspannung des Netzteils mit dem ADC des ESP32
   - Wert wird aus dem Victron SmartSolar über das USART VE.Direct Protokoll gelesen

   Die **Charge Voltage** wird ständig gemessen und für die Ermittlung der Zustände _charged_ und _discharged_
   verwendet.

1. **Charge Current**: Es gibt eine Quelle für diesen Wert

   - Wert wird aus dem Victron SmartSolar über das USART VE.Direct Protokoll gelesen

   Die **Charge Current** wird ständig ausgelesen und wird die Ermittlung der Zustände _charging_ und _discharging_
   verwendet.

Aus den genannten Eingangsgrößen wir eine Ausgangsgröße bestimmt:

1. **Feed-in limited DC Current**: Maximaler Stromwert für das Einspeisen in den Wechselrichter bzw. ins Stromnetz.

Ausgehend von den genannten Aufgaben, den gegeben Eingangsgrößen und den möglichen Zuständen ergibt sich folgender
Zustandsautomat des Batteriewächters.

![processing](./docs/images/battery_guard_sm.drawio.svg)

Beschreibung der Zustände:

- **charge**: Der Strom wird auf den Wert **Charge Current** vom MPPT begrenzt, solange die Bedingung 5. oder 7. oder 9. nicht erfüllt ist
- **charge and discharge**: Der Strom wird nicht begrenzt, solange die Bedingung 1. oder 8. nicht erfüllt ist
- **charge until charged**: Der Strom wird auf den Wer 0.0 A begrenzt, solange die Bedingung 10. nicht erfüllt ist
- **charged**: Der Strom wird nicht begrenzt und der Zeitstempel wird gespeichert, solange die Bedingung 2. nicht erfüllt ist
- **discharge**: Der Strom wird nicht begrenzt, solange die Bedingung 3. oder 5. nicht erfüllt ist
- **discharged**: Der Strom wird auf den Wer 0.0 A begrenzt, solange die Bedingung 4. nicht erfüllt ist

Beschreibung der Zustandsübergänge:

1. **Charge Voltage** >= CHARGE_CUTOFF_VOLTAGE (58.4 V)

2. **Charge Voltage** < (CHARGE_CUTOFF_VOLTAGE - 0.4V) (58.0 V)

3. **Charge Voltage** <= (DISCHARGE_VOLTAGE) (42.0 V)

4. (**Charge Voltage** > (ABSORPTION_VOLTAGE) (51.2 V)) && (**Charge Current** > 0.1 A)

5. **Charge Current** < 0.2 A

6. **Charge Current** > 0.5 A

7. **Charge Voltage** >= (CHARGE_CUTOFF_VOLTAGE - 5.0 V)

8. **Charge Voltage** <= ABSORPTION_VOLTAGE (51.2)

9. (Zeitstempel - SavedZeitstempel) > 2 Wochen

10. **Charge Voltage** >= CHARGE_CUTOFF_VOLTAGE (58.4 V)

Um festzustellen ob die Batterie geladen oder entladen wird, is der **Charge Current** Wert vom Victron SmartSolar
erforderlich.

Nur mit dem Wert **Charge Voltage** verschmelzen die Beiden Zustände _charging_ und _discharging_ zu einem und
es kann lediglich nur eine Sicherheitsabschaltung für das Entladen der Batterie realisiert werden. Zudem Muss eine
Entscheidung getroffen werden, wann die Einspeisung wieder eingeschaltet werden soll.

### Sicher Zustand

Die Batterieüberwachung basiert auf **Charge Voltage** und **Charge Current**. Fehlt der Parameter für Strom, kann
nur eine Abschaltung, also Begrenzung des Stroms auf 0.0A nur bei geringer Spannung erfolgen. Ist neben dem Strom auch
der Spannungswert nicht vorhanden ist Batterieüberwachung nicht möglich und der Strom wird nicht begrenzt.
