# Kokirii — Handoff / State File

**Fork version:** v0.1.0
**Date:** 2026-11-12
**Branch:** `master`
**Purpose of this file:** the single source of truth for continuing work on Kokirii. Read this
before doing anything. Update it at the end of every working session (see §9).

---

## 1. What Kokirii is

Kokirii is a fork of **Lichii** (itself a fork of [Eglantine](https://github.com/Djihads80/Eglantine),
`nx2` branch = Switch 2 emulation), trimmed and retuned for exactly one game:

- **The Legend of Zelda: Ocarina of Time** (Switch 2 remake), program id `0400486024DEC000`.

Goals, in order:

1. Cull all other-game content (code, tests, docs, tools) — done, see §3.1.
2. Keep every generic diagnostic, shared test and shared system intact — see §4 for the kept list.
3. Fix emulator systems that are "not up to standard" per the attached research report — done, see §3.2.
4. Full rebrand (Lichii → Kokirii) — **not started** (§5.1).
5. Launcher redesign (charcoal sidebar, forest-green/white accents, Home/Library/Settings) — **not started** (§5.2).
6. Speed pass for OoT on modest PCs — **not started** (§5.3).

The driving research document is `C:\Users\lachl\Desktop\Switch 2 Emulation Research Report.pdf`
(66 pages, dated 2026-11-11). It was written about this exact lineage and this exact machine
(6-core Ryzen 5 6500X-class, RX 7600 8 GB RDNA3, 16 GB RAM) with Ocarina of Time as its case
study. Its text has been extracted to
`C:\Users\lachl\AppData\Local\Temp\opencode\report_text.txt` (temp — re-extract with
`C:\Users\lachl\AppData\Local\Temp\opencode\pdf_text.ps1` if gone; that script works, see §8).
Note: report claims marked `[Reported]` are second-hand; `[Inference]` are the author's guesses.
Everything below was verified against our tree line by line.

## 2. Git state and working rules

- `master` = `c152537fa8` "testing" (upstream nx2 tip + user's README commits + baseline +
  **complete cull**) + the v0.1.0 session changes (§3.2, §3.3). Pushed to `origin/master`.
- `beta` = upstream tip + user's README commits. **Do not work here.** It is only useful as a
  reference for what upstream looks like un-culled.
- `b061ce928c` "Baseline: Lichii nx2 tree before Kokirii cull/rebrand" — kept as a reference
  point; diff against it to see everything Kokirii removed.
- The repo still has remotes `origin` (this fork) and possibly `upstream` (Eglantine). Before
  porting any fix, follow the AGENTS.md protocol: check upstream PRs/issues first, merge
  upstream if behind, port before writing new.
- **This machine has no compiler, no CMake, no Python, no Qt.** Every change must be
  grep-verified for compile correctness by hand (includes, symbols, CMakeLists file lists).
  Builds and real testing happen on the owner's Windows PC (see AGENTS.md "Builds").

## 3. Completed work

### 3.1 Cull phase (committed in `c152537fa8` "testing")

All of this is already committed on master:

- Removed game-specific code: `WindowTrace` (Star Fox) block in
  `src/core/hle/service/nx2_gsv.h`, `game_dlss_status`, `fire_emblem_upscale.{h,cpp}`,
  `thread_startup_policy` (test + sources), `EDEN_NX2_RESETFIX` (had no src consumer),
  other per-game switches from `src/yuzu/nx2_switches.h` and `dist/nx2_switches.txt`
  (DKB/MKW comment blocks trimmed).
- Tests: `tests/CMakeLists.txt` entries for `window_trace.cpp` and `thread_startup_policy.cpp`
  removed; `tests/video_core/game_presets.cpp` **kept** (standalone test validating
  ScaleForce/FXAA defaults in `settings.h`).
- Tooltips: Fire Emblem sentence removed from the `nvidia_dlss` tooltip in
  `src/qt_common/config/shared_translation.cpp`.

### 3.2 Report-driven audit and fixes (v0.1.0 session)

The report's list of "not up to standard" systems was audited claim-by-claim. Result:

| # | Report claim | Status in our tree | Where |
|---|---|---|---|
| 1 | Guest counter must be 31.25 MHz (19.2 is the Switch 1 trap) | ✅ already present | `src/common/cpu_features.h:23-24` (`Switch2CNTFRQ = 31'250'000`), `src/core/core.cpp:418-419`, `src/core/hle/service/psc/time/common.h:141`, `src/core/arm/dynarmic/arm_dynarmic_64.cpp:274` |
| 2 | `DISABLE_OPTIMIZATION_BIT` for the two OoT compute programs that take 6–18 min in AMD's compiler | ✅ already present | `src/video_core/renderer_vulkan/vk_compute_pipeline.cpp:278` |
| 3 | AMD mesh-shader hangs → compute fallback, force switches | ✅ already present | `EDEN_NX2_FORCE_MESH_FALLBACK` (`src/video_core/vulkan_common/vulkan_device.cpp:1758-1761`), `EDEN_NX2_NO_MESH` (`vk_rasterizer.cpp:777`), tests `src/tests/video_core/mesh_fallback.cpp` |
| 4 | **GPU accuracy High silently disables the split GPU thread** — a user reading "High accuracy = better" loses most of the speed | ✅ **FIXED in v0.1.0** — the only real gap | Default changed High → Low on **all desktop platforms** in `src/common/settings.h:565` (was Android-only Low; the desktop default High blocked the split at `src/video_core/gpu_split.cpp:105-107`). Tooltip updated in `src/qt_common/config/shared_translation.cpp:292-295` to warn that Accurate disables the split. Note `current_gpu_accuracy` (settings.h:577) is just the runtime mirror synced at load (`settings.cpp:176`) — leave it. |
| 5 | ~35k occlusion reads/sec stalling the GPU thread → DMA snapshot / async readback | ✅ already present | `src/video_core/nx2_async_readback.h`, `EDEN_DKB_INDIRECT_READBACK` (`vk_rasterizer.cpp:1008-1016`) |
| 6 | VRAM budget from driver recheck ~30 frames; BC3 ASTC recompression default on <12 GB | ✅ already present | `VK_EXT_memory_budget` (`vulkan_device.cpp:841, 2004, 2078`), `astc_recompression` default `Bc3` (`settings.h:640-641`) |
| 7 | Thread pinning for 6-core hosts | ✅ already present | `src/common/thread.h:138-142`, `thread.cpp:585, 731` (`pin_gpu_threads`, `PinCurrentThread`, `PlanGpuThreadPinning`) |
| 8 | Matrix-core (RDNA3) gate on the Vulkan device | ✅ already present | `vulkan_device.cpp:1804-1806` |
| 9 | 12 GB memory layout + 39-bit address space forced for NX2 | ✅ already present | `src/core/core.cpp:412-415` (forced `Memory_12Gb`), 39-bit space in the NX2 memory setup |
| 10 | Six guest cores (not four) | ✅ already present | `src/core/hardware_properties.h:17-19` |
| 11 | Recommended-settings table should pin accuracy Low | ✅ already present | `src/common/recommended_settings.h:23` (`entry(settings.gpu_accuracy, GpuAccuracy::Low)`) |
| 12 | Device-loss diagnostics (VK_EXT_device_fault) always on | ✅ already present | `vulkan_device.cpp:1043` (`ReportLoss`), `:1375` (fault dump); `vk_scheduler.cpp:878-879` calls `ReportLoss()` then throws with a clear log (`vk_master_semaphore.cpp:114`) |
| 13 | Spin-wait / idle-loop detection (skip or yield guest spin loops) | ❌ **absent** — document as P2 future work | Nothing in tree (`SpinWait`/idle-detect greps clean). Doing it blindly would be a guess; needs profiling first (report §7.1: measure before changing). |
| 14 | Movie-to-3D handoff AMD device loss (report cites upstream issue 871) | ⚠️ no special handling; crash path has full diagnostics | OoT's intro movie exercises this path on AMD. If it reproduces on the owner's PC, **port the upstream fix first** (AGENTS.md protocol), do not invent one. The `Device fault` diagnostic lines are always on, so a reproduction produces actionable data. |

Conclusion: everything in the report's fix list is already implemented except #4 (fixed in
v0.1.0) and #13 (future work). Nothing else should be "re-fixed" from the report.

