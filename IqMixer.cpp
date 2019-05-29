/**
 * IQ mixer simulation
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
#include <fstream>
#include "Mixer.h"
#include "Signal.h"

using namespace std;

//===================================================================

/**
 * Constructor
 *
 * @param lpFreqHz low pass filter cutoff frequency
 */
IqMixer::IqMixer(const floating lpFreqHz) :
  lpFreqHz(lpFreqHz) {}

//===================================================================

/**
 * This simulates a conventional multiplying IQ mixer.  It outputs the
 * results into a CSV data file.
 *
 * @param outputFilename output filename
 * @param cycleCount number of carrier cycles to simulate
 * @pram signal signal characteristics
 * @param phaseAngleDeg Initial phase angle of carrier compared to 
 *                      local oscillator
 */
auto IqMixer::run(const string& outputFilename,
		  size_t cycleCount,
		  const Signal& signal,
		  floating phaseAngleDeg) -> void {

  cout << "Writing " << outputFilename << endl;
  reset();
  
  const auto headings = "timesteps, time, signal, localOsc, "
    "modulation, inphase, quadrature, filteredInphase, "
    "filteredQuadrature, demodulated"s;

  constexpr auto INDEX_SIGNAL = size_t{0};
  constexpr auto INDEX_LOCAL_OSC = size_t{1};
  constexpr auto INDEX_MODULATION = size_t{2};
  constexpr auto INDEX_INPHASE = size_t{3};
  constexpr auto INDEX_QUADRATURE = size_t{4};
  constexpr auto INDEX_FILTERED_INPHASE = size_t{5};
  constexpr auto INDEX_FILTERED_QUADRATURE = size_t{6};
  constexpr auto INDEX_DEMODULATED = size_t{7};
  
  // Add a few extra cycles to let the simulation stabilise
  cycleCount += EXTRA_CYCLES;

  auto timeStepsPerCarrierCycle = signal.getTimeStepsPerCarrierCycle(0);
    
  auto totalTimeSteps = size_t{0};

  // Local oscillator is phaseAngle behind carrier
  auto localOscillator = Signal{signal.getCarrierAmplitude(0),
				signal.getCarrierFreqHz(0),
				signal.getModFreqHz(0),
				-phaseAngleDeg};

  for (auto cycles = decltype(cycleCount){0}; cycles < cycleCount; cycles++) {
    for (auto timeStep = decltype(timeStepsPerCarrierCycle){1};
	 timeStep <= timeStepsPerCarrierCycle; timeStep++) {

      totalTimeSteps++;
      auto signalVoltage = signal.getTotalSignal(totalTimeSteps);
      auto localOscRadians = localOscillator.getRadians(0, totalTimeSteps);

      auto dataLine = unique_ptr<DataLine>{new DataLine(INDEX_DEMODULATED + 1,
							totalTimeSteps)};
      dataLine->fields.at(INDEX_SIGNAL) = signalVoltage;
      dataLine->fields.at(INDEX_LOCAL_OSC) = localOscRadians;
      dataLine->fields.at(INDEX_MODULATION) = signal.getAmplitude(0, totalTimeSteps);
      dataLine->fields.at(INDEX_INPHASE) = signalVoltage * sin(localOscRadians);
      dataLine->fields.at(INDEX_QUADRATURE) = signalVoltage * cos(localOscRadians);

      add(dataLine);
    }
  }

  butterworth(INDEX_INPHASE, INDEX_FILTERED_INPHASE,
	      2, lpFreqHz, false);
  
  butterworth(INDEX_QUADRATURE, INDEX_FILTERED_QUADRATURE,
	      2, lpFreqHz, false);
  
  amDemod(INDEX_FILTERED_INPHASE,
	  INDEX_FILTERED_QUADRATURE,
	  INDEX_DEMODULATED);

  outputData(outputFilename, headings, timeStepsPerCarrierCycle);  
}

