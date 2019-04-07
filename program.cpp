/**
 * Non-Spice simulation of ZetaSDR radio
 * http://www.qrz.lt/ly1gp/SDR
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

#include <array>
#include <cmath>
#include <fstream>
#include <iostream>
#include <list>
#include <memory>
#include <optional>
#include <rtf_common.h>
#include <sstream>
#include <stdexcept>
#include <vector>

// So we can change the precision easily...
using floating = long double;

using namespace std;

constexpr auto LOGIC_ONE_VOLTAGE = floating{2.4};         // Voltage corresponding to logic 1
constexpr auto TIME_STEP_SIZE = floating{10e-12}; // 10 picoseconds time step size
constexpr auto CARRIER_FREQUENCY =  floating{7e6}; // 7 MHz RF carrier frequency
constexpr auto MODULATION_FREQUENCY = floating{2e5}; // 200 kHz amplitude modulation
constexpr auto NO_MODULATION = floating{0};        // Use this where we don't want modulation
constexpr auto CARRIER_AMPLITUDE = floating{1e-3}; // 1 mV
constexpr auto PHASE_ANGLE = floating{35};

constexpr auto EXTRA_CYCLES = 100;  // Extra cycles at the start to get output stable
constexpr auto SPAN = size_t{10}; // Output every n values to output file, to keep size manageable

// Sample and hold capacitors C2-C5
constexpr auto CAPACITANCE_SANDH = floating{0.022e-6}; // 0.022 uF
// Two 74HC4052 channels in parallel at 70 ohms each
constexpr auto RESISTANCE_74HC4052 = floating{35}; // Two 70 ohm channels in parallel

constexpr auto CYCLES = 80;
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
  JohnsonCounter() : state(0) {}

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
    return OUTPUT_VALUE[state];
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
  static constexpr const floating AMPLITUDE = 5.0;

  /**
   * Get voltage level of local oscillator at the current timestep
   *
   * @return the voltage
   */
  auto getVoltage() {
    auto value = floating{sin(2.0 * M_PI * timeStep / timeStepsPerCycle)};
    value = ((value + 1.0) / 2.0) * AMPLITUDE;
    return value;
  }

public:
  /**
   * Constructor.  Set the start state of the counter, and the
   * time steps corresponding to a cycle of the carrier.  The phase
   * offset is handled by retarding the initial value of the current time
   * steps so that it takes the number of time steps corresponding to the
   * phase angle of the carrier before it reaches 0.
   *
   * @param timeStepsPerCarrierCycle
   * @param phaseOffsetRadians
   * @param johnsonCounter Johnson counter object that the oscillator drives
   */
  LocalOscillator(floating timeStepsPerCycle,
		  floating phaseOffsetRadians,
		  JohnsonCounter& johnsonCounter):
    timeStep(static_cast<decltype(timeStep)>(-floor(phaseOffsetRadians))),
    timeStepsPerCycle(floor(timeStepsPerCycle)),
    johnsonCounter(johnsonCounter),
    voltage(getVoltage()) {
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
    if (previousVoltage < LOGIC_ONE_VOLTAGE && voltage >= LOGIC_ONE_VOLTAGE) {
      johnsonCounter.clock();
    }
  }
};
  

/**
 * This represents a sample and hold capacitors on the outputs from
 * the 74HC4052.  It incorporates the resistance through the pair of
 * 74HC4052 channels.
 */
class Capacitor {
private:
  floating capacitance;
  floating resistance;
  floating actualResistance;
  floating voltage;  // voltage currently across capacitor
public:

  /**
   * Constructor
   *
   * @param capacitance capacitance in Farads
   * @param resistance resistance through 74HC4052
   */
  Capacitor(floating capacitance,
	    floating resistance) : capacitance(capacitance),
				   resistance(resistance),
				   voltage{0} {}

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
  auto turnMuxChannelOnForOneTimeStep(floating appliedVoltage) {
    auto voltageDifference = appliedVoltage - voltage;
    voltage +=
      voltageDifference * exp(-TIME_STEP_SIZE/(capacitance * resistance));
    actualResistance = resistance;
  }

  /**
   * Place holder
   */
  auto turnMuxChannelOffForOneTimeStep() {
    actualResistance = numeric_limits<decltype(actualResistance)>::max();
  }
};

