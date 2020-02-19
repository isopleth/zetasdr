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

#pragma once

#include "misc.h"
#include <cmath>
#include <cstddef>
#include <vector>

//===================================================================
 
class Signal {
private:
  struct SingleSignal {
    const floating carrierAmplitude;
    const floating carrierFreqHz;
    const floating modFreqHz;
    const floating initialPhaseAngleRadians;
    const floating timeStepsPerCarrierCycle;
    
    SingleSignal(floating carrierAmplitude,
		 floating carrierFreqHz,
		 floating modFreqHz,
		 floating initialPhaseAngleDegrees);

    auto getAmplitude(std::size_t timeStep) const -> floating;
    auto getRadians(std::size_t timeStep) const -> floating;
    auto getSignal(std::size_t timeStep) const -> floating;
    auto timeStepsIntoACycle(std::size_t timeStep) const -> floating;
  };

  std::vector<SingleSignal> signals;
  
public:

  Signal(floating carrierAmplitude,
	 floating carrierFreqHz,
	 floating modFreqHz,
	 floating initialPhaseAngleDegrees = 0);
    
  auto add(floating carrierAmplitude,
	   floating carrierFreqHz,
	   floating modFreqHz,
	   floating initialPhaseAngleDegrees = 0) -> void;
  
  auto getCarrierAmplitude(std::size_t index) const -> floating;
  auto getAmplitude(std::size_t index, std::size_t timeStep) const -> floating;
  auto getModFreqHz(std::size_t index) const -> floating;
  auto getCarrierFreqHz(std::size_t index) const -> floating;
  auto getTimeStepsPerCarrierCycle(std::size_t index) const -> floating;

  auto getRadians(std::size_t index,
		  std::size_t timeStep) const -> floating;
  auto getTotalSignal(std::size_t timeStep) const -> floating;
};


