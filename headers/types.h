/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: Copyright (c) 2023 Brian S. Stephan <bss@incorporeal.org>
 */

#ifndef TYPES_H_
#define TYPES_H_

// common types
#define	Pin_t		int32_t		// signed to accommodate for -1
#define Mask_t		uint64_t
// input pins the firmware tracks: real GPIOs plus keyboard matrix switches (see keymatrix.h)
#define NUM_INPUT_PINS	64

#endif