/**
 * This class represents the amplitude modulation
 */
class AmplModulation {
private:
  floating modFreqHz; // Modulation frequency

public:
  /**
   * Constructor
   *
   * @param modFreqHz modulation frequency in Hertz
   */
  AmplModulation(floating modFreqHz) : modFreqHz(modFreqHz) {}

  /**
   * Get the current value of the modulated signal
   *
   * @param timeStep get the modulation for this time step
   * @return current modulation value
   */
  auto get(size_t timeStep) -> floating {
    if (!modFreqHz) {
      // No modulation
      return 1;
    }
    else {
      auto radiansPerSecond = floating{2.0 * M_PI * modFreqHz};
      auto radians = radiansPerSecond * timeStep * TIME_STEP_SIZE;
      return cos(radians);
    }
  }
};


/**
 * This is just a vector where we can specify the number of slots to
 * reserve in the constructor
 */
template <class T> class SizedVector : public vector<T> {
public:
  /**
   * Constructor reserves specified number of entries in vector.
   *
   * @param size number of entries to reserve
   */
  SizedVector(size_t size) {
    this->reserve(size);
  }
  
  SizedVector(floating size) {
    this->reserve(static_cast<size_t>(size));
  }
};

class FloatingVector : public SizedVector<floating> {};
class DoubleVector : public SizedVector<double> {};


//===================================================================

/**
 * Make the file stream.  The filename is based on the parameters
 *
 * @param prefix prefix string
 * @param cycleCount number of carrier cycles to simulate
 * @param modFreqHz amplitude modulation frequency
 * @param phaseAngleDeg phase angle of carrier with respect to local oscillator
 */
auto makeFileStream(const string& prefix,
		    floating cycleCount,
		    floating modFreqHz,
		    floating phaseAngleDeg) {
  auto filename = stringstream{};
  filename << prefix << "_" << cycleCount << "_" << modFreqHz;
  filename << "_" << phaseAngleDeg << ".txt";

  cout << "Writing " << filename.str() << endl;
  return ofstream(filename.str());
}

//===================================================================

/**
 * Butterworth low pass digital filter.
 *
 * @param signal signal to pass through low pass filter
 * @param filteredSignal resulting filtered signal
 * @param poles number of poles
 * @param cutoffHz filter cut-off frequency
 */
auto lpFilter(const FloatingVector& signal,
	      FloatingVector& filteredSignal,
	      unsigned poles,
	      double cutoffHz) {

  const auto normalisedCutoffFreq = static_cast<double>
    (cutoffHz * TIME_STEP_SIZE);
  constexpr auto LOW_PASS = 0;

  auto filter = hfilter{rtf_create_butterworth(1,
					       RTF_DOUBLE,
					       normalisedCutoffFreq,
					       poles,
					       LOW_PASS)};
  if (filter == nullptr) {
    cout << "Unable to create low pass filter" << endl;
    exit(EXIT_FAILURE);
  }
  auto size = signal.size();
  auto inputVector = DoubleVector{size};
  auto outputVector = DoubleVector{size};

  for (auto&& value : signal) {
    inputVector.push_back(static_cast<double>(value));
    outputVector.push_back(0);
  }
  rtf_filter(filter, inputVector.data(), outputVector.data(), size);
  rtf_destroy_filter(filter);
  for (auto&& value : outputVector) {
    filteredSignal.push_back(value);
  }
}


/**
 * AM demodulation of the I/Q signal
 *
 * @param inphaseVector I samples vector
 * @param quadratureVector Q samples vector
 * @param demodulatedOutputVector output vector
 */
auto amDemod(const FloatingVector& inphaseVector,
	     const FloatingVector& quadratureVector,
	     FloatingVector& demodulatedOutputVector) {

  auto size = inphaseVector.size();
  demodulatedOutputVector.clear();
  demodulatedOutputVector.reserve(size);
  
  for (decltype(size) index = 0; index < size; index++) {
    auto inphaseValue = inphaseVector[index];
    auto value = inphaseValue * inphaseValue;
    auto quadratureValue = quadratureVector[index];
    value += quadratureValue * quadratureValue;
    value = sqrt(value);
    // Need to make sure that square root value is given correct sign
    if (inphaseValue + quadratureValue < 0) {
      value = -value;
    }
    demodulatedOutputVector.push_back(value);
  }
}


