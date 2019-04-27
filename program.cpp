/**
 * Non-Spice simulation of ZetaSDR radio
 * http://www.qrz.lt/ly1gp/SDR
 *
 * Entry point
 *
 * Specifically it demonstrates how the Tayloe quadrature product
 * detector.
 *
 * The modulation frequency is an unrealistic 200 kHz to provide an
 * intelligible plot.  In practice, 7 MHz is in an amateur band and AM
 * modulation would probably be below 10 kHz. The active filter op-amp
 * circuits aren't simulated because they have a cut-off of around 10
 * kHz, which will attenuate the 200 kHz signal too strongly.  Equally
 * the carrier amplitude is set to 1 mV, which will saturate the
 * active filters because of their gain.
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

// The program is using AAA (almost-always-auto) style, in case you
// are wondering

#include <fstream>
#include <iostream>
#include "Mixer.h"
#include "Signal.h"

using namespace std;

// 7 MHz RF carrier frequency
constexpr auto CARRIER_FREQUENCY =  floating{7e6};

// 100 kHz amplitude modulation
constexpr auto MODULATION_FREQUENCY = floating{1e5};

// 7.5 MHz adjacent frequency
constexpr auto ADJ_CARRIER_FREQUENCY = floating{7.6e6};

// 83 kHz adjacent signal amplitude modulation
constexpr auto ADJ_MODULATION_FREQUENCY = floating{8.3e4};

// Use this where we don't want modulation
constexpr auto NO_MODULATION = floating{0};

// 1 mV
constexpr auto CARRIER_AMPLITUDE = floating{1e-3};

constexpr auto PHASE_ANGLE_DEGREES = floating{35};

// Two 74HC4052 channels in parallel at 70 ohms each
// = 50 ohm antenna impedance + 35 ohm through 74HC4052
constexpr auto RESISTANCE = floating{85};

// Detector capacitors C2-C5
constexpr auto CAPACITANCE = floating{0.022e-6}; // 0.022 uF

// 400 kHz cutoff
constexpr auto FILTER_CUTOFF = 4e5;

// Number of carrier cycles
constexpr auto CYCLES = 200;

//===================================================================

auto main() -> int {

  const auto zetaSdrCircuit = Circuit{RESISTANCE,
				      CAPACITANCE,
				      FILTER_CUTOFF};

  auto zetasdr = ZetaSdr{zetaSdrCircuit};
  auto iqmixer = IqMixer{FILTER_CUTOFF};

  // Signals to use
  const auto unmodulatedSignal = Signal{CARRIER_AMPLITUDE,
					CARRIER_FREQUENCY,
					NO_MODULATION};

  const auto modulatedSignal = Signal{CARRIER_AMPLITUDE,
				      CARRIER_FREQUENCY,
				      MODULATION_FREQUENCY};

  // Same signal as before but with additional signal 0.5 MHz away
  auto adjacentSignal = modulatedSignal;
  adjacentSignal.add(CARRIER_AMPLITUDE,
		     ADJ_CARRIER_FREQUENCY,
		     ADJ_MODULATION_FREQUENCY);

  // The ZetaSDR radio simulation and the IQ mixer tune themselves to
  // the first element of the signal object.  So swap the two elements
  // over to get these simulators to tune to the adjacent frequency
  // instead.
  auto tunedToAdjacentSignal = Signal(CARRIER_AMPLITUDE,
				      ADJ_CARRIER_FREQUENCY,
				      ADJ_MODULATION_FREQUENCY);
  tunedToAdjacentSignal.add(CARRIER_AMPLITUDE,
			    CARRIER_FREQUENCY,
			    MODULATION_FREQUENCY);

  /*
   * Unmodulated carrier, in phase with local oscillator
   */
  zetasdr.run("zetasdr_unmodulated_0.txt", 4,
  	      unmodulatedSignal, 0);

  /*
   * Unmodulated carrier, with 35 degree phase difference in start
   * state compared to local oscillator
   */
  zetasdr.run("zetasdr_unmodulated_35.txt", 4,
	      unmodulatedSignal, PHASE_ANGLE_DEGREES);

  /*
   * Modulated carrier, in phase with local oscillator
   */
  zetasdr.run("zetasdr_modulated_0.txt", CYCLES,
	      modulatedSignal, 0);


  /*
   * Modulated carrier with 35 degree phase difference in initial
   * state compared to local oscillator
   */
  zetasdr.run("zetasdr_modulated_35.txt", CYCLES,
	      modulatedSignal, PHASE_ANGLE_DEGREES);

  /*
   * Modulated carrier with 35 degree phase difference in initial
   * state compared to local oscillator, plus another signal 0.5 MHz
   * higher frequency.
   */
  zetasdr.run("zetasdr_adjacent_35.txt", CYCLES,
	      adjacentSignal, PHASE_ANGLE_DEGREES);

  /*
   * Ideal multiplying IQ mixer.
   */
  iqmixer.run("iq_modulated_0.txt", CYCLES,
	      modulatedSignal, 0);

  /*
   * Ideal multiplying IQ mixer with 35 degree phase difference
   */
  iqmixer.run("iq_modulated_35.txt", CYCLES,
	      modulatedSignal, PHASE_ANGLE_DEGREES);

  /*
   * Ideal multiplying IQ mixer with adjacent signal present
   */
  iqmixer.run("iq_adjacent_35.txt", CYCLES,
	      adjacentSignal, PHASE_ANGLE_DEGREES);

  /*
   * Tune the ZetaSDR to the adjacent channel and see what that looks
   * like.
   */
  zetasdr.run("zetasdr_tuned_adjacent_35.txt", CYCLES,
	      tunedToAdjacentSignal, PHASE_ANGLE_DEGREES);

  /*
   * Tune the IQ mixer to the adjacent channel and see what that looks
   * like.
   */
  iqmixer.run("iq_tuned_adjacent_35.txt", CYCLES,
	      tunedToAdjacentSignal, PHASE_ANGLE_DEGREES);
}
