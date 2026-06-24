# Motion HAL

The Motion HAL is the platform boundary between the portable MPLC motion engine and drive hardware (stepper/servo pulse generators, ClearCore motor managers, vendor-specific homing sequences, and so on).

It mirrors the role of `hal/include/mplc_hal.h` for digital/analog I/O: the runtime owns PLCopen-inspired semantics; the HAL owns motors, encoders, and completion reporting from the backend.

## Stack placement

```
MC_* function blocks (runtime/motion/mc_fb.c)
        │
        ▼
Motion engine (runtime/motion/motion.c)
  • command IDs, arbitration, Stop/Halt locks
  • validate-before-abort, axis state machine
        │
        ▼
Motion HAL (hal/include/mplc_motion_hal.h)   ← you implement this
        │
        ▼
Drive / pulse engine / Enhanced ClearCore Library
```

Public headers:

| Header | Purpose |
|--------|---------|
| `hal/include/mplc_motion_types.h` | Shared types: axis refs, scales, move structs |
| `hal/include/mplc_motion_hal.h` | HAL function contract |
| `runtime/include/mplc/motion.h` | Motion engine API (calls the HAL) |

Reference implementation (simulator only): `runtime/motion/motion_hal_default.c` — linked into `mplc_runtime` for unit tests.

## Units and axis references

All motion quantities are fixed-point `int32_t` values with **independent** scales defined in `mplc_motion_types.h`:

| Quantity | Scale macro | Meaning |
|----------|-------------|---------|
| Position | `MPLC_MOTION_POSITION_SCALE` (1000) | engineering units × 1000 |
| Velocity | `MPLC_MOTION_VELOCITY_SCALE` (1000) | engineering units/s × 1000 |
| Acceleration | `MPLC_MOTION_ACCELERATION_SCALE` (1000) | engineering units/s² × 1000 |

Example: a position of `12.5 mm` with scale 1000 is `12500`. A velocity of `50 mm/s` is `50000`.

Axes are referenced by `mplc_axis_ref_t` (a `uint16_t` index). `MPLC_AXIS_INVALID` is reserved. The engine is initialized with a configured axis count (`MPLC_CONFIGURED_AXIS_COUNT`, default 2); the HAL must accept the same count in `mplc_motion_hal_init()`.

Per-axis limits and unit conversion to motor counts belong in `mplc_axis_config_t` (engine layer). The HAL receives already-scaled profile values in `mplc_motion_move_t`.

## Lifecycle

The motion engine calls the HAL in this order:

1. **`mplc_motion_hal_init(axis_count)`** — once at runtime load (via `mplc_motion_init()`).
2. **`mplc_motion_hal_enable(axis, true/false)`** — when `MC_Power` enables or disables an axis.
3. **Command entry points** — when a move/stop/home is accepted by the engine.
4. **`mplc_motion_hal_cycle(dt_us)`** — once per PLC scan from `mplc_motion_cycle()`.
5. **`mplc_motion_hal_get_status()` / `mplc_motion_hal_read_actual_position()`** — polled by the engine for completion and FB outputs.
6. **`mplc_motion_hal_shutdown()`** — on runtime teardown.

Return `0` from command functions on success, non-zero on rejection. Do not abort an in-flight move inside the HAL unless the engine has issued a new command or stop/halt.

## Two API tiers

### Command-level (production backends)

Use these when integrating real hardware. Each call should start or update a **segment** on the drive, not merely stash values for the next scan.

| Function | When the engine calls it |
|----------|--------------------------|
| `mplc_motion_hal_start_home` | Homing command accepted |
| `mplc_motion_hal_start_absolute` | Absolute move accepted |
| `mplc_motion_hal_start_velocity` | Velocity move accepted |
| `mplc_motion_hal_stop` | `MC_Stop` — pass `lock_axis=true` |
| `mplc_motion_hal_halt` | `MC_Halt` — axis may be superseded later |
| `mplc_motion_hal_get_status` | Every scan for completion and velocity FB |

Homing is **not** just an absolute move. Real backends implement switch search, backoff, offset, and position zeroing inside `start_home` or a platform-specific state machine driven from `cycle()`.

### Scan-level (default simulator only)

These exist for the built-in simulator in `motion_hal_default.c`. **Do not** treat them as the long-term production contract:

- `mplc_motion_hal_write_target_position`
- `mplc_motion_hal_write_command_velocity`

