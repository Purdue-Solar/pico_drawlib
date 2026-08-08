# Thank You
Thank you to [ST7789 Library for Pico](https://github.com/ArmDeveloperEcosystem/st7789-library-for-pico)

# Building
Use `CMakeLists.txt` to compile for the Raspberry Pi Pico 2.

# Simulation (make, linux only)
Run `make` to compile a simulation that you can run on your computer.
The built simulation will be located at `simbuild/pico_drawlib`.

## Display simulation (cmake, windows or linux)

`components/pico_drawlib/Sim` contains a desktop simulation of the ILI9341
display that renders to a [raylib](https://www.raylib.com/) window. It is a
**completely separate build** from the embedded firmware — it uses your native
host `gcc`/`g++`, not the ARM cross-compiler, and has its own out-of-source
build directory.

### Simulation prerequisites

**Windows (Chocolatey)**

The VS Code Pico extension's ARM toolchain is a cross-compiler and cannot build
native Windows code. Install MinGW-w64 (provides native `gcc`/`g++`):

```powershell
choco install mingw
```

Confirm it is on your PATH before running CMake:

```powershell
gcc --version
```

**Linux / WSL2**

```bash
sudo apt install gcc g++ cmake ninja-build
# raylib is auto-downloaded by CMake if not found; to install system-wide:
sudo apt install libraylib-dev   # Ubuntu 22.04+ / Debian 12+
```

### Build the simulation

**Windows** — run from the repo root in a plain PowerShell (not the Pico
extension terminal, which only has the ARM toolchain on its PATH):

```powershell
cmake -S Sim -B Sim/build `
      -G Ninja -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++
cmake --build Sim/build
.\Sim\build\pico_drawlib_sim.exe
```

**Linux / WSL2:**

```bash
cmake -S Sim -B Sim/build -G Ninja
cmake --build Sim/build
./Sim/build/pico_drawlib_sim
```

If raylib is not installed system-wide, CMake fetches and builds it from GitHub
on the first configure (requires internet; takes about a minute). Subsequent
builds reuse the cached download.

### Clean simulation build

```bash
rm -rf Sim/build   # Linux
rmdir /S /Q Sim\build  # Windows PowerShell
```

---

## Fault and Warning Reference

All fault detection and display logic lives in `Src/pdl.cpp`. The steering wheel (`main/`) only supplies raw CAN data and staleness flags; the draw library owns all classification and safety display. Bit positions are single-sourced in `components/pico_canlib/Inc/artemis_canid.hpp` — changing an enum value there automatically updates every check in this file.

---

### Critical Faults (Red / Yellow overlay)

Critical faults draw a full-screen overlay over the centre of the display, obscuring non-critical data while leaving the vehicle speed (main page) and the 8 LV status icons (diagnostics page) visible. The warning count and warning banner at the bottom are always visible.

| Colour | Meaning |
|--------|---------|
| **Red** | Fault condition is currently active |
| **Yellow + "LATCHED"** | Condition has cleared, but was seen this session — persists until a new active fault supersedes it |

Critical faults are evaluated in the order below; the first matching condition is shown. When multiple faults are active simultaneously, the highest-priority one (lowest number) is displayed.

| Priority | Overlay Text | Trigger condition | Driver action |
|----------|--------------|-------------------|---------------|
| 1 | **ISOLATION FAULT** | `bms_dtc_flags2_2` bit `HighVoltageIsolationFault` | ESTOP + EXIT CAR, CHASSIS LIVE |
| 2 | **CONTACTOR FAULT** | `monitor_status` bit `ContactorFault` (`DistroDisplayMisc::ContactorFault`, set by the power distro board) | ESTOP + EXIT CAR, BATTERY LIVE |
| 3 | **BMS NON-OPERATIONAL** | Any OBD-II code in the table below | ESTOP + EXIT CAR, BMS ERROR |
| 4 | **AUX BATTERY FAULT** | Any error bit (0–3) in `aux_status`, or `AuxHardwareDetectedFault` / `AuxPowerMonitorI2cError` in `monitor_status` | TURN OFF, AUX ERROR |

> The contactor fault is detected and timed by the power distro board, which sets `DistroDisplayMisc::ContactorFault` (bit 6 of `monitor_status`). The steering wheel checks this single bit with a staleness guard so false alarms cannot occur at boot.

#### BMS Non-Operational — OBD-II code mapping

| OBD-II Code | Description | PDLInfo field | `artemis_canid.hpp` enum value |
|-------------|-------------|---------------|-------------------------------|
| P0A07 | Discharge Limit Enforcement | `bms_dtc_flags1` | `BmsDtcFlags1::DischargeLimitEnforcementFault` |
| P0A09 | Internal Hardware Fault | `bms_dtc_flags1` | `BmsDtcFlags1::InternalHardwareFault` |
| P0A0B | Internal Software Fault | `bms_dtc_flags1` | `BmsDtcFlags1::InternalSoftwareFault` |
| P0A1F | Internal Communication Fault | `bms_dtc_flags2_1` | `BmsDtcFlags21::InternalCommunicationFault` |
| P0A04 | Open Wiring Fault | `bms_dtc_flags2_1` | `BmsDtcFlags21::OpenWiringFault` |
| P0AC0 | Current Sensor Fault | `bms_dtc_flags2_1` | `BmsDtcFlags21::CurrentSensorFault` |
| P0A0F | Cell ASIC Fault | `bms_dtc_flags2_1` | `BmsDtcFlags21::CellAsicFault` |
| P0560 | Redundant Power Supply Fault | `bms_dtc_flags2_2` | `BmsDtcFlags22::RedundantPowerSupplyFault` |
| P0A05 | Input Power Supply Fault | `bms_dtc_flags2_2` | `BmsDtcFlags22::InputPowerSupplyFault` |

---

---

### Acceleration Enable

The motor current/velocity command (`0x501 MotorCurrentVelocityControl`) is sent to zero when **any** of the following conditions is true. All three sources must be simultaneously healthy for the pedal to drive the motor.

| Condition | Source | Field(s) checked |
|-----------|--------|-----------------|
| Motor controller has error flags | MC errors CAN message (`mc_errors_stale` + `mc_error_flags1` / `mc_error_flags2`) | Any bit non-zero in either byte inhibits acceleration |
| BMS discharge relay disabled | BMS safety message (`bms_safety_stale` + `bms_relay_state1`) | `BmsRelayState1::DischargeRelayEnabled` must be set |
| Power distro reports bad state | Power distro display message (`power_distro_stale` + `monitor_status`) | `DistroDisplayMisc::DistroBad` (bit 0) must be clear |

Stale CAN data (message not received within 3× its nominal period) counts as unsafe — acceleration is inhibited until fresh data arrives. This prevents driving with no visibility into motor, BMS, or distro state. Also note that "DistroBad" is meant to indicate when the car is not safe to drive - including precharge. 

Power hold is cancelled by the brake pedal (`Accelerator::update`) and shares the same `accel_permitted` gate, so it cannot produce torque when the above conditions are not met.

---

### Warning Banner

The warning banner at the bottom of both display pages shows the single highest-priority active condition. Only one message is shown at a time; the priority order is fixed. The main page also shows a total warning/error count (counts all set DTC flag bits and stale CAN flags).

> Some conditions appear in both the critical overlay and the warning banner simultaneously. For example, `DischargeLimitEnforcementFault` triggers both the BMS Non-Operational overlay and a "Battery Current Fault" or "BMS Fault" banner entry. The overlay covers the centre of the screen; the banner is always visible below it.

| Priority | Banner text | PDLInfo field(s) | Trigger |
|----------|-------------|-----------------|---------|
| 1 | ISOLATION FAULT | `bms_dtc_flags2_2` | `HighVoltageIsolationFault` |
| 2 | CAN: BMS Safety Stale | `bms_safety_stale` | BMS safety message not received within 3× its nominal period |
| 3 | CAN: BMS Voltage Stale | `bms_voltage_stale` | BMS voltage message stale |
| 4 | CAN: BMS Power Stale | `bms_power_stale` | BMS power message stale |
| 5 | CAN: MC Errors Stale | `mc_errors_stale` | Motor controller error message stale |
| 6 | CAN: MC Bus Stale | `mc_bus_stale` | Motor controller bus message stale |
| 7 | CAN: MC Speed Stale | `mc_speed_stale` | Motor controller speed message stale |
| 8 | CAN: MC Temp Stale | `mc_temp_stale` | Motor controller temperature message stale |
| 9 | CAN: Distro Stale | `power_distro_stale` | Power distro message stale |
| 10 | Battery Voltage HIGH | `bms_dtc_flags2_1` | `HighestCellVoltageAbove5vFault` |
| 11 | Battery Voltage LOW | `bms_dtc_flags1`, `bms_dtc_flags2_1` | `HighestCellVoltageTooLowFault`, `LowestCellVoltageTooLowFault`, `LowCellVoltageFault`, or `WeakCellFault` |
| 12 | Battery Current Fault | `bms_dtc_flags1`, `bms_dtc_flags2_1`, `bms_dtc_flags2_2` | `DischargeLimitEnforcementFault`, `CurrentSensorFault`, or `ChargeLimitEnforcementFault` |
| 13 | Battery Temp Fault | `bms_dtc_flags1`, `bms_dtc_flags2_2` | `PackTooHotFault`, `ThermistorFault`, or `FanMonitorFault` |
| 14 | BMS Fault | `bms_dtc_flags1`, `bms_dtc_flags2_1`, `bms_dtc_flags2_2` | `ChargerSafetyRelayFault`, `InternalHardwareFault`, `InternalHeatsinkThermistorFault`, `InternalSoftwareFault`, `InternalCommunicationFault`, `CellBalancingStuckOffFault`, `OpenWiringFault`, `CellAsicFault`, `WeakPackFault`, `ExternalCommunicationFault`, `RedundantPowerSupplyFault`, or `InputPowerSupplyFault` |
| 15 | Aux LV Fault | `monitor_status` | `AuxHardwareDetectedFault` or `AuxPowerMonitorI2cError` |
| 16 | Motor Controller Fault | `mc_error_flags1`, `mc_error_flags2` | Any error flag bit non-zero |
| 17 | Main LV Fault | `monitor_status` | `MainHardwareDetectedFault` or `MainPowerMonitorI2cError` |
| 18 | Aux LV Overvoltage ERROR | `aux_status` | `AuxOverVoltageError` |
| 19 | Aux LV Undervoltage ERROR | `aux_status` | `AuxUnderVoltageError` |
| 20 | Aux LV Overcurrent ERROR | `aux_status` | `AuxOverCurrentError` |
| 21 | Aux LV Undercurrent ERROR | `aux_status` | `AuxUnderCurrentError` |
| 22 | Main LV Overvoltage ERROR | `main_status` | `MainOverVoltageError` |
| 23 | Main LV Undervoltage ERROR | `main_status` | `MainUnderVoltageError` |
| 24 | Main LV Overcurrent ERROR | `main_status` | `MainOverCurrentError` |
| 25 | Main LV Undercurrent ERROR | `main_status` | `MainUnderCurrentError` |
| 26 | Motor Temp Limit | `mc_limit_flags` | `McLimitFlags::IpmMotorTemperature` active |
| 27 | Aux LV Overvoltage WARN | `aux_status` | `AuxOverVoltageWarning` |
| 28 | Aux LV Undervoltage WARN | `aux_status` | `AuxUnderVoltageWarning` |
| 29 | Aux LV Overcurrent WARN | `aux_status` | `AuxOverCurrentWarning` |
| 30 | Aux LV Undercurrent WARN | `aux_status` | `AuxUnderCurrentWarning` |
| 31 | Main LV Overvoltage WARN | `main_status` | `MainOverVoltageWarning` |
| 32 | Main LV Undervoltage WARN | `main_status` | `MainUnderVoltageWarning` |
| 33 | Main LV Overcurrent WARN | `main_status` | `MainOverCurrentWarning` |
| 34 | Main LV Undercurrent WARN | `main_status` | `MainUnderCurrentWarning` |

