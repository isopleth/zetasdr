/**
 * Non-Spice simulation of ZetaSDR radio
 * http://www.qrz.lt/ly1gp/SDR
 *
 * Various definitions
 *
 * Copyright 2019  Jason Leake
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once

#include <list>
#include <memory>
#include <string>
#include <vector>

// So we can change the precision easily...
using floating = long double;

// 10 picoseconds time step size
constexpr auto TIME_STEP_SIZE = floating{1e-11}; 

// Extra cycles at the start to get output stable
constexpr auto EXTRA_CYCLES = 100;

// 1 nanosecond resolution in output files
constexpr auto OUTPUT_RESOLUTION = floating{1e-9};

//===================================================================

struct Circuit {
  const floating resistance;
  const floating capacitance;
  const floating lpFreqHz;

  Circuit(floating resistance,
	  floating capacitance,
	  floating lpFreqHz) : resistance{resistance},
    capacitance{capacitance},
    lpFreqHz{lpFreqHz} {
  }
};

