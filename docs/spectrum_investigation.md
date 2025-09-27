# ExpressLRS Spectrum Telemetry Investigation

_Last updated: 2025-09-27_

## 1. What the official docs confirm

- ExpressLRS exposes only two telemetry packet types today: **LINK** (scalar link statistics such as RSSI, LQ, SNR, TX power, voltage) and **DATA** (flight-controller payloads sharing bandwidth with MSP).  
- The default LINK cadence is one packet every **512 ms (~2 Hz)**. Faster rates require "burst mode", which steals RC bandwidth.  
- Telemetry bandwidth is limited (sub-kbps at most ratios). Large payloads must be chunked carefully and should run on higher serial baud rates (≥400 kbps) to avoid overruns.  
- Continuous MSP polling triggers burst mode and is discouraged in flight. Spectrum capture must therefore be a ground-testing feature.

## 2. Practical implications for our F2 screen

- There is **no documented native spectrum/FFT telemetry**. Any bytes beyond the core link stats are unofficial extensions. Our current synthetic bar graph stays necessary until we confirm a real payload.  
- Polling for extra data now runs at a **1 Hz cadence** to keep the radio link healthy while still giving the UI fresh bins.  
- The desktop marks spectrum data stale if no bins arrive within ~1.5 s, falling back to the synthetic model automatically.
- A realistic sweep (e.g., 96–128 bins) already consumes most of an MSP frame. We must plan for dedicated opcodes and optional chunking.

## 3. Investigation plan

1. **Protocol reconnaissance**  
   - Capture the USB serial stream (RealTerm or logic analyser) while using both the ExpressLRS Lua script and our demo tool.  
   - Inspect the ELRS firmware sources and changelogs for hidden telemetry options.  
   - Ask the ExpressLRS community (Discord/GitHub) whether spectrum telemetry exists or has been prototyped.

2. **If no spectrum telemetry exists**  
   - Design a custom MSP opcode (`MSP_ELRS_SPECTRUM` placeholder) with a predictable payload layout and optional chunking.  
   - Implement the request in the transmitter firmware (SuperG Nano) and throttle responses to ≤2 Hz.  
   - Document payload structure (frequency axis, bin resolution, units).

3. **PC-side integration path**  
   - Extend `MspCommands` to send the new spectrum request (alternating with regular link stats). ✅ (Host now sets `status` bit 0 on `MSP_ELRS_TELEMETRY_PUSH` requests via a dedicated spectrum thread.)  
   - Enhance `TelemetryHandler::processTelemetryFrame` to parse bin arrays and update `RadioState`. ✅ (Full MSP deframer in place; appends bins beyond the canonical link stats tuple.)  
   - Refresh the F2 spectrum panel to consume real bins when fresh, falling back to the synthetic model otherwise. ✅ (Live badge + freshness timestamp in place.)  
   - Surface clear UI feedback (timestamp, stale indicator, “Live Spectrum” badge). ✅

4. **Validation & safety**  
   - Bench test with a known RF source and, if possible, cross-check against a spectrum analyser.  
   - Guard spectrum polling behind an “on the bench” toggle to avoid burst mode mid-flight.  
   - Log when spectrum data is missing or stale and expose troubleshooting tips.

## 4. Supporting assets

- `samples/elrs_otg_demo_single.cpp` — stand-alone Windows C++17 demo that opens the ELRS COM port, deframes MSP/CRSF, parses the placeholder link-stats+spectrum payload, and maintains an in-memory `RadioState`. Use it to validate firmware behaviour before wiring changes into the main codebase.

## 5. Next immediate steps

1. Run the sample tool against a real SuperG Nano (ELRS v3.x) to confirm current telemetry payloads.  
2. Record and catalogue any undocumented bytes that appear after the four scalar link-stat fields.  
3. Decide whether to push for a firmware change (custom MSP opcode) or to rely on existing data.  
4. Once the telemetry path is confirmed, proceed to rework the F2 screen around real data capture.

## 6. Firmware handshake expectations (new)

- **Request flag**: When the desktop app toggles spectrum polling, it sends `MSP_ELRS_TELEMETRY_PUSH` with `status` bit 0 set (`0x01`). Treat this as “include FFT bins with the next reply”.
- **Response layout**: Reuse the existing link-stats frame header (`deviceId`, `handsetId`, `fieldId`, `status`/flags, RSSI1, RSSI2, LQ, SNR, TX Power, etc.) and append the bin array (`uint8_t binCount`, followed by `binCount` bytes) or simply stream raw bins after the canonical 10-byte tuple. The current host parser accepts either `length`-based trailing payload.
- **Cadence**: Match the host’s 500 ms poll interval. Skip bin emission if the flag is clear or the radio is in-flight to prevent burst-mode starvation.
- **Cadence**: Host requests bins every **1000 ms**; mirror that in firmware and drop bins when the flag is clear or the radio is in-flight to prevent burst-mode starvation.
- **Graceful fallback**: If firmware cannot deliver bins, send the classic 10-byte frame; the UI automatically falls back to the synthetic spectrum model and marks the data as stale.
