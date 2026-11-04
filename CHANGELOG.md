# Changelog

Pods of helium-pods (games). Until the split of the PicoPods tree their
history was kept in Pod-ium/PicoPods; the libraries' changes are in the
CHANGELOG of hydrogenium-pods.

## Unreleased

### Repository
- Moved here from Pod-ium/PicoPods, with the pods' scripted host tests. Built
  on the libraries of hydrogenium-pods (`picopods_import.cmake`).
- CI (G-host, G-fw, G-web) on pushes and pull requests; release workflow on
  tags `v*` and published releases.

### Pods
- **PodCraft** (new pod): voxel sandbox game. Procedural block world
  ray-cast on both cores, 18-bit frame streamed to the panel in bands by
  DMA, saves on the SD card. Needs an RP2350.
- **PodABC** (new pod): letters and numbers for young children, in Italian.
  Game 1 *Ascolta e trova*: the PicoCalc says a letter or a number, the
  child presses its key (right: the symbol on the whole screen, a jingle and
  a "well done"; wrong: "Prova ancora!" and the symbol again). Game 2 *Premi
  e ascolta*: every key pressed is shown and said. Esc + Up go back to the
  menu. Italian voice (Piper it_IT-paola) and Andika letters embedded in the
  firmware (~0.5 MB, `tools/make_assets.py`); every Pico board. Host tests:
  a scripted session and the player checked sample by sample.
- CI builds the `pico` board too (PodABC; PodCraft is skipped there).
