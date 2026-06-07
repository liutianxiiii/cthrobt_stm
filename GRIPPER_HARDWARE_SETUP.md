# Gripper Hardware Setup

## Overview

Button A closes the gripper, button B opens it. In simulation mode the firmware
prints `[GRIPPER] CLOSE` / `[GRIPPER] OPEN` over UART3. On real hardware a servo
is driven by TIM3 CH1 PWM on PB4.

---

## Wiring

| Servo wire | Connect to |
|---|---|
| Signal (orange/yellow) | PB4 — CN9 pin 6 (Arduino D5) |
| Power (red) | 5 V rail (VIN on CN8 pin 7, or external 5 V) |
| Ground (brown/black) | GND (CN8 pin 11 or any GND) |

> **Note:** Do not power the servo from the Nucleo 3.3 V rail — servo stall current
> will cause a brown-out. Use the 5 V rail or a separate supply.

---

## STM32CubeMX Configuration

The following changes must be made to `cthrobt.ioc` before flashing to hardware.
After making changes click **Project → Generate Code**.

### 1. Enable TIM3 Channel 1

| Setting | Value |
|---|---|
| Peripheral | TIM3 |
| Channel 1 | PWM Generation CH1 |
| Pin | PB4 (auto-assigned as TIM3_CH1, AF2) |

**Parameter Settings for TIM3:**

| Parameter | Value | Notes |
|---|---|---|
| Prescaler | 95 | 96 MHz ÷ 96 = 1 MHz tick (1 µs/tick) |
| Counter Period (ARR) | 19999 | 20 ms period = 50 Hz |
| Counter Mode | Up | |
| Auto-reload preload | Enable | |
| CH1 Pulse (initial) | 2000 | 2 ms = open position |
| CH1 Fast Mode | Disable | |
| CH1 Polarity | High | |

### 2. Enable TIM3 Clock in NVIC (optional)

No interrupt is needed for PWM output — the timer runs in hardware. Leave
TIM3 global interrupt disabled unless you add other TIM3 features.

---

## PWM Pulse Reference

| State | Pulse width | CCR value |
|---|---|---|
| Open | 2.0 ms | 2000 |
| Closed | 1.0 ms | 1000 |

These are typical values for a standard 180° servo. If your gripper
travels differently, adjust `GRIPPER_CCR_CLOSE` and `GRIPPER_CCR_OPEN`
in [`Core/Src/gripper.c`](Core/Src/gripper.c).

---

## Build for Real Hardware

Remove `SIMULATION_MODE` from the compile definitions in `CMakeLists.txt`:

```cmake
# Remove or comment out this line:
SIMULATION_MODE
```

Then rebuild:

```bash
bash scripts/build.sh
```

The preprocessor will activate the TIM3 PWM code in `gripper.c` and
deactivate the simulation printf stubs automatically.

---

## Button Mapping

| Button | Action |
|---|---|
| A | Close gripper (servo → 1 ms pulse) |
| B | Open gripper (servo → 2 ms pulse) |

Commands are edge-triggered: holding a button does not repeat the action.
