/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: Copyright (c) 2021 Jason Skuby (mytechtoybox.com)
 */

#ifndef FLASHPROM_H_
#define FLASHPROM_H_

#include <stdint.h>
#include <string.h>
#include <pico/lock_core.h>
#include <pico/multicore.h>
#include <hardware/flash.h>
#include <hardware/timer.h>

// The saved config sits at the very end of the region, right before the end of
// the 2 MB flash, so growing the region downward keeps existing configs readable
// (and older firmware, which reads the last 32k, still finds them).
#define EEPROM_SIZE_BYTES    0xA000           // Reserve 40k of flash memory (ensure this value is divisible by 4096)
#define EEPROM_ADDRESS_END   _u(0x10200000)
#define EEPROM_ADDRESS_START (EEPROM_ADDRESS_END - EEPROM_SIZE_BYTES)
// Where the arduino-pico EEPROM lib, and the pre-protobuf config, started
#define LEGACY_EEPROM_ADDRESS_START _u(0x101F8000)

// Warning: If the write wait is too long it can stall other processes
#define EEPROM_WRITE_WAIT    50             // Amount of time in ms to wait before blocking core1 and committing to flash

class FlashPROM
{
	public:
		void start();
		void commit();
		void reset();

		static uint8_t writeCache[EEPROM_SIZE_BYTES];
};

inline FlashPROM EEPROM;

#endif