Hardware backends should implement the command-level functions and may leave the scan-level symbols as weak stubs or omit them entirely if you provide a full replacement HAL object file.

## Status and completion

`mplc_motion_hal_get_status()` fills `mplc_motion_hal_status_t`:

| Field | Intended meaning |
|-------|------------------|
| `actual_position` | Feedback position in scaled engineering units |
| `command_velocity` | Current commanded velocity |
| `in_velocity` | Velocity move has reached command speed |
| `moving` | Axis is actively moving |
| `target_reached` | Discrete move complete (prefer drive/HLFB state over position equality alone) |
| `stopping` | Controlled deceleration in progress |
| `homed` | Homing reference established |
| `power_enabled` | Drive enabled |
| `error` / `error_id` | Drive fault |

The engine uses `target_reached` (and fallbacks) to mark discrete commands **Done**. For ClearCore and similar stacks, map move-complete and HLFB inputs into `target_reached` and `moving` rather than inferring completion only from `actual_position == target`.

## Stop vs Halt (HAL view)

| | `mplc_motion_hal_stop` | `mplc_motion_hal_halt` |
|---|------------------------|-------------------------|
| Engine semantics | `MC_Stop` — locks axis against new moves until Execute drops | `MC_Halt` — may be superseded |
| `lock_axis` | `true` | N/A (always unlockable by new motion) |
| Typical backend action | Controlled decel + hold | Controlled decel, ready for new segment |

The engine tracks **which** `MC_Stop` instance owns the lock (`stop_owner_command_id`). Only that command ID may call `mplc_motion_release_stop()` when Execute goes false.

## Example: minimal backend skeleton

Create a platform file (e.g. `platforms/clearcore/motion_hal_clearcore.c`) that implements all `mplc_motion_hal_*` symbols:

```c
#include "mplc_motion_hal.h"
#include <string.h>

typedef struct {
    bool    enabled;
    bool    homed;
    int32_t actual_position;
    /* vendor handles, move state, etc. */
} clearcore_axis_t;

static clearcore_axis_t g_axes[MPLC_MOTION_HAL_MAX_AXES];
static uint32_t         g_axis_count;

int mplc_motion_hal_init(uint32_t axis_count)
{
    if (axis_count == 0U || axis_count > MPLC_MOTION_HAL_MAX_AXES) {
        return -1;
    }
    memset(g_axes, 0, sizeof(g_axes));
    g_axis_count = axis_count;
    /* init motor managers, GPIO, encoders */
    return 0;
}

void mplc_motion_hal_shutdown(void)
{
    g_axis_count = 0U;
}

void mplc_motion_hal_enable(mplc_axis_ref_t axis, bool enable)
{
    if (axis >= g_axis_count) {
        return;
    }
    g_axes[axis].enabled = enable;
    /* EnableRequest(enable) on motor connector */
}

void mplc_motion_hal_reset(mplc_axis_ref_t axis)
{
    if (axis >= g_axis_count) {
        return;
    }
    /* Clear drive faults */
}

void mplc_motion_hal_read_actual_position(mplc_axis_ref_t axis, int32_t *position)
{
    if (!position || axis >= g_axis_count) {
        return;
    }
    *position = g_axes[axis].actual_position;
}

bool mplc_motion_hal_read_power_status(mplc_axis_ref_t axis)
{
    return axis < g_axis_count && g_axes[axis].enabled;
}

bool mplc_motion_hal_read_error(mplc_axis_ref_t axis, int32_t *error_id)
{
    (void)error_id;
    (void)axis;
    return false;
}
```

## Example: absolute move and completion polling

Engine-side flow (simplified — normally driven by `MC_MoveAbsolute`):

```c
#include "mplc/motion.h"
#include "mplc_motion_hal.h"

void example_absolute_move(mplc_axis_ref_t axis)
{
    mplc_motion_move_t move;
    mplc_motion_hal_status_t hal_status;
    mplc_power_status_t power;

    mplc_motion_init(1U);
    mplc_motion_power(axis, true, &power);

    /* 25.0 mm @ 100 mm/s, accel/decel 500 mm/s² (scale 1000) */
    move.target_position = 25000;
    move.velocity        = 100000;
    move.acceleration    = 500000;
    move.deceleration    = 500000;

    mplc_motion_hal_start_absolute(axis, &move);

    for (;;) {
        mplc_motion_hal_cycle(10000U); /* 10 ms scan */
        mplc_motion_cycle(10000U);

        if (mplc_motion_hal_get_status(axis, &hal_status) == 0 &&
            hal_status.target_reached) {
            break;
        }
    }
}
```

