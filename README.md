# PVZero

With an ESP32, the power grid data of a **Shelly 3EM** is queried, processed and used for current limiting with a
**DPM86xx** power supply unit, so that a zero-current feed-in of a PV system can be realized.

## Arduino Framework

The project is based on [Arduino Framework](https://www.arduino.cc/reference/en) and is developed in
[VS Code](https://code.visualstudio.com/) using the [PlatformIO IDE](https://platformio.org/) extension.

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

1. **Battery Voltage**: Es gibt zwei Quellen für diesen Wert

   - Messung der Versorgungsspannung des Netzteils mit dem ADC des ESP32
   - Wert wird aus dem Victron SmartSolar über das USART VE.Direct Protokoll gelesen

   Die **Battery Voltage** wird ständig gemessen und für die Ermittlung der Zustände _charged_ und _discharged_
   verwendet.

1. **Battery Current**: Es gibt eine Quelle für diesen Wert

   - Wert wird aus dem Victron SmartSolar über das USART VE.Direct Protokoll gelesen

   Die **Battery Current** wird ständig ausgelesen und wird die Ermittlung der Zustände _charging_ und _discharging_
   verwendet.

Aus den genannten Eingangsgrößen wir eine Ausgangsgröße bestimmt:

1. **Feed-in limited DC Current**: Maximaler Stromwert für das Einspeisen in den Wechselrichter bzw. ins Stromnetz.

Ausgehend von den genannten Aufgaben, den gegeben Eingangsgrößen und den möglichen Zuständen ergibt sich folgender
Zustandsautomat des Batteriewächters.

![processing](./docs/images/battery_guard_sm.drawio.svg)

Beschreibung der Zustände:

- **charging**: Der Strom wird auf den Wert **Battery Current** vom Victron SmartSolar begrenzt
- **charged**: Der Strom wird nicht begrenzt, Zeitstempel wird gespeichert
- **discharging**: Der Strom wird nicht begrenzt
- **discharged**: Der Strom wird auf den Wer 0.0 A begrenzt, Einspeisung wird eingestellt

Beschreibung der Zustandsübergänge:

1. **Battery Voltage** >= CHARGE_CUTOFF_VOLTAGE (58.4 V)

2. **Battery Voltage** < CHARGE_CUTOFF_VOLTAGE (58.4 V)

3. **Battery Voltage** <= DISCHARGE_VOLTAGE (40.0 V)

4. **Battery Voltage** > DISCHARGE_VOLTAGE (40.0 V)

5. (**Battery Current** == 0.0 A) && (Zeitstempel < 2 Wochen)

6. **Battery Current** > 0.0 A

Um festzustellen ob die Batterie geladen oder entladen wird, is der **Battery Current** Wert vom Victron SmartSolar
erforderlich.

Nur mit dem Wert **Battery Voltage** verschmelzen die Beiden Zustände _charging_ und _discharging_ zu einem und
es kann lediglich nur eine Sicherheitsabschaltung für das Entladen der Batterie realisiert werden. Zudem Muss eine
Entscheidung getroffen werden, wann die Einspeisung wieder eingeschaltet werden soll.

### Sicher Zustand

Die Batterieüberwachung basiert auf **Battery Voltage** und **Battery Current**. Fehlt der Parameter für Strom, kann
nur eine Abschaltung, also Begrenzung des Stroms auf 0.0A nur bei geringer Spannung erfolgen. Ist neben dem Strom auch
der Spannungswert nicht vorhanden ist Batterieüberwachung nicht möglich und der Strom wird nicht begrenzt.
