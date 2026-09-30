# pulse-pipeline

A bench instrument and analysis pipeline that measures two properties of a
supercapacitor from its response to a switched load:

- **Equivalent series resistance (ESR)** — from the instantaneous voltage step
  at the moment current begins to flow.
- **Capacitance** — from the charge delivered during discharge divided by the
  resulting voltage change.

A timed load pulse is applied across the supercapacitor while an oscilloscope
records the terminal voltage and the current-sense voltage. A Python pipeline
converts those waveforms into the two values, runs integrity checks on the data,
and stores a traceable per-pulse record.

The same circuit is modeled in NGspice. Because the model's ESR and capacitance
are set by the author, the analysis pipeline can be validated against known
values before any hardware is built: if the pipeline recovers the numbers put
into the model, it is working.

The measurement technique is the same one used in battery pulse testing (HPPC).
A supercapacitor is used here because its true ESR and capacitance are datasheet
constants, which provides a reliable reference for validating the pipeline.

## Purpose

Test engineering for batteries and energy storage depends on measuring internal
resistance and capacity accurately and repeatably. That requires more than a
meter: a controlled test fixture, calibrated instrumentation, a data pipeline
that preserves traceability, and a way to verify the pipeline itself.

This project builds that stack in miniature on a device whose true values are
known, so each layer — fixture, capture, analysis, storage — can be checked
independently.
