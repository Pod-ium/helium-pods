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
- **The Last Bell** (new pod): Final-Fantasy-style RPG. Party of four
  wardens in the drifting sky of Vael; overworld, towns, four bell towers
  and the Veilspire, turn-based battles, skills learned on level-up, shops
  and inns, three save slots on the SD card. 160x160 indexed framebuffer
  scaled 2x by DMA (IndexedBlitter), tiles and sprites authored as ASCII
  art. Needs an RP2350.
