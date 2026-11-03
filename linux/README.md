# GP2040-CE for Raspberry Pi 5 / CM5 / Pi 4 / CM4

`gp2040-linux` runs GP2040-CE controller logic on a Raspberry Pi running Linux.
The Pi shows up to the host (PC, Switch) as a USB controller through its USB
device port, using the kernel's USB gadget support. Buttons are wired to the
40-pin header.

This is a separate program from the RP2040/RP2350 firmware in the repository
root. It reuses the firmware's HID descriptors and report layouts from
`headers/drivers/`, and it does not change or take part in the firmware build.

## Polling rate: 8 kHz and where it is possible

USB interrupt endpoints are polled by the host. The device only asks for an
interval (`bInterval`); the host decides.

| Link speed | Unit of `bInterval` | Fastest possible | Boards |
|---|---|---|---|
| High Speed (480 Mbit/s) | 125 µs microframe, 2^(N-1) | **8 kHz** (`bInterval 1`) | Pi 5, CM5, Pi 4, CM4, Pi Zero 2 W |
| Full Speed (12 Mbit/s) | 1 ms frame | **1 kHz** (`bInterval 1`) | RP2040, RP2350 (all GP2040-CE firmware boards) |

* The RP2040 and RP2350 USB controllers are Full Speed only, so 8 kHz is not
  possible on them. The firmware already asks for `bInterval 1` in most modes,
  which is the 1 kHz Full Speed maximum. Nothing changes for those boards.
* On the Pi, `poll_rate_hz` (8000/4000/2000/1000/500/250/125) sets the High
  Speed `bInterval`. The value needs the f_hid `interval` configfs option,
  which was added to mainline Linux in 2025. On older kernels
  `gp2040-gadget` prints a warning and the kernel default (1 kHz) is used.
* If the Pi ends up on a Full Speed link (a Full Speed hub, or a host port that
  is Full Speed only), `bInterval 1` is read as 1 ms, so it falls back to
  1 kHz on its own.
* The host still decides. Windows and Linux PCs honour 8 kHz on High Speed. Game
  consoles often poll slower whatever the device asks for.

## How reports are sent

* **Full state only.** Every report carries the full button and stick state,
  never deltas, so a lost or skipped report can't leave a button stuck.
* **Send on change.** A report is written only when input changes. Between
  changes the device NAKs the host's polls. `heartbeat_hz` can add a periodic
  re-send (for example 1000).
* **Latest only.** f_hid keeps one transfer in flight. While it is busy,
  `gp2040-linux` holds only the newest report and replaces it on every change,
  so the host never receives a queue of old states.
* **Input.** Lines are read through the kernel GPIO character device
  (`/dev/gpiochipN`, uAPI v2). `input_method = events` sleeps on GPIO edge
  interrupts. `input_method = busypoll` samples in a tight loop and is meant for
  a core you reserve (`isolcpus=3 nohz_full=3` in `cmdline.txt`, then `cpu = 3`
  and `rt_priority = 80`).
* **Debounce.** Eager: a change is sent at once, then bounces on that pin are
  ignored for `debounce_us`. When the window ends the pin is read again so a
  release inside the window isn't lost.
* **SOCD and d-pad mode.** Same options as the firmware: up priority, neutral,
  last input wins, first input wins, off. The d-pad can act as the left or
  right stick.
* **Analog.** The Pi has no ADC, so there's no analog input yet. The deadzone
  and hysteresis filter (`AnalogFilter`) is ready for an external ADC so noise
  won't turn into a stream of reports.

## Supported modes

| GP2040-CE mode | On the Pi | Notes |
|---|---|---|
| Nintendo Switch (HORI HID) | **Yes**, `mode = switch` | Plain HID with VID/PID, works with f_hid. |
| DirectInput / generic HID | **Yes**, `mode = dinput` | Same descriptor as the firmware's HID mode. |
| Keyboard | Not yet | Plain HID; can be added with a second f_hid descriptor. |
| PS3 | Not yet | Needs replies to HID feature `GET_REPORT` requests, which f_hid doesn't handle. Needs a FunctionFS function. |
| Switch Pro | Not yet | Needs the subcommand handshake over OUT reports. f_hid can do it (`/dev/hidg0` is read/write); the protocol code needs porting. |
| XInput (Xbox 360) | Not yet | Not a HID class device (vendor class 0xFF). Needs a FunctionFS function with custom descriptors. A 360 console also needs XSM3 auth, which the firmware already implements. |
| Xbox One / Series | Not yet | GIP vendor protocol over FunctionFS, plus auth through a licensed controller on one of the Pi's USB host ports. |
| PS4 / PS5 | Not yet | HID plus console auth (key files or a dongle on a host port). |

## Hardware setup

### Raspberry Pi 5 / CM5

1. The USB-C port is the device port. Add to `/boot/firmware/config.txt`:

   ```
   dtoverlay=dwc2,dr_mode=peripheral
   ```

2. **Power.** The USB-C port is also the Pi 5's power input, and a PC or console
   port can't supply enough current. Power the Pi through the GPIO 5V pins or a
   USB-C power/data splitter, and don't feed 5V back into the host. Check the
   official Raspberry Pi power guidance before wiring this.

### Raspberry Pi 4 / CM4

* Pi 4: same as the Pi 5. The USB-C port is the device port and also the power
  input.
* CM4: add `dtoverlay=dwc2,dr_mode=peripheral` and use the IO board's USB 2.0
  device port (micro-USB on the official IO board). On the CM4, the
  BCM2711's USB 2.0 controller also drives the IO board's USB-A ports, so they
  stop working while it is in device mode.

### Install

```sh
sudo apt install cmake g++
cmake -S linux -B build-linux
cmake --build build-linux
sudo cmake --install build-linux --prefix /usr/local
sudo cp linux/gp2040-linux.conf.example /etc/gp2040-linux.conf   # then edit the pin map
sudo systemctl enable --now gp2040-gadget gp2040-linux
```

`gp2040-gadget up` builds the gadget with configfs. It gets the VID/PID,
strings, report descriptor and polling interval from `gp2040-linux` itself, so
they always match what the daemon sends. Changing `mode` or `poll_rate_hz`
needs `sudo systemctl restart gp2040-gadget gp2040-linux`.

Use `gp2040-linux --dry-run` to print reports to the terminal instead of
sending them, which helps when checking the wiring.

### Check the polling rate

On a Linux host: `sudo lsusb -v -d 0f0d:0092 | grep bInterval` should show
`bInterval 1`, and `evhz` or `usbmon` timestamps show the real rate. On Windows,
use a polling-rate tester.

## Tests

```sh
cmake -S linux -B build-linux && cmake --build build-linux
ctest --test-dir build-linux --output-on-failure
```

The tests run on any Linux machine and cover debounce, SOCD, d-pad mode, the
analog filter, report layouts against the firmware headers, the latest-only
sender and the config parser.