### 3.3 Additional cull finished in the v0.1.0 session (uncommitted until the v0.1.0 commit)

- `src/yuzu/util/icon_search.cpp` and `src/yuzu/configuration/configure_per_game.cpp`:
  the duplicated `ResolveGameNameAlias` tables (MKW/SMO/MK8D/BotW/... aliases) replaced with
  the OoT-only table (`0x0400486024DEC000` + aliases `oot`, `zeldaoot`, `ocarinaoftime`,
  `ocarina-of-time`).
- Docs removed (19 files): `docs/bananza-*.md` ×11, `docs/duskbloods-*.md` ×2,
  `docs/FE_UPSCALE_STANDIN.md`, `docs/kirby-white-world.md`, `docs/STARFOX_*.md` ×3,
  `docs/performance/MKW-INTEGRATION.md` (and the now-empty `docs/performance/` dir).
  Verified first: all cross-references to those docs were inside the removed cluster itself;
  no dangling links remain.
- Tools removed: `tools/nx2/mkw_race_bench.sh` (MKW-only bench).
- `AGENTS.md`: game bug-report table trimmed to OoT only; wording updated Lichii → Kokirii.

## 4. Decisions — what to change and what NOT to change

This section is the contract. If a future session wants to violate one of these, it must
update this file and say why.

