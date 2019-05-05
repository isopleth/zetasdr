/**
 * Non-Spice simulation of ZetaSDR radio
 * http://www.qrz.lt/ly1gp/SDR
 *
 * Implements the ZetaSDR radio simulation itself
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

#include <array>
#include <fstream>
#include <iostream>
#include "Mixer.h"
#include "Signal.h"

using namespace std;

// Voltage corresponding to logic 1
constexpr auto LOGIC_ONE_VOLTAGE = floating{2.4};

//===================================================================

/**
 * This represents the Johnson counter constructed from two D type
 * flip flops.  In practice there will be some propagation delay
 * between the clock changing and the output from a counter changing,
 * but this is not modelled by this class.
 */
class JohnsonCounter {
  // This is the state, 0->3
  unsigned state;

  // The counter is a twisted ring counter, so
  // the output pattern is {AB} 00, 01, 11, 10
  static constexpr auto OUTPUT_VALUE = array<unsigned, 4>{0, 1, 3, 2};

public:
  JohnsonCounter() : state{0} {}

  /**
   * Advance the Johnson counter by one clock
   */
  auto clock() {
    if (++state > 3) {
      state = 0;
    }
  }

  /**
   * Get the number of states
   *
   * @return number of states
   */
  static auto stateCount() {
    return OUTPUT_VALUE.size();
  }

  /**
   * Get the current output value of the Johnson counter
   */
  auto get() {
    return OUTPUT_VALUE.at(state);
  }
};


/**
 * This represents the local oscillator.  This provides the clock to
 * the Johnson Counter
 */
class LocalOscillator {

private:
  // Current time step
  floating timeStep;
  // Number of time steps per carrier cycle
  floating timeStepsPerCycle;
  JohnsonCounter& johnsonCounter;
  floating voltage;
  bool errorFlagged;
  static constexpr const floating AMPLITUDE = 5.0;

  /**
   * Get voltage level of local oscillator at the current timestep
   *
   * @return the voltage
   */
  auto getVoltage() {
    auto value = floating{sin(2.0 * M_PI * timeStep / timeStepsPerCycle)};
    value = ((value + 1.0) / 2.0) * AMPLITUDE;
    if (!errorFlagged && isnan(value)) {
      cerr << value << " (LocalOscillator) is not a number" << endl;
      errorFlagged = true;
    }
    else {
      errorFlagged = false;
    }
    return value;
  }

public:
  /**
   * Constructor.  Set the start state of the counter, and the time
   * steps corresponding to a cycle of the carrier.  The phase offset
   * is handled by retarding the initial value of the current time
   * steps so that it takes the number of time steps corresponding to
   * the phase angle of the carrier before it reaches 0.
   *
   * @param frequencyHz local oscillator frequency
   * @param phaseOffsetRadians
   * @param johnsonCounter Johnson counter object that the oscillator 
   *                       drives
   */
  LocalOscillator(floating frequencyHz,
		  floating phaseOffsetRadians,
		  JohnsonCounter& johnsonCounter) :
    timeStep{static_cast<decltype(timeStep)>(-floor(phaseOffsetRadians))},
    timeStepsPerCycle{floor(1.0 / (TIME_STEP_SIZE * frequencyHz))},
    johnsonCounter{johnsonCounter},
    voltage{getVoltage()},
    errorFlagged{false} {
    }

  /**
   * Advance counter by one timestep.  The local oscillator which
   * drives the counter runs at four times the carrier frequency. The
   * phase difference between the local oscillator and the carrier is
   * handled by retarding the start value of the time step counter by
   * an amount corresponding to the initial phase difference.
   */
  auto step() {
    // One more time step
    timeStep++;
    auto previousVoltage = voltage;
    voltage = getVoltage();
    // Clock the Johnson counter when the local oscillator output
    // changes from logic 0 to logic 1
    if (previousVoltage < LOGIC_ONE_VOLTAGE &&
	voltage >= LOGIC_ONE_VOLTAGE) {
      johnsonCounter.clock();
    }
  }
};
  

/**
 * This represents a sample and hold capacitors on the outputs from
 * the 74HC4052.  It incorporates the resistance through the pair of
 * 74HC4052 channels.
 */
class SeriesRC {
private:
  const floating timeConstant;
  floating voltage;  // voltage currently across capacitor
  bool errorFlagged;
public:

  /**
   * Constructor
   *
   * @param capacitance capacitance in Farads
   * @param resistance resistance through 74HC4052
   */
  SeriesRC(const Circuit& circuit) :
    timeConstant{circuit.resistance * circuit.capacitance},
    voltage{0},
    errorFlagged{false} {}

  /**
   * Get the voltage across the capacitor
   *
   * @return the voltage across the capacitor
   */
  auto getVoltage() {
    return voltage;
  }

  /**
   * Apply the specified voltage for one time step.
   *
   * @param appliedVoltage applied voltage
   */
  auto applyVoltageForOneTimeStep(floating appliedVoltage) {
    auto voltageDifference = appliedVoltage - voltage;

    voltage += voltageDifference * exp(-TIME_STEP_SIZE / timeConstant);
				       
    if (!errorFlagged && isnan(voltage)) {
      cerr << voltage << " (SeriesRC) is not a number" << endl;
      errorFlagged = true;
    }
    else {
      errorFlagged = false;
    }
  }

  /**
   * Place holder
   */
  auto isolateForOneTimeStep() {
  }
};

//===================================================================

/**
 * Constructor
 *
 * @param circuit circit characteristics
 */
