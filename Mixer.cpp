/**
 * Superclass for mixer implementations
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

#include <algorithm>
#include <rtf_common.h>
#include <iostream>
#include <fstream>
#include "Mixer.h"
#include "Signal.h"

using namespace std;

//===================================================================

/**
 * Butterworth filter.  Input and output entries in the DataLine struct
 * can be the same
 *
 * @param inputIndex index of signal entry in the DataLine struct
 * @param outputIndex index of the filtered entry
 * @param poles number of poles
 * @param cutoffHz filter cut-off frequency
 * @param highPass true for high pass, false for low pass
 */
auto Mixer::butterworth(size_t inputIndex,
			size_t outputIndex,
			unsigned poles,
			floating cutoffHz,
			bool highPass) -> void {

  if (cutoffHz) {
    const auto normalisedCutoffFreq = 
      cutoffHz * static_cast<floating>(TIME_STEP_SIZE);

    auto filter =
      hfilter{rtf_create_butterworth(1,
				     RTF_DOUBLE,
				     static_cast<double>(normalisedCutoffFreq),
				     poles,
				     static_cast<int>(highPass))};

    if (filter == nullptr) {
      cout << "Unable to create low pass filter" << endl;
      exit(EXIT_FAILURE);
    }
   
    auto size = results.size();
    auto inputVector = vector<double>{};
    inputVector.reserve(size);
    auto outputVector = vector<double>{};
    outputVector.reserve(size);
    
    for (auto&& dataLine : results) {
      auto value = static_cast<double>(dataLine->fields.at(inputIndex));
      inputVector.push_back(value);
      outputVector.push_back(0.);
    }
    
    rtf_filter(filter, inputVector.data(), outputVector.data(), size);
    rtf_destroy_filter(filter);
    
    auto index = size_t{0};
    for (auto&& dataLine : results) {
      dataLine->fields.at(outputIndex) =
	static_cast<floating>(outputVector.at(index++));
    }
  }
  else {
    // Disabled, so just copy input to output
    for (auto&& dataLine : results) {
      dataLine->fields.at(outputIndex) = dataLine->fields.at(inputIndex);
    }
  }  
}

//===================================================================

/**
 * AM demodulation of the I/Q signal.  This is not part of the mixer
 * but this is a convenient place to put it.
 *
 * @param inphaseIndex index of inphase entry in the DataLine struct
 * @param quadratureIndex index of the quadrature entry in the 
 *                        DataLine struct
 * @param demodulatedOutputIndex index of the demodulated output 
 *                               entry in the DataLine struct
 */
auto Mixer::amDemod(size_t inphaseIndex,
		    size_t quadratureIndex,
		    size_t demodulatedOutputIndex) -> void {

  // Dealing with the signs is a bit problematic.  The easiest solution
  // is to add a DC offset so that all the I and Q values are positive
  // and remove it afterwards.

  // Initial values for minima
  auto minI = results.front()->fields.at(inphaseIndex);
  auto minQ = results.front()->fields.at(quadratureIndex);

  for (auto&& dataLine : results) {
    minI = min(minI, dataLine->fields.at(inphaseIndex));
    minQ = min(minQ, dataLine->fields.at(quadratureIndex));
  }

  // Set the offset to be aplied to each value
  minI = (minI < 0) ? - minI : 0;
  minQ = (minQ < 0) ? - minQ : 0;

  auto meanValue = floating{0};
  for (auto&& dataLine : results) {
    auto inphaseValue = dataLine->fields.at(inphaseIndex) + minI;
    auto inphaseSquare = inphaseValue * inphaseValue;
    auto quadratureValue = dataLine->fields.at(quadratureIndex) + minQ;
    auto quadratureSquare = quadratureValue * quadratureValue;
    auto value = sqrt(quadratureSquare + inphaseSquare);
    dataLine->fields.at(demodulatedOutputIndex) = value;
    meanValue += value;
  }

  meanValue = meanValue / results.size();

  // Now remove the DC level. Easiest way is to remove the mean
  // value
  for (auto&& dataLine : results) {
    dataLine->fields.at(demodulatedOutputIndex) -= meanValue;
  }

}

//===================================================================

/**
 * Add another DataLine entry to the results list
 *
 * @param dataline reference to dataLine entry to be added
 */
auto Mixer::add(unique_ptr<DataLine>& dataLine) -> void {
  results.emplace_back(move(dataLine));
}

//===================================================================

/**
 * Clean out the existing results.
 */
auto Mixer::reset() -> void {
  results.clear();
}

//===================================================================

/**
 * Write the output file, and deallocate the results list.
 *
 * @param outputFilename output filename
 * @param column headings
 * @param timeStepsPerCarrierCycle times steps per carrier cycle, for
 *              excluding the first cycles
 */
auto Mixer::outputData(const string& outputFilename,
		       const string& headings,
		       floating timeStepsPerCarrierCycle) -> void {
  auto file = ofstream(outputFilename);
  file.precision(9);
  file << scientific << "# " << headings << "\n";  

  const auto startTimeStepF = EXTRA_CYCLES * timeStepsPerCarrierCycle;
  const auto startTimeStep = static_cast<size_t>(startTimeStepF);
  // Output a result every 100 ns
  auto stepF = floating{OUTPUT_RESOLUTION / TIME_STEP_SIZE};
  auto step = static_cast<size_t>(stepF);
  
  auto started = false;
  auto oldTimeStep = size_t{0};

  for (auto&& dataLine : results) {
    auto timeStep = dataLine->timeStep;
    if (!started && timeStep >= startTimeStep) {
      started = true;
    }
    
    if (started && timeStep >= oldTimeStep + step) {
      file << timeStep << "," << dataLine->timeStamp;
      for (auto&& field : (*dataLine).fields) {
	file << "," << field;
      }
      file << "\n";
      oldTimeStep = timeStep; 
    }
  }
}