**Kept (do not cull):**

- `EDEN_NX2_DLSS_REPLACEMENT` and `EDEN_NX2_DLSS_CONTRACT_TRACE` — real OoT consumers exist
  (`src/video_core/renderer_vulkan/vk_ocarina_camera_motion.{h,cpp}`,
  `vk_texture_cache.cpp:1817-1978`).
- `EDEN_DKB_INDIRECT_READBACK` and every other `EDEN_DKB_*` / `dkb_*`-named switch or tracer
  (`DkbCpuTrace`, `DkbFrameTrace`, ...) — the name is historical; they are **generic
  diagnostics**, not game content. Same precedent: the
  `tools/shader-sweep/fixtures/{bananza-byte-store,bananza-unmapped-atomic-counter,tennis-geometry-passthrough}.json`
  fixtures are **kept** even though their names name games — the shared test
  `tools/shader-sweep/test_replay.py` depends on them and they exercise generic
  shader-compiler paths (byte stores, unmapped atomic counters, geometry passthrough).
  This deliberately deviates from an earlier cull plan note; that note was wrong.
- `tests/video_core/game_presets.cpp` — standalone shared test.
- The `Flush` Stop(207) `CompleteHandedFences()` block in `nx2_gsv.h` — it is
  command-gated, not game-id-gated; OoT's movie player may exercise it.
- `NEEDS_D24` in `vk_rasterizer.h:176-180` (SSBU/XC2/FE-Engage depth-bias quirk table) and
  `FIRMWARE_REQUIRED_GAMES` in `firmware_manager.h:23-25` — generic driver/infra tables with
  per-title entries; OoT never matches them, they are harmless and trimming them is churn
  with no benefit. Left in place on purpose.
- Tests that use other-title ids as **negative cases** (`tests/video_core/oot_network.cpp:28`
  MKW id, `tests/core/builtin_patches.cpp:30` other title) — they prove gates don't fire for
  other games.
- The `nx2` name in env vars (`EDEN_NX2_*`), internal paths and branch names — renaming these
  is part of the rebrand phase (§5.1), not ad-hoc.

**Culled and must stay culled:**

- All game-specific features/traces/presets listed in §3.1 and §3.3 (window_trace,
  game_dlss_status, fire_emblem_upscale, thread_startup_policy, RESETFIX, MKW bench,
  the 19 game docs, other-game alias tables).

**Do not "re-fix" from the report:** items 1–12 and 14 of the table in §3.2 are already in
the tree; adding them again creates regressions. The only open item is #13 (spin-wait
detection, P2).

**Settings defaults policy:** defaults in `settings.h` are the fork's shipped defaults;
`recommended_settings.h` is what "Use recommended settings" writes. They are separate on
purpose — keep them consistent (both now say accuracy Low) but do not merge the files.

## 5. Pending phases (roadmap)

### 5.1 Rebrand (next phase)

- Strings: all user-visible "Lichii"/"Eden"/"Eglantine" naming in the Qt frontend, window
  titles, about dialog, updater metadata.