//===================================================================

/**
 * This simulates the Tayloe quadrature product detector.  It outputs
 * the results into a data file. The phase angle is the phase of the
 * initial state of the carrier with respect to the initial state of
 * the local oscillator.
 *
 * @param cycleCount number of carrier cycles to simulate
 * @param carrierAmplitude carrier amplitude in volts
 * @param carrierFreqHz carrier frequency
 * @param modFreqHz Amplitude modulation frequency
 * @param phaseAngleDeg Initial phase angle of carrier compared to local oscillator
 * @param lpPassFreqHz cut off frequency of the low filter
 */
auto tayloe(size_t cycleCount,
	    floating carrierAmplitude,
	    floating carrierFreqHz,
	    floating modFreqHz = 0,
	    floating phaseAngleDeg = 0,
	    floating lpPassFreqHz = 0) {

  auto file = makeFileStream("tayloe", cycleCount, modFreqHz, phaseAngleDeg);
  auto timeStepsPerCarrierCycle = 1.0/(TIME_STEP_SIZE * carrierFreqHz);

  cycleCount += EXTRA_CYCLES;

  auto ampModulation = AmplModulation{modFreqHz};

  auto capC2 = Capacitor{CAPACITANCE_SANDH, RESISTANCE_74HC4052};
  auto capC3 = Capacitor{CAPACITANCE_SANDH, RESISTANCE_74HC4052};
  auto capC4 = Capacitor{CAPACITANCE_SANDH, RESISTANCE_74HC4052};
  auto capC5 = Capacitor{CAPACITANCE_SANDH, RESISTANCE_74HC4052};
  Capacitor *capacitor[] = {&capC2, &capC4, &capC5, &capC3};

  // phaseOffset is the fraction of a carrier cycle that the
  // local oscillator starts at. The carrier is ahead of the local
  // oscillator
  auto phaseOffset = timeStepsPerCarrierCycle * phaseAngleDeg / 360.;
  auto johnsonCounter = JohnsonCounter{};
  auto localOscillator = LocalOscillator{timeStepsPerCarrierCycle / 4,
					 phaseOffset,
					 johnsonCounter};

  // Accumulate the results rather than print them out immediately
  // so we can add a filter that processes a set of values if we want
  auto size = cycleCount * floor(timeStepsPerCarrierCycle);

  auto signalVector = FloatingVector{size};
  auto modulationVector = FloatingVector{size};
  auto time = FloatingVector{size};
  auto capC2voltageVector = FloatingVector{size};
  auto capC3voltageVector = FloatingVector{size};
  auto capC4voltageVector = FloatingVector{size};
  auto capC5voltageVector = FloatingVector{size};
  auto differenceIC2Avector = FloatingVector{size};
  auto differenceIC2Bvector = FloatingVector{size};
  auto filteredInphaseVector = FloatingVector{size};
  auto filteredQuadratureVector = FloatingVector{size};
  auto demodulatedOutputVector = FloatingVector{size};
  
  auto totaltimeSteps = size_t{0};
  
  for (decltype(cycleCount) cycles = 0; cycles < cycleCount; cycles++) {
    for (decltype(timeStepsPerCarrierCycle) timeStep = 1;
	 timeStep <= timeStepsPerCarrierCycle; timeStep++) {

      totaltimeSteps++;
      auto fractionThroughCycle = floating{timeStep/timeStepsPerCarrierCycle};
      localOscillator.step();
      auto carrierAngle = floating{(cycles + fractionThroughCycle) * 2.0 * M_PI};
      auto modulation = ampModulation.get(totaltimeSteps);
      auto signal = floating{modulation * carrierAmplitude * sin(carrierAngle)};

      // Add 2.5 volts (Vcc/2) bias
      signal += 2.5;

      // Johnson counter (IC1A and IC1B) selects which capacitor gets
      // connected to the RF signal.  The other capacitors are electrically
      // isolated during the time step and so do not change their state at
      // all (they are assumed to have no leakage resistance)
      auto enabledChannel = johnsonCounter.get();
      for (auto index = size_t{0}; index < johnsonCounter.stateCount();
	   index++) {
	auto* cap = capacitor[index];
	if (index == enabledChannel) {
	  cap->turnMuxChannelOnForOneTimeStep(signal);
	}
	else {
	  cap->turnMuxChannelOffForOneTimeStep();
	}
      }

      time.push_back(totaltimeSteps * TIME_STEP_SIZE);
      signalVector.push_back(signal);
      modulationVector.push_back(modulation);
      capC2voltageVector.push_back(capC2.getVoltage());
      capC3voltageVector.push_back(capC3.getVoltage());
      capC4voltageVector.push_back(capC4.getVoltage());
      capC5voltageVector.push_back(capC5.getVoltage());
      
      differenceIC2Avector.push_back(capC2.getVoltage()
				     - capC3.getVoltage());
      differenceIC2Bvector.push_back(capC4.getVoltage()
				     - capC5.getVoltage());
    }
  }

  if (lpPassFreqHz) {
    lpFilter(differenceIC2Avector, filteredInphaseVector, 2, lpPassFreqHz);
    lpFilter(differenceIC2Bvector, filteredQuadratureVector, 2, lpPassFreqHz);
  }
  else {
    filteredInphaseVector = differenceIC2Avector;
    filteredQuadratureVector = differenceIC2Bvector;
  }
  
  amDemod(filteredInphaseVector,
	  filteredQuadratureVector,
	  demodulatedOutputVector);
  
  auto counter = size_t{0};
  const auto initialIndex = static_cast<FloatingVector::size_type>
    (EXTRA_CYCLES*timeStepsPerCarrierCycle);
  for (auto index = initialIndex; index < signalVector.size(); index++) {
    if (counter++ % SPAN == 0) {
      file << time[index] << ", " // 0
	   << signalVector[index] << ", "        // 1
	   << modulationVector[index] << ","     // 2
	   << capC2voltageVector[index] << ", "  // 3
	   << capC3voltageVector[index] << ", "  // 4
	   << capC4voltageVector[index] << ", "  // 5
	   << capC5voltageVector[index] << ", "  // 6
	   << differenceIC2Avector[index] << ", "// 7
	   << differenceIC2Bvector[index] << "," // 8
	   << filteredInphaseVector[index] << ", " // 9
	   << filteredQuadratureVector[index] << "," // 10
	   << demodulatedOutputVector[index] // 11
	   << endl;
    }
  }

}

