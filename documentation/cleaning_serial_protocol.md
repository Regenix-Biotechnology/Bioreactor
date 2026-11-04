# Cleaning cycle serial protocol

Cleaning commands are ASCII lines terminated by `\n`, sent at the configured
115200 baud rate. The firmware does not physically detect whether detergent or
water has been added; the waiting-state confirmations are operator acknowledgments.

## Detergent confirmation

Start by placing the firmware in a safe waiting state:

```text
CLEANING=DETERGENT,WAITING_FOR_DETERGENT
```

The firmware replies `OK:CLEANING=DETERGENT,WAITING_FOR_DETERGENT` and emits
`EVENT:CLEANING:WAITING_FOR_DETERGENT_CONFIRMATION`. Confirm the detergent has
been added by sending:

```text
CLEANING=DETERGENT,APPROV
```

This second command starts the detergent cycle, provided cleaning temperature
sensors are ready. The command is rejected with
`ERR:CLEANING:TEMPERATURE_SENSOR_NOT_READY` otherwise.

The initial waiting command is accepted only from `IDLE`. The confirmation is
accepted only while the firmware is waiting for detergent. To cancel either
waiting state, send `CLEANING=STOP`.

## Water confirmation

After detergent completes, the firmware enters `CLEANING_WAITING_WATER`.
After adding water, confirm and start rinse with:

```text
CLEANING=RINSE,APPROV
```

The rinse command is accepted only from the waiting-for-water state. Rinse is
followed automatically by the dry cycle.

`STATUS?` reports `STATUS:WAITING_FOR_DETERGENT=yes|no` and
`STATUS:WAITING_FOR_WATER=yes|no`. The interface should treat the matching
`OK:` response as command acceptance, and should display a matching
`ERR:CLEANING:` response as failure.
