# Cleaning cycle serial protocol

Cleaning commands are ASCII lines terminated by `\n`, sent at the configured
115200 baud rate. The firmware does not physically detect whether detergent or
water has been added; the waiting-state confirmations are operator acknowledgments.

## Sequence (since 2026-11-23)

1. Water pre-rinse
2. Detergent cycle
3. Water rinse

Each cycle runs APPROV → CIRCULATION → RETURN. The heater is only used during
the detergent cycle. There is no separate dry cycle anymore: the RETURN of the
final water rinse empties the circuit.

| Cycle | APPROV | CIRCULATION | RETURN | Total |
|---|---|---|---|---|
| Pre-rinse (water) | 12 min | 15 min | 12 min | 39 min |
| Detergent | 7 min | 40 min | 7 min | 54 min |
| Rinse (water) | 12 min | 15 min | 12 min | 39 min |

Total: 2 h 12 min plus the operator confirmations.

## Start: water for the pre-rinse

Start from `IDLE` by placing the firmware in a safe waiting state:

```text
CLEANING=PRE_RINSE,WAITING_FOR_WATER
```

The firmware replies `OK:CLEANING=PRE_RINSE,WAITING_FOR_WATER` and emits
`EVENT:CLEANING:WAITING_FOR_WATER;CYCLE=PRE_RINSE;PHASE=WAITING_FOR_WATER` and
`EVENT:CLEANING:WAITING_FOR_WATER_CONFIRMATION`. After adding water, start the
pre-rinse with:

```text
CLEANING=RINSE,APPROV
```

(`CLEANING=PRE_RINSE,APPROV` is accepted too.) The firmware emits
`EVENT:CLEANING:CYCLE_START;CYCLE=PRE_RINSE;PHASE=APPROV`.

The former start command `CLEANING=DETERGENT,WAITING_FOR_DETERGENT` is no longer
accepted from `IDLE` (`ERR:CLEANING:INVALID_TRANSITION`).

## Detergent confirmation

When the pre-rinse ends, the firmware emits `EVENT:CLEANING:PRE_RINSE_COMPLETE`,
enters `CLEANING_WAITING_DETERGENT` and emits
`EVENT:CLEANING:WAITING_FOR_DETERGENT;CYCLE=DETERGENT;PHASE=WAITING_FOR_DETERGENT`
and `EVENT:CLEANING:WAITING_FOR_DETERGENT_CONFIRMATION`. Confirm the detergent
has been added by sending:

```text
CLEANING=DETERGENT,APPROV
```

This command starts the detergent cycle, provided cleaning temperature sensors
are ready. The command is rejected with
`ERR:CLEANING:TEMPERATURE_SENSOR_NOT_READY` otherwise. To cancel any waiting
state, send `CLEANING=STOP`.

## Water confirmation for the final rinse

After detergent completes, the firmware enters `CLEANING_WAITING_WATER` again
(`CYCLE=DETERGENT`). After adding water, confirm and start the rinse with:

```text
CLEANING=RINSE,APPROV
```

When the rinse RETURN ends, the firmware emits `EVENT:CLEANING:RINSE_COMPLETE`,
goes back to `IDLE` and emits `EVENT:CLEANING:COMPLETE`.

`STATUS?` reports `STATUS:WAITING_FOR_DETERGENT=yes|no` and
`STATUS:WAITING_FOR_WATER=yes|no`. The interface should treat the matching
`OK:` response as command acceptance, and should display a matching
`ERR:CLEANING:` response as failure.


## Related states (outside the cleaning sequence)

- `STATE=CELL-RETURN` (state 4, **END** in the interface): same outputs as the
  cleaning RETURN phase (pumps −100 / −100 / −50 / −50 mL/min, circulation and
  cleaning valves open, heater off) for 12 min, then `EVENT:END:COMPLETE` and
  `IDLE`. It replaces the former CELL-RETURN → TRASH chain (`STATE=TRASH`
  still exists but is no longer chained automatically).
- `STATE=ASSEMBLY` (state 22): assembly / disassembly of the bioreactor before
  or after the autoclave. The three process valves (supply, circulation,
  cleaning) are open; pumps, heater, fans and gas valves are off. It stays in
  this state until another state is sent (e.g. `STATE=IDLE`), and is restored
  after a reboot.
