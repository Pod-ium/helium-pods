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
- **PicoCraft 1.8** (new pod): the voxel sandbox game PicoCraft 1.7 moved
  onto the libraries. 1.7 did not work on the PicoCalc: its own display
  driver set 16-bit colour, which the panel ignores over SPI (it only takes
  18-bit), toggled CS/DC with no settle time and split the window setup
  from the pixels; its keyboard reads waited 150 µs instead of the 16 ms
  the controller needs. The frame now goes to the panel in 18-bit bands by
  DMA while both cores ray-cast the next one (rows handed out one at a
  time), and the keyboard is read while core 1 renders. Saves of 1.7 still
  load. Needs an RP2350.
