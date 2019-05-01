/**
 * Signal structure describes the RF signal
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

#include <iostream>
#include "Signal.h"

using namespace std;

/**
 * Constructor
 *
 * @param carrierAmplitude carrier amplitude
 * @param carrierFreqHz carrier frequency
 * @param modFreqHz modulation frequency
 * @param phaseAngleDegrees phase angle of signal
 */
Signal::SingleSignal::SingleSignal(floating carrierAmplitude,
				   floating carrierFreqHz,
				   floating modFreqHz,
				   floating initialPhaseAngleDegrees) :
  carrierAmplitude{carrierAmplitude},
  carrierFreqHz{carrierFreqHz},
  modFreqHz{modFreqHz},
  initialPhaseAngleRadians{initialPhaseAngleDegrees * M_PI / 180.0},
  timeStepsPerCarrierCycle{1.0 / (TIME_STEP_SIZE * carrierFreqHz)} {}

/**
 * Get a carrier signal angle at specified time step.
 *
 * @param timeStep time step
 * @return sum of all the single signals at this time
 */
auto Signal::SingleSignal::getRadians(size_t timeStep) const -> floating {
  return initialPhaseAngleRadians +
    timeStepsIntoACycle(timeStep) * 2.0 * M_PI / timeStepsPerCarrierCycle;
}

/**
 * Get the current value of the modulated signal
 *
 * @param timeStep get the modulation for this time step
 * @return instananeous signal amplitude due to modulation
 */
auto Signal::SingleSignal::getAmplitude(size_t timeStep) const -> floating {
  auto radiansPerSecond = floating{2.0 * M_PI * modFreqHz};
  auto radians = radiansPerSecond * timeStep * TIME_STEP_SIZE;
  radians += initialPhaseAngleRadians;
  return carrierAmplitude * cos(radians);
}

/**
 * Get the instantanous signal level.
 *
 * @param timeStep time step
 * @return signal level
 */
auto Signal::SingleSignal::getSignal(size_t timeStep) const -> floating {
  return getAmplitude(timeStep) * sin(getRadians(timeStep));
}

/**
 * Get the number of time steps into the current cycle, ignoring
 * the initial phase angle.
 *
 * @param timeStep time step
 * @param time steps into the current cycle.
 */
auto Signal::SingleSignal::timeStepsIntoACycle(size_t timeStep) const
  -> floating {
  // This is potentially a narrowing conversion
  auto timeStepFloating = static_cast<floating>(timeStep);
  auto completeCycles = floor(timeStepFloating / timeStepsPerCarrierCycle);  
  return timeStepFloating - timeStepsPerCarrierCycle * completeCycles;
}

//===================================================================

/**
 * Adds another single signal to the signal vector
 *
 * @param carrierAmplitude carrier amplitude
 * @param carrierFreqHz carrier frequency
 * @param modFreqHz modulation frequency
 * @param phaseAngleDegrees phase angle of signal
 */
auto Signal::add(floating carrierAmplitude,
		 floating carrierFreqHz,
		 floating modFreqHz,
		 floating phaseAngleDegrees) -> void {
  signals.push_back(SingleSignal(carrierAmplitude,
				 carrierFreqHz,
				 modFreqHz,
				 phaseAngleDegrees));
}

/**
 * Constructor adds a single signal to the signal vector
 *
 * @param carrierAmplitude carrier amplitude
 * @param carrierFreqHz carrier frequency
 * @param modFreqHz modulation frequency
 * @param phaseAngleDegrees phase angle of signal
 */
Signal::Signal(floating carrierAmplitude,
	       floating carrierFreqHz,
	       floating modFreqHz,
	       floating phaseAngleDegrees) {
  add(carrierAmplitude,
      carrierFreqHz,
      modFreqHz,
      phaseAngleDegrees);
}
  
/**
 * Get the current value of the modulated signal
 *
 * @param index signal index
 * @param timeStep get the modulation for this time step
 * @return instananeous signal amplitude due to modulation
 */
auto Signal::getAmplitude(size_t index, size_t timeStep)
  const -> floating {
  return signals.at(index).getAmplitude(timeStep);
}

/**
 * Get the carrier amplitude
 *
 * @param index signal index
 * @return carrier amplitude
 */
auto Signal::getCarrierAmplitude(size_t index)
  const -> floating {
  return signals.at(index).carrierAmplitude;
}

/**
 * Get modulation frequency for the specified signal
 *
 * @param index signal index
 * @return modulation frequency
 */
auto Signal::getModFreqHz(size_t index) const -> floating {
  return signals.at(index).modFreqHz;
}

/**
 * Get carrier frequency for the specified signal
 *
 * @param index signal index
 * @return modulation frequency
 */
auto Signal::getCarrierFreqHz(size_t index) const -> floating {
  return signals.at(index).carrierFreqHz;
}

/**
 * Get the number of time steps per carrier cycle
 *
 * @param index single signal index
 * @return time steps per carrier cycle
 */

auto Signal::getTimeStepsPerCarrierCycle(size_t index) const -> floating {
  return signals.at(index).timeStepsPerCarrierCycle;
}

/**
 * Get a carrier signal angle at specified time step.
 *
 * @param index signal index
 * @param timeStep time step
 * @return sum of all the single signals at this time
 */
auto Signal::getRadians(size_t index,
			size_t timeStep) const -> floating {
  return signals.at(index).getRadians(timeStep);
}

/**
 * Get total signal voltage at specified time step.
 *
 * @param timeStep time step
 * @return sum of all the single signals at this time
 */
auto Signal::getTotalSignal(size_t timeStep) const -> floating {
  auto signalVoltage = floating{0};
  for (auto&& signal : signals) {
    signalVoltage += signal.getSignal(timeStep);
  }
  return signalVoltage;
}
