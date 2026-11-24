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
- "Pod test build" workflow (`.github/workflows/pod-test.yml`,
  `tools/package_test.sh`): run by hand from the Actions tab on any branch to
  build the pods for a board and attach them to the `test` pre-release as
  `<pod>-test.uf2` (e.g. `podcraft-test.uf2`), for flashing and testing on
  hardware.

### Pods
- **PodCraft** (new pod): voxel sandbox game. Procedural block world
  ray-cast on both cores, 18-bit frame streamed to the panel in bands by
  DMA, saves on the SD card. Needs an RP2350.
