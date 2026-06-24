# MotionStack

MotionStack is a standalone **C99** motion math and planning library for MPLC. It ports core motor-control concepts from EnhancedClearCoreLibrary (StepGenerator, coordinated planner, linear/arc interpolators, UnitConverter, and ClearPath drive logic) into portable C structs and function-pointer vtables.

**Phase 2 status:** Junction planner, accel-limited linear interpolation, and parametric arc geometry are implemented and validated via host unit tests. MotionStack remains **not wired** into `mplc_runtime`, `mplc_motion_hal`, or any platform port.

## Modules

| Header | Role |
|--------|------|
| `mplc_motionstack_types.h` | Segments, mechanics, wire layout constants |
| `mplc_motionstack_endian.h` | LE pack/unpack for segment wire structs |
| `mplc_motionstack_q15.h` | Q15 fixed-point helpers (`FRACT_BITS=15`) |
| `mplc_motionstack_units.h` | Steps ↔ engineering units |
| `mplc_motionstack_profile.h` | Single-axis trap/velocity profile |
| `mplc_motionstack_interp.h` | 2-axis linear and arc interpolators |
| `mplc_motionstack_planner.h` | Segment queue + GRBL junction planner |
| `mplc_motionstack_port.h` | Platform step/encoder callback API |
| `mplc_motionstack_drive.h` | Generic stepper and ClearPath drive profiles |
| `mplc_motionstack.h` | Public init/tick/queue API |

## Planner tuning (Phase 2)

| API | Purpose |
|-----|---------|
| `mplc_ms_planner_set_junction_deviation(planner, steps)` | GRBL junction deviation in steps (default `1`) |
| `mplc_ms_planner_set_junction_dv_max(planner, dv_max)` | Optional per-axis Δv cap; `0` uses angle-based GRBL formula |
| `mplc_ms_planner_set_stop_at_queue_end(planner, stop)` | Tail segment exits at `0` when `true` (default) |
| `mplc_ms_planner_set_grbl_mode(planner, enabled)` | Enable/disable `RecalculatePlanner` on queue push |

## Endianness

On-wire segment descriptors are **always little-endian** (consistent with `.mplc` ABI). In-RAM planner state uses native host endian. See `schema/mplc_endian.h` for `mplc_write_le16/32/64` helpers.

## Drive profiles

- **Generic stepper** — step/direction with enable and fault flags (logic only).
- **ClearPath (golden example)** — step/dir mode, HLFB gating before motion, 1:1 pulse resolution, typical 5 kHz step clock.

## Testing

```bash
cmake -B build -DMPLC_BUILD_TESTS=ON -DMPLC_BUILD_COMPILER=ON
cmake --build build --config Release
ctest --test-dir build -C Release -R test_ms_
```

Tests live under `tests/motionstack/`:

| Test | Validates |
|------|-----------|
| `test_ms_endian` / `test_ms_endian_be` | LE wire round-trip |
| `test_ms_q15` | Q15 math, profile, units |
| `test_ms_linear` | Accel-limited line reaches target |
| `test_ms_planner` | Queue + deferred start |
| `test_ms_junction_planner` | Junction entry/exit speeds |
| `test_ms_junction_trajectory` | Corner decel in executed path |
| `test_ms_linear_accel` | Ramp / cruise / decel profile |
| `test_ms_arc` | CW/CCW quarter arc endpoints |

## ClearCore lineage

Algorithms are ported from EnhancedClearCoreLibrary reference sources (C++), not vendored. MotionStack ships **C only** — no `.cpp`, classes, or STL.

See [docs/architecture.md](docs/architecture.md) for tick model, coordination, and future platform integration.