ZetaSdr::ZetaSdr(const Circuit& circuit) : circuit{circuit} {}

/**
 * This simulates the Tayloe quadrature product detector.  It outputs
 * the results into a data file. The phase angle is the phase of the
 * initial state of the carrier with respect to the initial state of
 * the local oscillator.
 *
 * @param outputFilename output filename
 * @param cycleCount number of carrier cycles to simulate
 * @param signal signal characteristics
 * @param phaseAngleDeg Initial phase angle of carrier compared to 
 *                      local oscillator
 */
auto ZetaSdr::run(const string& outputFilename,
		  size_t cycleCount,
		  const Signal& signal,
		  floating phaseAngleDeg) -> void {

  reset();
  cout << "Writing " << outputFilename << endl;

  const auto headings = "timestep, time, signal, modulation, C2, "
    "C3, C4, C5, IC2A, IC2B, "
    "filteredInphase, filteredQuadrature, demodulated";

  constexpr auto INDEX_SIGNAL = size_t{0};
  constexpr auto INDEX_MODULATION = size_t{1};
  constexpr auto INDEX_CAPC2_VOLTAGE = size_t{2};
  constexpr auto INDEX_CAPC3_VOLTAGE = size_t{3};
  constexpr auto INDEX_CAPC4_VOLTAGE = size_t{4};
  constexpr auto INDEX_CAPC5_VOLTAGE = size_t{5};
  constexpr auto INDEX_DIFFERENCE_IC2A = size_t{6};
  constexpr auto INDEX_DIFFERENCE_IC2B = size_t{7};
  constexpr auto INDEX_FILTERED_INPHASE = size_t{8};
  constexpr auto INDEX_FILTERED_QUADRATURE = size_t{9};
  constexpr auto INDEX_DEMODULATED = size_t{10};
  
  auto timeStepsPerCarrierCycle = signal.getTimeStepsPerCarrierCycle(0);

  cycleCount += EXTRA_CYCLES;

  auto capC2 = SeriesRC{circuit};
  auto capC3 = SeriesRC{circuit};
  auto capC4 = SeriesRC{circuit};
  auto capC5 = SeriesRC{circuit};
  const auto capacitor = array<SeriesRC*, 4 >{&capC2, &capC4, &capC5, &capC3};

  // phaseOffset is the fraction of a carrier cycle that the local
  // oscillator starts at. The carrier is ahead of the local
  // oscillator
  auto phaseOffset = timeStepsPerCarrierCycle * phaseAngleDeg / 360.;
  auto johnsonCounter = JohnsonCounter{};
  auto localOscillator = LocalOscillator{4 * signal.getCarrierFreqHz(0),
					 phaseOffset,
					 johnsonCounter};
    
  auto totalTimeSteps = size_t{0};

  for (decltype(cycleCount) cycles = 0; cycles < cycleCount; cycles++) {
    for (decltype(timeStepsPerCarrierCycle) timeStep = 1;
	 timeStep <= timeStepsPerCarrierCycle; timeStep++) {
	
      totalTimeSteps++;

      localOscillator.step();
      // Modulation
      auto amplitude = signal.getAmplitude(0, totalTimeSteps);
      // Modulated signal
      auto signalVoltage = signal.getTotalSignal(totalTimeSteps);

      // Add 2.5 volts (Vcc/2) bias
      signalVoltage += 2.5;
	
      // Johnson counter (IC1A and IC1B) selects which capacitor gets
      // connected to the RF signal.  The other capacitors are
      // electrically isolated during the time step and so do not
      // change their state at all (they are assumed to have no
      // leakage resistance)
      auto enabledChannel = johnsonCounter.get();
      for (auto&& index = size_t{0}; index < johnsonCounter.stateCount();
	   index++) {
	auto* cap = capacitor.at(index);
	if (index == enabledChannel) {
	  cap->applyVoltageForOneTimeStep(signalVoltage);
	}
	else {
	  cap->isolateForOneTimeStep();
	}
      }

      auto dataLine = unique_ptr<DataLine>{new DataLine(INDEX_DEMODULATED + 1,
							totalTimeSteps)};
      dataLine->fields.at(INDEX_SIGNAL) = signalVoltage;
      dataLine->fields.at(INDEX_MODULATION) = amplitude;
      dataLine->fields.at(INDEX_CAPC2_VOLTAGE) = capC2.getVoltage();
      dataLine->fields.at(INDEX_CAPC3_VOLTAGE) = capC3.getVoltage();
      dataLine->fields.at(INDEX_CAPC4_VOLTAGE) = capC4.getVoltage();
      dataLine->fields.at(INDEX_CAPC5_VOLTAGE) = capC5.getVoltage();
      dataLine->fields.at(INDEX_DIFFERENCE_IC2A) =
	capC2.getVoltage() - capC3.getVoltage();
      dataLine->fields.at(INDEX_DIFFERENCE_IC2B) =
	capC4.getVoltage() - capC5.getVoltage();
      add(dataLine);
    }
  }

  butterworth(INDEX_DIFFERENCE_IC2A, INDEX_FILTERED_INPHASE,
	      2, circuit.lpFreqHz, false);

  butterworth(INDEX_DIFFERENCE_IC2B, INDEX_FILTERED_QUADRATURE,
	      2, circuit.lpFreqHz, false);
  
  amDemod(INDEX_FILTERED_INPHASE,
	  INDEX_FILTERED_QUADRATURE,
	  INDEX_DEMODULATED);

  outputData(outputFilename, headings, timeStepsPerCarrierCycle);
}

