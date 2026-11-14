# AGENTS.md: helium-pods

Instructions for an AI agent that adds features, fixes bugs or writes
documentation **in this repository**. Read this file and
`pods/podcraft/README.md` before touching code.

## What this repository is

The **games** of the Pod-ium family, firmware for the ClockworkPi
**PicoCalc** with a Raspberry Pi **Pico 2 W**:

| Pod | Folder | What it is |
|---|---|---|
| **PodCraft** | `pods/podcraft` | Voxel sandbox: procedural block world ray-cast on both cores, 18-bit frame streamed to the panel in bands by DMA, saves on the SD card. **Needs an RP2350** (Pico 2 / Pico 2 W) |

Built on the **PicoPods libraries** of
[hydrogenium-pods](https://github.com/Pod-ium/hydrogenium-pods)
(`picopods_import.cmake`: `PICOPODS_PATH`, otherwise a git fetch of its
`main`). Siblings: carbo-pods (productivity), aurum-pods (emulators and the
PicoPods Web page, which also runs PodCraft).

## Layout

```
pods/podcraft/src/   main.cpp (main loop, menus), config.h (tunables, keys),
                     world.* (world generation, blocks), render.* (ray caster, both cores),
                     screen.* (banded output to the panel), gfx.* (2D/HUD), textures.*,
                     storage.* (save/load of worlds)
tests/               CMakeLists.txt (scripted session)
tools/               gate.sh, package_release.sh, package_test.sh, build_web_pods.sh, ci_ref.sh, ci_apt.sh
picopods_import.cmake
```

## What belongs here, and what doesn't

- The game: world, rendering, controls, HUD, menus, saves.
- **Not here**: display/keyboard/SD drivers, file API, UI dialogs, the crash
  screen, the host and browser terminals. Those are in **hydrogenium-pods**.
  If the game needs a generic facility (a faster blit, a new input mode),
  add it to the libraries rather than to the pod.

## Rules for the game

- **RP2350 only**: the game relies on its RAM and both cores. It is built for
  `pico2_w` and `pico2`; don't try to make it fit an RP2040 unless asked.
- **Performance is the feature.** Rendering runs on both cores; measure with
  F3 (FPS, cores) before and after a change that touches `render.*`,
  `screen.*` or `world.*`, and state the effect in your report. Avoid
  allocations, virtual calls and floating-point divisions in per-pixel paths;
  keep hot tables `const` / in RAM as the existing code does.
- **Core 1** is launched with `sys::launch_core1()`; keep shared state
  hand-off as it is (no blocking on core 1 while core 0 waits on it).
- No full 18-bit framebuffer: output stays banded through DMA.
- Pods never name pins or chips; plain C++17 except what the libraries
  provide; the pod must also build for the PC host and the browser.
- Waits through `ui::Context::wait_key()` / the idle hook, sleeps through
  `pd::sys` (the browser build yields there).
- Key bindings: check the Controls table in the README, `config.h` and the
  F1 controls screen for clashes before adding one, and update all three.

### Saves (compatibility)
- Worlds are saved as `/podcraft/world1.pcw` … `world3.pcw`: **64-byte
  header + 256 KB world = 262208 bytes** (checked by `podcraft_script`).
- The game must start without an SD card.
- Changing the format requires a version field in the header, loading of
  the previous format (or a clear refusal message), an updated test, and a
  changelog entry. Ask the user before changing the save format.

## Build and gates

Toolchain (as CI): Pico SDK **2.1.1**, Arm GNU Toolchain **13.2.Rel1**,
CMake ≥ 3.13, Ninja, C++17 compiler, Python 3, `libmbedtls-dev`,
`libsdl2-dev`; Emscripten **3.1.74** for G-web.

| Gate | Command | Must |
|---|---|---|
| G-host | `PICO_SDK_PATH=… tools/gate.sh host` | PC build, `podcraft_script` passes |
| G-fw | `PICO_SDK_PATH=… tools/gate.sh fw pico2_w pico2` | `podcraft.uf2` links on both RP2350 boards |
| G-web | `PICO_SDK_PATH=… tools/build_web_pods.sh` | `podcraft.pod.js` builds |

If you also changed hydrogenium-pods, add `PICOPODS_PATH=/path/to/hydrogenium-pods`.
Logs: `build-gate/*.log`.

Play on the PC: `PD_SDCARD=./sd ./build-gate/host/pods/podcraft/podcraft`
(window with SDL2), or scripted:
`PD_SDCARD=./sd PD_FRAMES=./frames PD_KEYS="wait:1800,enter,wait:200,f3,shot" ./build-gate/host/pods/podcraft/podcraft`.

## Tests

Scripted sessions in `tests/CMakeLists.txt`: the real game runs headless on a
folder used as the SD card.

- `PD_SDCARD`, `PD_KEYS`, `PD_FRAMES`. Key tokens, comma-separated:
  `enter esc bksp space up down left right f1…f12 ctrl+x shift+x`, single
  characters, `text:<s>`, `wait:<ms>`, `shot`.
- The title screen needs time (`wait:1800` before the first key).
- Start from a clean folder, set `TIMEOUT`, and assert on the result (save
  file size/content, `PASS_REGULAR_EXPRESSION` on output).
- Rendering cannot be asserted pixel-exactly across compilers; check what is
  deterministic (saves, world generation from a seed, state printed to
  stdout) and keep `shot` frames for eyeballing.
- Feature ⇒ extend or add a session. Bug fix ⇒ a session that failed before.

## Documentation

- `pods/podcraft/README.md` (English): pitch, installation, **Controls**
  table (`| Key | Action |`), how it works, limits. Keep Controls in sync
  with `config.h` and the F1 screen.
- Root README: pod table, build, CI/releases, layout; its Build / CI /
  Layout sections are parallel to the sibling repositories'.
- **CHANGELOG.md**: add to `## Unreleased` → `### Pods` (or `### Repository`
  for build/CI); start with **PodCraft**, say what the player sees.
- The name is **PodCraft**. Some sibling docs (hydrogenium-pods, aurum-pods)
  still say *PicoCraft*; that is drift to be fixed there, not a second name.
- Versions: `VERSION` in `picopods_add_pod(...)`, bumped only when asked or
  for a release. Release assets: `podcraft_<v>.uf2`, `SHA256SUMS_<v>.txt`.
- Style: British spelling, terse, every key and path verified against code.

## Adding a game here
`pods/<pod>/` from hydrogenium-pods' `examples/template`; `add_subdirectory`
in `CMakeLists.txt`; a scripted test; `pods=` in `tools/package_release.sh`
and `tools/build_web_pods.sh`; CI board matrix if it supports RP2040; README
pod table here, in hydrogenium-pods' and aurum-pods' READMEs; a description
in aurum-pods' `web/app.js`; changelog entry.

## Git

- Work on the branch you are given; push to `main` only when explicitly told
  to. Open a pull request only when asked.
- Changes that also need hydrogenium-pods use **the same branch name** there
  (CI's `tools/ci_ref.sh` builds against it); the library side reaches
  hydrogenium-pods `main` first.
- Commit subject `PodCraft: <imperative summary>` (or `CI: …`, `Docs: …`),
  wrapped body with what and why, test names and measured FPS when relevant.
  Add the attribution lines your harness requires. No model identifiers in
  commits, code or docs.
- Never skip, disable or loosen a test or CI step to get green.

## Done means

- [ ] Change is in the right repository.
- [ ] Test added or extended; save compatibility kept.
- [ ] G-host, G-fw (`pico2_w`, `pico2`), G-web pass.
- [ ] README (controls) and CHANGELOG updated and verified.
- [ ] Report: what changed (`file:line`), gates and results, FPS effect,
      what needs a check on a real PicoCalc (frame rate, input feel).