/**
 * This simulates a conventional I/Q mixer.  It outputs the results
 * into a CSV data file.
 *
 * @param cycleCount number of carrier cycles to simulate
 * @param carrierAmplitude carrier amplitude in volts
 * @param carrierFreqHz carrier frequency
 * @param modFreqHz Amplitude modulation frequency
 * @param phaseAngleDeg Initial phase angle of carrier compared to local oscillator
 */
auto iqMixer(size_t cycleCount,
	     floating carrierAmplitude,
	     floating carrierFreqHz,
	     floating modFreqHz = 0,
	     floating phaseAngleDeg = 0,
	     floating lpPassFreqHz = 0) {

  auto file = makeFileStream("iq", cycleCount, modFreqHz, phaseAngleDeg);
  // Add a few extra cycles to let the capacitors stabilise
  cycleCount += EXTRA_CYCLES;

  // Phase angle in radians
  const auto phaseAngle = phaseAngleDeg * M_PI / 180.0;
  auto timeStepsPerCarrierCycle = 1/(TIME_STEP_SIZE * carrierFreqHz);
  
  // Accumulate the results rather than print them out immediately
  // so we can add a filter that processes a set of values if we want
  auto size = cycleCount * floor(timeStepsPerCarrierCycle);
  auto time = FloatingVector{size};
  auto signalVector = FloatingVector{size};
  auto localOscAngleVector = FloatingVector{size};
  auto modulationVector = FloatingVector{size};
  auto inphaseVector = FloatingVector{size};
  auto quadratureVector = FloatingVector{size};
  auto filteredInphaseVector = FloatingVector{size};
  auto filteredQuadratureVector = FloatingVector{size};
  auto demodulatedOutputVector = FloatingVector{size};

  auto ampModulation = AmplModulation{modFreqHz};

  auto totaltimeSteps = size_t{0};
  for (decltype(cycleCount) cycles = 0; cycles < cycleCount; cycles++) {
    for (decltype(timeStepsPerCarrierCycle) timeStep = 1;
	 timeStep <= timeStepsPerCarrierCycle; timeStep++) {

      totaltimeSteps++;
      auto fractionThroughCycle = floating{timeStep/timeStepsPerCarrierCycle};
      auto carrierAngle = floating{(cycles + fractionThroughCycle) * 2.0 * M_PI};
      auto modulation = ampModulation.get(totaltimeSteps);
      auto signal = modulation * carrierAmplitude * sin(carrierAngle);

      // Local oscillator is phaseAngle behind carrier
      auto localOscAngle = carrierAngle - phaseAngle;
      auto inphase = signal * sin(localOscAngle);
      auto quadrature = signal * cos(localOscAngle);


      time.push_back(totaltimeSteps * TIME_STEP_SIZE);
      signalVector.push_back(signal);
      localOscAngleVector.push_back(localOscAngle);
      modulationVector.push_back(modulation);
      inphaseVector.push_back(inphase);
      quadratureVector.push_back(quadrature);
    }
  }

  if (lpPassFreqHz) {
    lpFilter(inphaseVector, filteredInphaseVector, 4, lpPassFreqHz);
    lpFilter(quadratureVector, filteredQuadratureVector, 4, lpPassFreqHz);
  }
  else {
    filteredInphaseVector = inphaseVector;
    filteredQuadratureVector = quadratureVector;
  }

  amDemod(filteredInphaseVector,
	  filteredQuadratureVector,
	  demodulatedOutputVector);

  auto counter = size_t{0};
  const auto initialIndex = static_cast<FloatingVector::size_type>
    (EXTRA_CYCLES*timeStepsPerCarrierCycle);
  for (auto index = size_t{initialIndex};
       index < signalVector.size(); index++) {
    if (counter++ % SPAN == 0) {
      file << time[index] << ", "                    // 0
	   << signalVector[index] << ", "            // 1
	   << localOscAngleVector[index] << ", "     // 2
	   << modulationVector[index] << ", "        // 3
	   << inphaseVector[index] << ", "           // 4
	   << quadratureVector[index] << ", "        // 5
	   << filteredInphaseVector[index] << ", "   // 6
	   << filteredQuadratureVector[index] << "," // 7
	   << demodulatedOutputVector[index] << endl;
    }
  }
}

