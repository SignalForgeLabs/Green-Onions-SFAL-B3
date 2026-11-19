# Stage PNG Sprite Assets (Optional Drop-In Pipeline)

Drop any of the following transparent `.png` files into `Resources/Stage/` and re-run `cmake --build build --parallel`.
CMake automatically embeds them via JUCE `BinaryData` (`SFA_HAS_STAGE_PNG_ASSETS=1`). Any sprite not present on disk automatically falls back to the built-in procedural renderer.

| Filename | Layer / Element |
|---|---|
| `stage_maple_topdown.png` | Top-Down maple floor & black curtains backdrop |
| `stage_maple_front.png` | Front Stage maple floor, footlights & curtains backdrop |
| `b3_console_top.png` | Top-Down Hammond B-3 console |
| `b3_console_front.png` | Front Stage Hammond B-3 console |
| `leslie_122_top.png` | Top-Down Leslie 122 cabinet |
| `leslie_122_front.png` | Front Stage Leslie 122 cabinet |
| `leslie_147_top.png` | Top-Down Leslie 147 cabinet (falls back to `leslie_122_top.png`) |
| `leslie_147_front.png` | Front Stage Leslie 147 cabinet (falls back to `leslie_122_front.png`) |
| `marshall_bluesbreaker_top.png` | Top-Down Marshall 1962 Bluesbreaker 2×12 combo |
| `marshall_bluesbreaker_front.png` | Front Stage Marshall 1962 Bluesbreaker 2×12 combo |
| `mic_u47.png` | Neumann U47 capsule sprite |
| `mic_c414.png` | AKG C414 capsule sprite |
| `mic_r121.png` | Royer R-121 ribbon capsule sprite |
| `mic_sm57.png` | Shure SM57 dynamic capsule sprite |
