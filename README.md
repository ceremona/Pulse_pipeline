# pulse-pipeline

A pulse-response measurement and analysis pipeline for an instrumented
supercapacitor test fixture — modeled in NGspice, captured with a Siglent
SDS804X HD over SCPI, and reduced to a single per-pulse summary table with
integrity checks and provenance.

This is a lab-notebook project. It is written to be read, not just run, and it
is explicit about what is simulation, what is instrument validation, and what is
measurement.

## Status

| Stage | State | Evidence |
|---|---|---|
| Analysis pipeline | **Validated** | Recovers injected ground-truth ESR (30.00 mΩ) and capacitance (10.0005 F) from the NGspice run; all 6 QA checks pass |
| NGspice fixture model | **Done** | Netlist generated from the fixture registry; runs in-notebook |
| Siglent SCPI capture path | **Planned** | Driver written against documented commands; awaiting first contact with hardware |
| Physical fixture + Arduino pulse controller | **Planned** | BOM, schematic, and firmware drafted; awaiting bench build |

*Edit this table as each stage completes. The repo and any résumé line must
always be in the same state — see "What is honestly claimable" at the bottom.*

## Why this exists

A companion to the battery-cycle-life notebook. That project analyzed public
Li-ion cycling data and hit a wall it stated plainly: DCIR estimated from the
discharge-onset voltage step was sparse, because the public recordings start
with the load already applied. A proper HPPC test pulses in both directions and
reads the ohmic drop at pulse onset.

So this project makes the measurement the other notebook needed. Same analysis
functions, better data structure — and a fixture simple enough to build on a
bench and honest enough to defend.

## Architecture

Three data sources, one analysis pipeline. The model and the instrument never
talk to each other; the notebook is the bridge.
