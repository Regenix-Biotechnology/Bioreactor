# Sensor telemetry over serial

The firmware sends one structured sensor snapshot per second on the main
serial port (`Serial`, 115200 baud). Each message is one newline-terminated
ASCII line. The interface should parse lines beginning with `SENSORS;` and
ignore the human-readable debug lines that follow.

Example:

```text
SENSORS;PH=7.12;PH_STATUS=0;PH_VALID=1;PH_AGE_MS=145;DO=83.47;DO_TEMP_C=36.92;DO_STATUS=0;DO_VALID=1;DO_AGE_MS=310
```

Fields:

| Field | Meaning |
|---|---|
| `PH` | Latest pH value, formatted to two decimal places. |
| `PH_STATUS` | Numeric `eAtlasStatus` from `AtlasBase.h`; `0` is `ATLAS_STATUS_OK`. |
| `PH_VALID` | `1` when the pH status is OK and its latest successful reading is no older than 15 seconds; otherwise `0`. |
| `PH_AGE_MS` | Age of the last successful pH reading in milliseconds; `4294967295` means there has not been a successful reading. |
| `DO` | Latest dissolved-oxygen value, formatted to two decimal places in the VisiFerm's configured unit (usually % saturation). |
| `DO_TEMP_C` | Latest temperature reported by the VisiFerm, in °C. |
| `DO_STATUS` | Numeric `eVisiFermStatus`; `0` is `VISIFERM_STATUS_OK`. |
| `DO_VALID` | `1` when the DO status is OK and the last complete DO/temperature reading is no older than 5 seconds; otherwise `0`. |
| `DO_AGE_MS` | Age of the last complete DO/temperature read in milliseconds; `4294967295` means no complete reading has succeeded. |

Use the `*_VALID` fields to decide whether to display values as current; the
numeric values may still contain the last reading (or the driver's initial
zero) when the corresponding validity field is `0`.