Backend implementation of `start_absolute`:

```c
int mplc_motion_hal_start_absolute(mplc_axis_ref_t axis, const mplc_motion_move_t *move)
{
    if (axis >= g_axis_count || !move || !g_axes[axis].enabled) {
        return -1;
    }

    /* Convert scaled units → motor counts using axis config, then: */
    /* MotorMgr.MoveAbsolute(counts, vel_counts_s, accel, decel); */
    (void)move;
    return 0;
}

int mplc_motion_hal_get_status(mplc_axis_ref_t axis, mplc_motion_hal_status_t *status)
{
    if (axis >= g_axis_count || !status) {
        return -1;
    }

    memset(status, 0, sizeof(*status));
    status->power_enabled = g_axes[axis].enabled;
    status->actual_position = g_axes[axis].actual_position;
    /* status->moving = MotorMgr.IsMoving(); */
    /* status->target_reached = MotorMgr.MoveComplete(); */
    /* status->in_velocity = ... */
    return 0;
}
```

## Example: homing

```c
int mplc_motion_hal_start_home(mplc_axis_ref_t axis, const mplc_home_request_t *request)
{
    if (axis >= g_axis_count || !request) {
        return -1;
    }

    /*
     * Platform-specific sequence, e.g.:
     *  1. Fast move toward home switch
     *  2. Back off, slow re-approach
     *  3. Set actual_position = request->position
     *  4. Set homed = true in get_status()
     */
    (void)request;
    return 0;
}
```

Poll homing completion via `get_status()`:

```c
mplc_motion_hal_status_t st;
mplc_motion_hal_get_status(axis, &st);
if (st.homed && st.target_reached && !st.moving) {
    /* homing command can complete in the engine */
}
```

## Example: stop and halt

```c
/* MC_Stop: decelerate and lock until engine calls release_stop */
int mplc_motion_hal_stop(mplc_axis_ref_t axis, int32_t deceleration, bool lock_axis)
{
    if (axis >= g_axis_count) {
        return -1;
    }
    /* MotorMgr.StopDecel(decel_counts_s2); record lock_axis */
    (void)deceleration;
    (void)lock_axis;
    return 0;
}

/* MC_Halt: decelerate but remain ready for a new segment */
int mplc_motion_hal_halt(mplc_axis_ref_t axis, int32_t deceleration)
{
    if (axis >= g_axis_count) {
        return -1;
    }
    /* Same decel path as stop, but do not block subsequent starts */
    (void)deceleration;
    return 0;
}
```

Engine release after `MC_Stop.Execute` goes false (FB layer does this automatically):

```c
mplc_motion_release_stop(axis, stop_command_id);
```

Only the command ID that acquired the stop lock may release it.

## Example: CMake integration

Link your HAL into the platform target and **exclude** the default simulator if you provide a full replacement:

```cmake
add_library(mplc_hal_clearcore STATIC
    hal_clearcore.c
    motion_hal_clearcore.c
)
target_include_directories(mplc_hal_clearcore PUBLIC
    ${MPLC_REPO_ROOT}/hal/include
    ${MPLC_REPO_ROOT}/runtime/include
)
target_link_libraries(my_firmware PRIVATE mplc_runtime mplc_hal_clearcore)
```

If both `motion_hal_default.c` (inside `mplc_runtime`) and your platform HAL define the same symbols, the linker will report duplicate definitions. For production ports, either:

- build `mplc_runtime` without `motion_hal_default.c`, or
- provide the platform HAL as the only implementation and omit the default from the runtime library.

## Simulator reference

For tests and bring-up without hardware, `runtime/motion/motion_hal_default.c` integrates position in `mplc_motion_hal_cycle()` and implements command-level entry points as lightweight state updates. Run arbitration tests with:

```bash
cmake --build build --target test_motion
build/tests/runtime/test_motion
```

## Related reading

- [architecture.md](architecture.md) — overall runtime layers
- `runtime/include/mplc/motion.h` — engine API and MC FB separation
- `tests/runtime/test_motion.c` — command arbitration, stop ownership, validation
