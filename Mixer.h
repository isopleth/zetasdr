/**
 * Class declaratons for the mixers
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

class Signal;

class Mixer {
  
 protected:
  struct DataLine {
    std::size_t timeStep;
    floating timeStamp;
    std::vector<floating> fields;
    
  DataLine(std::size_t fieldcount,
	   std::size_t timeStep) : timeStep{timeStep},
      timeStamp(timeStep * TIME_STEP_SIZE) {
	fields.reserve(fieldcount);
	for (auto index = size_t{0}; index < fieldcount; index++) {
	  fields.push_back(0.);
	}
      }
  };
  
  std::list<std::unique_ptr<DataLine>> results;

  Mixer() = default;

  auto reset() -> void;
  
  auto add(std::unique_ptr<DataLine>& newLine) -> void;

  auto butterworth(size_t inputIndex,
		   size_t outputIndex,
		   unsigned poles,
		   floating cutoffHz,
		   bool highPass) -> void;
  
  auto amDemod(size_t inphaseVectorIndex,
	       size_t quadratureVectorIndex,
	       size_t demodulatedOutputVector) -> void;

  auto outputData(const std::string& filename,
		  const std::string& headings,
		  floating timeStepsPerCarrierCycle) -> void;
  
  virtual ~Mixer() = default;

};

//===================================================================

class ZetaSdr : public Mixer {
 private:
  const Circuit& circuit;

 public: 
  ZetaSdr(const Circuit& circuit);
  auto run(const std::string& outputFilename,
	   size_t cycleCount,
	   const Signal& signal,
	   floating phaseAngleDeg) -> void;
  virtual ~ZetaSdr() = default;
};

//===================================================================

class IqMixer : public Mixer {
 private:
  const floating lpFreqHz;
  
 public:
  IqMixer(floating lpFreqHz);
  auto run(const std::string& outputFilename,
	   size_t cycleCount,
	   const Signal& signal,
	   floating phaseAngleDeg) -> void;

  virtual ~IqMixer() = default;
};