- Exe name → `kokirii.exe`; check `src/CMakeLists.txt`, app icon setup.
- Data folder: `src/common/fs/fs_paths.h:11` `#define EDEN_DIR "lichii"`. **Decision needed
  before touching:** forks share data folders deliberately (report §5.1: Lichii/Fritchii read
  each other's saves because of the shared Eden folder). Eglantine PR #1253 gave Lichii its
  own folder with a recoverable Eden import (`290da9e8af` in our history) — follow that
  pattern: rename the folder to `kokirii` but keep an import path for existing Lichii data.
- `dist/installer.nsi`, `dist/` shipped files, `CMakeModules/GenerateSCMRev.cmake`,
  updater metadata.
- Rebrand must be mechanical and grep-driven; keep the `EDEN_NX2_*` env-var names until the
  end, then decide (renaming them breaks every existing log/debug workflow — AGENTS.md
  documents them extensively).

### 5.2 Launcher redesign

- New theme `dist/qt_themes/kokirii/` (model on the existing themes there); register it in
  `src/qt_common/config/uisettings.{h,cpp}` (theme list + default).
- Sidebar navigation (Home / Library / Settings) in `src/yuzu/main_window.cpp` +
  `src/yuzu/main.ui`; loading screen colors in `src/yuzu/loading_screen.{h,cpp}`.
- Palette: charcoal sidebar, forest-green accents, white text (owner's mock).

### 5.3 Speed pass (after rebrand)

- Sources: report §7 (the speed plan), `NX2_NOTES.md`, `docs/nx2/*`.
- Known open item: spin-wait/idle detection (§3.2 #13) — P2, needs measurement first.
- OoT speed levers that already exist and should be tuned/documented, not reinvented:
  builtin patches (`src/core/file_sys/patch_manager.cpp:107-131`: Max Resolution, FPS Unlock,
  third patch), `ocarina_fsr` (`settings.h:426`), `oot_anti_aliasing` (`settings.h:446-447`),
  `docs/nx2/ocarina-dynamic-fps.md`, `docs/nx2/oot-network-bypass.md`.
- Do not touch the report items in §3.2 table again.

### 5.4 Crash/visual findings doc (can run in parallel)

- Gather OoT-relevant crash/visual knowledge from `NX2_NOTES.md` and
  `docs/nx2/{amd,ocarina-dynamic-fps,native-dlss-replacement,oot-network-bypass}.md` into one
  user-facing doc (what symptoms mean what, which env switches to try).
- AGENTS.md's debug-tools table stays as-is — it is generic and already covers OoT.

## 6. Key file map

| Area | Files |
|---|---|
| Fork defaults | `src/common/settings.h` (defaults, `gpu_accuracy` :565, `ocarina_fsr` :426, `oot_anti_aliasing` :446), `src/common/recommended_settings.h` |
| Split GPU thread | `src/video_core/gpu_split.{h,cpp}` (accuracy gate :105-107) |
| Async readback | `src/video_core/nx2_async_readback.h` |
| OoT upscaler/patch hooks | `src/video_core/renderer_vulkan/vk_ocarina_camera_motion.{h,cpp}`, `src/core/file_sys/patch_manager.cpp:107-131`, `src/video_core/oot_network.{h,cpp}` |
| Env switch allowlist | `src/yuzu/nx2_switches.h`, `dist/nx2_switches.txt` |
| Tooltips/labels | `src/qt_common/config/shared_translation.cpp` (gpu_accuracy :292) |
| Data dir | `src/common/fs/fs_paths.h:11` |
| Theme/launcher | `dist/qt_themes/`, `src/qt_common/config/uisettings.{h,cpp}`, `src/yuzu/main_window.cpp`, `src/yuzu/loading_screen.{h,cpp}` |
| Tests CMake | `src/tests/CMakeLists.txt` |
| Research text | `C:\Users\lachl\AppData\Local\Temp\opencode\report_text.txt` (re-extract via `pdf_text.ps1`) |

## 7. Verification protocol (this machine)

No build tools here. For every change:

1. `grep` the exact new symbol in every file claimed to use it.
2. `grep` the old symbol to zero hits (excluding this file and git history).
3. Check includes: any new type/function must already be in scope (read the top of the file).
4. Check `CMakeLists.txt` for removed/added files (tests especially).
5. Never trust "it should compile" — match the surrounding code's exact patterns.

## 8. Report extraction notes (if the text file is gone)

The PDF is Chromium-printed. Working extractor: `pdf_text.ps1` (in
`C:\Users\lachl\AppData\Local\Temp\opencode\`). Run:
`powershell -NoProfile -ExecutionPolicy Bypass -File <path>\pdf_text.ps1`.
Its hard-won parsing rules, in case it needs porting: follow the xref trailer `/Prev` chain
(the last table is an incremental update and only covers new objects); map fonts from the
whole page dict (`/F5` → obj 5, don't use a `<<...>>` regex that stops at the ExtGState
dict); parse bfchar and bfrange (array-destination form too); Chromium fonts are Type0 with
width-2 codes; use `Td`/`Tm` y-coordinates for line breaks (per-glyph positioning). Do not
pipe PowerShell scripts from stdin — write script files only.

## 9. How to update this file

- Bump the version and date at the top when a phase or a meaningful batch lands
  (v0.1.0 = cull complete + report audit/fixes; rebrand will be v0.2.0, etc.).
- Add finished work under §3 with file:line references; move roadmap items out of §5 only
  when actually done; record every decision (including "we decided NOT to do X") in §4 —
  the next AI must not re-litigate them without evidence.
- Keep §2 (git state) current: commit hashes and what they contain.
- Never delete the "do not change" contract items; append new ones.

## 10. Known risks / open questions

- Movie-to-3D AMD device loss on OoT's intro (§3.2 #14) — unverified on the owner's PC.
  Diagnostics are armed (VK_EXT_device_fault always on).
- Accuracy default change (Low) alters behavior for users who relied on the old High default
  and hit accuracy-dependent rendering bugs; the tooltip and the recommended-settings
  identity make the recovery path obvious ("GPU Mode: → Accurate").
- `beta` still contains un-culled upstream code; accidental work on `beta` would resurrect
  culled content. Always confirm `git rev-parse --abbrev-ref HEAD` says `master`.
