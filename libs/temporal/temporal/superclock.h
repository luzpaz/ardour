/*
 * Copyright (C) 2017 Paul Davis <paul@linuxaudiosystems.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#pragma once

#include <stdint.h>

#include "pbd/integer_division.h"

#include "temporal/visibility.h"

#ifdef DEBUG_EARLY_SCTS_USE
#include <cstdlib>
#include <csignal>
#endif

namespace Temporal {

typedef int64_t superclock_t;
LIBTEMPORAL_API extern superclock_t _superclock_ticks_per_second;

#ifdef DEBUG_EARLY_SCTS_USE
static inline superclock_t superclock_ticks_per_second() {
	if (_superclock_ticks_per_second == 0) {
		raise (SIGUSR2);
	}
	return _superclock_ticks_per_second;
}
#else
static inline superclock_t superclock_ticks_per_second() { return _superclock_ticks_per_second; }
#endif

static inline superclock_t superclock_to_samples (superclock_t s, int sr) { return PBD::muldiv_floor (s, sr, superclock_ticks_per_second()); }
static inline superclock_t samples_to_superclock (int64_t samples, int sr) { return PBD::muldiv_round (samples, superclock_ticks_per_second(), superclock_t (sr)); }

LIBTEMPORAL_API extern int most_recent_engine_sample_rate;

LIBTEMPORAL_API void set_sample_rate (int sr);
LIBTEMPORAL_API void set_superclock_ticks_per_second (superclock_t sc);

}

#define TEMPORAL_SAMPLE_RATE (Temporal::most_recent_engine_sample_rate)

