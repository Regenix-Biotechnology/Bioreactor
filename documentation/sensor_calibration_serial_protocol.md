# Sensor calibration over serial

The firmware exposes pH and dissolved-oxygen (DO) calibration commands over
the main serial interface. Send one ASCII command per line, terminated by
`\n`, at `115200` baud. Append a unique unsigned 32-bit decimal request ID to
each command; IDs are optional in firmware but recommended so the interface
can match replies and asynchronous events:

```text
CALIB-PH=7;ID=42
```

Replies and events include `;ID=42` when the request includes an ID.

## pH calibration

Available commands:

| Command | Calibration point |
|---|---:|
| `CALIB-PH=4` | pH 4.00 |
| `CALIB-PH=7` | pH 7.00 |
| `CALIB-PH=10` | pH 10.00 |

Calibrate pH 7.00 first. The Atlas EZO-pH calibration at 7.00 clears other
calibration points; after that, calibrate pH 4.00 and/or 10.00 as needed.
The firmware blocks while sending the command, so the interface should allow
several seconds for a response.

Successful command transmission:

```text
OK:CALIB-PH=7;ID=42
EVENT:CALIB-PH:REQUEST_SENT;POINT=7;ID=42
```

Transmission failure:

```text
ERR:CALIB-PH:SEND_FAILED;STATUS=3;ID=42
```

`OK` means the calibration command was sent over I2C; this driver does not
read back an Atlas calibration result, so the response does not confirm that
the sensor accepted or completed calibration. The interface should treat the
matching `OK` as command delivery, not as verified calibration success.

## DO calibration (Hamilton VisiFerm RS485)

Available commands:

| Command | Calibration point | Required calibration medium |
|---|---|---|
| `CALIB-DO=0` | CP1, 0% | Oxygen-free medium |
| `CALIB-DO=100` | CP2, 100% | Air or air-saturated water, following the sensor procedure |

Keep the sensor stable in the calibration medium for at least 3 minutes before
starting. The firmware runs the Modbus calibration sequence asynchronously;
the interface must wait for a terminal `SUCCESS` or `FAILED` event rather
than treating the initial `OK` as completion. On sensor rejection for an
unstable phase, a retry may be reported about 110 seconds later.

Calibration is accepted only while the bioreactor is in `IDLE` or `TEST`, and
only one DO calibration may run at a time.

Example request and immediate acknowledgment:

```text
CALIB-DO=100;ID=43
OK:CALIB-DO=100;ID=43
```

Terminal success:

```text
EVENT:CALIB-DO:SUCCESS;POINT=100;VALUE=100.00;ID=43
```

Retry notification (the operation is still pending):

```text
EVENT:CALIB-DO:WAITING_RETRY;POINT=100;DELAY_S=110;ID=43
```

Terminal failure examples:

```text
EVENT:CALIB-DO:FAILED;POINT=100;ERROR=REJECTED;REASON=PHASE_NOT_STABLE;ID=43
EVENT:CALIB-DO:FAILED;POINT=100;ERROR=MODBUS_EXCEPTION;EXCEPTION=2;ID=43
```

Command-level errors:

| Response | Meaning |
|---|---|
| `ERR:CALIB-DO:STATE_REQUIRED;STATE=...;ID=43` | The bioreactor is not in `IDLE` or `TEST`. |
| `ERR:CALIB-DO:ALREADY_RUNNING;ID=43` | Another DO calibration is already active. |

The UI should correlate all lines by ID, show progress after `OK`, keep the
operation pending after `WAITING_RETRY`, and report completion only on a
matching `SUCCESS` or `FAILED` event. Use a sufficiently long event-wait
period; a calibration may take over 110 seconds when a retry is needed.
