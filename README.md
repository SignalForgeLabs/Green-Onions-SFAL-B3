# Green Onions — Classic ToneWheels (Hammond B-3 + Leslie 122)

**Signal Forge Audio Labs** — Physical-modeling Hammond B-3 Tonewheel Console + Accutronics Spring Reverb + Leslie 122 Rotary Speaker VST3i / Standalone instrument in C++20 & JUCE.

## DSP Signal Flow

```text
[MIDI Channels 1 / 2 / 3] ──► [9-Busbar Key Matrix + Organic Key Click]
                                      │
          ┌───────────────────────────┼───────────────────────────┐
          ▼                           ▼                           ▼
   [Upper Manual (Ch 1)]       [Lower Manual (Ch 2)]       [Pedals (Ch 3)]
   9 Drawbars + Percussion     9 Drawbars                  2 Drawbars (16', 8')
          │                           │                           │
          └───────────────────────────┼───────────────────────────┘
                                      ▼
                     [91-Tonewheel Bank + Leakage/Crosstalk]
                                      │
                   [18-Stage Scanner Vibrato / Chorus (V1-V3, C1-C3)]
                                      │
                    [AO-28 Tube Preamp + Expression Swell Contour]
                                      │
                     [Accutronics 3-Spring Reverb Tank (Pre-Leslie)]
                                      │
               [Leslie 122: 800Hz LR4 Crossover -> Horn & Drum Doppler/AM]
```

## Key Features

- **91 Free-Running Tonewheels**: Continuously spinning phase accumulators with authentic Hammond B-3 synchronous gear tooth ratios (`A4 = 440.000 Hz` exact, authentic mechanical detuning on the other 11 pitch classes), top & bottom manual foldback, complex 5-harmonic pedal wheels (`C1–B1`), and bin/loom crosstalk leakage.
- **9-Contact Key Matrix & Organic Key Click**: Randomized `0.2–2.5 ms` palladium contact stagger per key + bandpass-shaped DC-offset transient thump (zero static samples).
- **Monophonic Single-Trigger Harmonic Percussion**: Upper Manual 2nd (`4'`) and 3rd (`2-2/3'`) harmonics, Normal/Soft volume, Fast (~150 ms) / Slow (~500 ms) decay, authentic `1'` drawbar steal, and ~3 dB Normal-mode drawbar dip.
- **18-Stage All-Pass Scanner Vibrato / Chorus**: ~6.9 Hz tri-phase mechanical scanner LFO, `V1–V3` and `C1–C3` modes, independent Upper/Lower V/C routing (percussion & pedals bypass the scanner).
- **AO-28 Tube Preamp & Variable-Capacitor Swell Pedal**: Loudness-contoured expression pedal (`CC 11`) feeding dual-stage asymmetric 6AU6/12AX7 tube saturation.
- **Accutronics 3-Spring Reverb Tank (Pre-Leslie)**: 3 parallel prime-length spring paths with 4-stage dispersive all-pass chains, fed into the Leslie rotary speaker.
- **Leslie 122 Rotary Speaker**: 4th-order Linkwitz-Riley 800 Hz crossover, independent Horn (~0.7s up / ~1.0s down) & heavy wooden Bass Drum (~4.5s up / ~5.5s down) inertia curves, stereo virtual microphone Doppler + directional cardioid AM + moving off-axis baffle filter + cabinet reflections.

## MIDI Mapping

| Channel / CC | Function |
|---|---|
| Channel 1 | Upper Manual (61 keys, C2–C7) |
| Channel 2 | Lower Manual (61 keys, C2–C7) |
| Channel 3 | 2-Octave Pedalboard (25 keys, C2–C4) |
| CC 1 (Mod Wheel) | Leslie Speed (Chorale / Brake / Tremolo) |
| CC 64 (Sustain) | Toggle Leslie Chorale ↔ Tremolo |
| CC 11 | Expression / Swell Pedal |
| CC 12–20 | Upper Drawbars (`16'` .. `1'`) |
| CC 21–29 | Lower Drawbars (`16'` .. `1'`) |
| CC 30–31 | Pedal Drawbars (`16'`, `8'`) |

*(Enable the **MIDI SPLIT** rocker switch in the top bar to play Pedals + Lower + Upper from a single Channel 1 controller.)*

## Building

```bash
# Configure (fetches JUCE 8.0.4 automatically, or pass -DJUCE_DIR=/path/to/JUCE)
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release

# Build VST3, Standalone, and DSP Tests
cmake --build build --parallel

# Run DSP Verification Tests
ctest --test-dir build --output-on-failure
```

Build outputs:
- Standalone: `build/GreenOnions_artefacts/Release/Standalone/Green Onions`
- VST3: `build/GreenOnions_artefacts/Release/VST3/Green Onions.vst3`
