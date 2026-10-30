# Culture setpoints over serial

The culture interface persists the full culture record in its database. This
firmware command carries only values currently supported by the firmware:
temperature limits and one target speed for each culture pump. pH and
dissolved-oxygen limits are not part of this command because the firmware has
no automatic pH or dissolved-oxygen controller.

## Command

Send one ASCII line terminated by `\n`, at the configured serial baud rate:

```text
CULTURE=TEMP_MIN:36.5,TEMP_MAX:37.5,CIRCULATION_PUMP:75,CULTURE_PUMP_1:20,CULTURE_PUMP_2:20;ID=42
```

`ID` is a required unsigned 32-bit decimal request identifier. Use a new ID for
each request and match it against the response ID. Fields are required exactly
once and in the order shown. Numeric values use a decimal point; exponent
notation is accepted by the firmware parser.

| Field | Unit | Meaning |
|---|---|---|
| `TEMP_MIN`, `TEMP_MAX` | °C | Inclusive lower and upper culture temperature limits. The firmware uses their arithmetic midpoint as the temperature controller target while in `APPROV`, `PREPARE`, or `RUN`. |
| `CIRCULATION_PUMP` | ml/min | Fixed circulation-pump speed, applied only in `RUN`. |
| `CULTURE_PUMP_1` | ml/min | Fixed culture pump 1 speed, applied only in `RUN`. |
| `CULTURE_PUMP_2` | ml/min | Fixed culture pump 2 speed, applied only in `RUN`. |

Temperature values must be finite and `TEMP_MIN <= TEMP_MAX`. Pump speeds must
be finite and in `[0, 255]` ml/min. The supply/approvisionnement pump keeps its
existing `RUN` speed of 5 ml/min. Other pump profiles are unchanged.

The command is accepted only while the bioreactor is `IDLE`. It stages and
persists the values; it does not change the state or immediately command the
culture pumps. After it is acknowledged, the interface may start the culture
with the existing state command, for example `STATE=APPROV;ID=43\n`.

## Responses

Success:

```text
OK:CULTURE;STATUS=STAGED;ID=42
```

This confirms parsing, validation, and persistence—not actuator motion. The
temperature midpoint is selected on entry to a culture state. Culture pump
speeds are sent by the normal `RUN` loop only.

Errors:

| Response | Meaning |
|---|---|
| `ERR:CULTURE:ID_REQUIRED` | The request omitted its ID. |
| `ERR:CULTURE:FORMAT;ID=42` | A field is missing, duplicated, misordered, or not a finite number. |
| `ERR:CULTURE:RANGE;ID=42` | Temperature bounds are reversed or a pump speed is outside `[0, 255]` ml/min. |
| `ERR:CULTURE:STATE;STATE=3;ID=42` | The command was sent outside `IDLE`; `STATE` is the current numeric firmware state. |
| `ERR:CULTURE:STORAGE;ID=42` | The values could not be persisted. |
| `ERR:COMMAND:INVALID_ID` | The request ID suffix is malformed. |

The interface should show success only for `OK:CULTURE` with the matching ID.
Show a transmission/firmware error for a matching `ERR:CULTURE` response or
when no matching response arrives before the interface's serial timeout.

## State behavior

`CULTURE=...` does not cause a state transition and is authorized only in
`IDLE`. The intended launch sequence is:

```text
IDLE -- CULTURE=... --> IDLE -- STATE=APPROV --> APPROV
     -- (firmware timer) --> PREPARE -- STATE=RUN --> RUN
```

The firmware automatically advances `APPROV` to `PREPARE` after its configured
duration. `PREPARE` to `RUN` is requested using `STATE=RUN`; `RUN` does not
automatically advance to the next state. State changes are otherwise requested
using the existing `STATE=...` command. Currently, that command accepts any
supported non-cleaning state rather than enforcing a complete linear culture
transition graph. Cleaning states cannot be entered with `STATE=`; they use
the guarded `CLEANING=...` commands.

Culture temperature target is restored to the saved base `TEMP` reference
outside `APPROV`, `PREPARE`, and `RUN`. Culture pump targets are read only in
`RUN`; on leaving `RUN`, the next state's existing pump profile takes over.
The culture configuration remains stored so the same active culture can resume
with it. The interface should send a fresh `CULTURE=...` command from `IDLE`
for each new culture.

pH and dissolved-oxygen bounds may remain in the interface/database, but must
not be sent as firmware-controlled setpoints until corresponding regulation
is implemented.
