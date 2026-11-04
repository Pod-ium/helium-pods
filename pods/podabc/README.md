# PodABC

Letters and numbers for young children on the **ClockworkPi PicoCalc**, entirely in **Italian**:
the PicoCalc speaks (a real recorded-style voice, not beeps), the letters fill the whole screen.
Runs on every Pico (Pico, Pico W, Pico 2, Pico 2 W); no SD card needed.

## The games

The menu has two big buttons. Up / Down choose (the PicoCalc says the name of the game,
for children who cannot read yet), Enter or Space start; 1 and 2 start a game directly.

| Game | What happens |
|---|---|
| **1 · Ascolta e trova** | The PicoCalc says a letter or a number (“Bi!”, “Sette!”). The child presses that key: the symbol fills the screen, a happy jingle plays and a “well done” (“Ottimo!”, “Molto bene!”, “Evviva!”, “Esatto!”). A wrong key changes nothing on screen: the PicoCalc says “Prova ancora!” and the symbol again. The next symbol comes only after the right key. Enter or Space say the symbol again; after 9 s without keys it is repeated (up to 3 times). Each symbol comes once before any repeats. |
| **2 · Premi e ascolta** | Every letter or number key pressed is shown on the whole screen and said aloud. |

**Esc and Up arrow held together** go back to the menu, from either game (one key alone does
nothing, so a child does not leave by chance).

**Impostazioni** (for the grown-ups): what game 1 asks (*Lettere e numeri*, *Solo lettere*,
*Solo numeri*), volume, light or dark background. They are saved in `/podabc/settings.txt`
when there is an SD card.

Names said for the letters: a, bi, ci, di, e, effe, gi, acca, i, i lunga, cappa, elle, emme,
enne, o, pi, cu, erre, esse, ti, u, vu, doppia vu, ics, ipsilon, zeta. Each symbol always has
the same colour.

## Installation

Copy `podabc.uf2` to the Pico in BOOTSEL mode, or put it in `pico2-apps/` (Pico 2 / 2 W) or
`pico1-apps/` (Pico / Pico W) of the SD card and start it from **UF2 Loader**.

## Building

From the repository root:

```sh
cmake -B build -DPICO_SDK_PATH=/path/to/pico-sdk      # PicoCalc + Pico 2 W (-DPD_BOARD=pico... for others)
cmake --build build -j --target podabc
# -> build/pods/podabc/podabc.uf2
```

On the PC (`cmake -B build-host -DPD_TERMINAL=host`) it opens in a window with sound (SDL2),
or runs a key script; `esc+up` holds Esc while Up is tapped:

```sh
PD_FRAMES=./frames PD_KEYS="wait:1500,shot,enter,wait:800,a,esc+up,wait:300,shot" ./build-host/pods/podabc/podabc
```

## How it works

- **Voice:** 45 clips (26 letters, 10 digits, 9 phrases) and the jingle, 24 s in all, stored as
  8-bit µ-law at 16 kHz (380 KB of flash) and played at 32 kHz (2× interpolated) so the PWM
  carrier is above hearing. Keys are read on core 1, and the audio is fed while the screen is
  drawn, so sound and picture start together.
- **Letters:** each symbol is a 16-level antialiased bitmap as large as fits in 304×304,
  run-length coded (110 KB). Drawn one row at a time, background included, so there is no
  flicker; the menu draws the same bitmaps scaled down with a box filter.
- **Assets:** `assets/voice.bin` and `assets/glyphs.bin` are committed; the build turns them into
  const arrays (`tools/embed.py`, Python 3). They are made by `tools/make_assets.py` from:
  - the Italian [Piper](https://github.com/rhasspy/piper) voice **it_IT-paola-medium**
    (as packaged by [sherpa-onnx](https://github.com/k2-fsa/sherpa-onnx/releases/tag/tts-models));
    its training data is the [Voice-Dataset-Italian](https://huggingface.co/datasets/paolapersico1/Voice-Dataset-Italian),
    see there for its terms;
  - the font **Andika Bold** by SIL International, designed for children learning to read
    ([SIL Open Font License 1.1](https://openfontlicense.org)).

To change what is said, a voice or the font, edit the tables at the top of
`tools/make_assets.py` and run it (`pip install piper-tts scipy pillow numpy`):

```sh
python3 tools/make_assets.py --voice it_IT-paola-medium.onnx --font Andika-Bold.ttf --wav /tmp/clips
```

`--wav` also writes every clip as a `.wav` to listen to. The voice is a little different at
every run; the script makes several takes of each clip and keeps one that ends cleanly.