//===================================================================

auto main() -> int {
  /*
   * Unmodulated carrier, in phase with local oscillator
   */
  tayloe(4, CARRIER_AMPLITUDE, CARRIER_FREQUENCY, NO_MODULATION);

  /*
   * Unmodulated carrier, with 35 degree phase difference in start state
   * compared to local oscillator
   */
  tayloe(4, CARRIER_AMPLITUDE, CARRIER_FREQUENCY, NO_MODULATION,
	 PHASE_ANGLE);

  /*
   * Modulated carrier, in phase with local oscillator
   */
  tayloe(CYCLES, CARRIER_AMPLITUDE, CARRIER_FREQUENCY, MODULATION_FREQUENCY);

  /*
   * Modulated carrier with 35 degree phase difference in initial
   * state compared to local oscillator
   */
  tayloe(CYCLES, CARRIER_AMPLITUDE, CARRIER_FREQUENCY, MODULATION_FREQUENCY,
	 PHASE_ANGLE, MODULATION_FREQUENCY * 2);

  /*
   * Ideal multiplying IQ mixer. Set the cut off frequency to the
   * carrier frequency because we will get a demodulated signal and a
   * signal at 2 x the carrier frequency.
   */
  iqMixer(CYCLES, CARRIER_AMPLITUDE, CARRIER_FREQUENCY, MODULATION_FREQUENCY,
	  0, CARRIER_FREQUENCY/2);

  /*
   * Ideal multiplying IQ mixer with 35 degree phase difference
   */
  iqMixer(CYCLES, CARRIER_AMPLITUDE, CARRIER_FREQUENCY, MODULATION_FREQUENCY,
	  PHASE_ANGLE, CARRIER_FREQUENCY/2);
}

