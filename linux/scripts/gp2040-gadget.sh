#!/bin/sh
# SPDX-License-Identifier: MIT
# SPDX-FileCopyrightText: Copyright (c) 2026 OpenStickCommunity (gp2040-ce.info)
#
# Create or remove the GP2040-CE USB HID gadget with configfs.
#
#   gp2040-gadget up   [CONFIG]   build the gadget and bind it to the USB device port
#   gp2040-gadget down            unbind and remove it
#
# The USB identity, report descriptor and polling rate come from
# gp2040-linux itself, so they always match the reports it sends.
#
# Needs: dtoverlay=dwc2,dr_mode=peripheral in config.txt, and the
# libcomposite module (loaded below).

set -eu

CMD="${1:-}"
CONFIG="${2:-/etc/gp2040-linux.conf}"
GP2040_LINUX="${GP2040_LINUX:-gp2040-linux}"
G=/sys/kernel/config/usb_gadget/gp2040

log() { echo "gp2040-gadget: $*" >&2; }

down() {
    [ -d "$G" ] || return 0
    if [ -n "$(cat "$G/UDC" 2>/dev/null)" ]; then
        echo "" > "$G/UDC"
    fi
    rm -f "$G/configs/c.1/hid.usb0"
    [ -d "$G/configs/c.1/strings/0x409" ] && rmdir "$G/configs/c.1/strings/0x409"
    [ -d "$G/configs/c.1" ] && rmdir "$G/configs/c.1"
    [ -d "$G/functions/hid.usb0" ] && rmdir "$G/functions/hid.usb0"
    [ -d "$G/strings/0x409" ] && rmdir "$G/strings/0x409"
    rmdir "$G"
}

up() {
    modprobe libcomposite
    if ! mountpoint -q /sys/kernel/config; then
        mount -t configfs none /sys/kernel/config
    fi
    down

    eval "$("$GP2040_LINUX" --config "$CONFIG" --print-gadget-env)"

    mkdir -p "$G"
    echo "$GADGET_VID" > "$G/idVendor"
    echo "$GADGET_PID" > "$G/idProduct"
    echo "$GADGET_BCD_DEVICE" > "$G/bcdDevice"
    echo 0x0200 > "$G/bcdUSB"
    mkdir -p "$G/strings/0x409"
    echo "$GADGET_MANUFACTURER" > "$G/strings/0x409/manufacturer"
    echo "$GADGET_PRODUCT" > "$G/strings/0x409/product"

    F="$G/functions/hid.usb0"
    mkdir -p "$F"
    echo 0 > "$F/protocol"
    echo 0 > "$F/subclass"
    echo "$GADGET_REPORT_LENGTH" > "$F/report_length"
    "$GP2040_LINUX" --config "$CONFIG" --print-report-desc > "$F/report_desc"

    # Interrupt endpoint polling interval. On a High Speed link bInterval N
    # means every 2^(N-1) x 125 us, so 1 = 8 kHz and 4 = 1 kHz. If the link
    # falls back to Full Speed the same value counts in 1 ms frames, so
    # bInterval 1 there is 1 kHz, the Full Speed maximum.
    if [ -e "$F/interval" ]; then
        echo "$GADGET_HS_INTERVAL" > "$F/interval"
        log "polling: ${GADGET_POLL_RATE_HZ} Hz requested (bInterval $GADGET_HS_INTERVAL)"
    else
        log "this kernel's f_hid has no 'interval' option; the host will poll at"
        log "the kernel default (bInterval 4 = 1 kHz on High Speed). A newer kernel"
        log "is needed for 2/4/8 kHz."
    fi

    mkdir -p "$G/configs/c.1/strings/0x409"
    echo "GP2040-CE" > "$G/configs/c.1/strings/0x409/configuration"
    echo 500 > "$G/configs/c.1/MaxPower"
    echo 0x80 > "$G/configs/c.1/bmAttributes"
    ln -s "$F" "$G/configs/c.1/hid.usb0"

    UDC_NAME="$(ls /sys/class/udc | head -n 1)"
    if [ -z "$UDC_NAME" ]; then
        log "no USB device controller found. Add dtoverlay=dwc2,dr_mode=peripheral"
        log "to /boot/firmware/config.txt and reboot."
        exit 1
    fi
    echo "$UDC_NAME" > "$G/UDC"
    log "bound $GADGET_MODE gadget ($GADGET_VID:$GADGET_PID) to $UDC_NAME"
}

case "$CMD" in
    up) up ;;
    down) down ;;
    *) echo "usage: $0 up [CONFIG] | down" >&2; exit 2 ;;
esac
