#!/usr/bin/python3

#
# Plot results
#
#  Copyright 2019  Jason Leake
#
# Permission is hereby granted, free of charge, to any person
# obtaining a copy of this software and associated documentation
# files (the "Software"), to deal in the Software without
# restriction, including without limitation the rights to use, copy,
# modify, merge, publish, distribute, sublicense, and/or sell copies
# of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be
# included in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
# NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
# BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
# ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
# CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#

import matplotlib.pyplot as plt
import csv

# Set this True to add titles to the plots
ENABLE_TITLE = False

# Indexes into Tayloe signal result CSV files
T_TIME = 0
T_SIGNAL = 1
T_MODULATION = 2
T_CAP_C2 = 3
T_CAP_C3 = 4
T_CAP_C4 = 5
T_CAP_C5 = 6
T_IC2A_IN =7 
T_IC2B_IN = 8
T_I_LOW_PASS = 9
T_Q_LOW_PASS = 10
T_AM_DEMOD = 11

# Indexes into the IQ mixer CSV files
IQ_TIME = 0
IQ_SIGNAL = 1
IQ_LOCAL_OSC_ANGLE = 2
IQ_MODULATION = 3
IQ_I = 4
IQ_Q = 5
IQ_I_LOW_PASS = 6
IQ_Q_LOW_PASS = 7
IQ_AM_DEMOD = 8

counter = 0

##
# c2c3Voltage.png and c4c5Voltage.png
#
# @param input input filename
# @param filename output filename
# @param title plot title
# @param cap1 capacitor 1 CSV column
# @param cap2 capacitor 2 CSV column
# @param ic IC CSV column, i.e. capacitor voltages difference column
# @param cap1label capacitor 1 plot label
# @param cap2label capacitor 2 plot label
# @param iclabel IC label
# @param threePlots True if three plots to be produced, False for two
#
def tayloeVoltage(input, filename, title, cap1, cap2,
                  ic, cap1label, cap2label, iclabel,
                  threePlots):
    print("Writing " + filename)
    global counter
    time = []
    rfSignal = []
    cap1Voltage = []
    cap2Voltage = []
    capsVoltage = []
    with open(input, 'rt') as csvfile:
        csvreader = csv.reader(csvfile, delimiter=',', quotechar='"')
        for row in csvreader:
            time.append(float(row[T_TIME])*1e6)
            rfSignal.append(float(row[T_SIGNAL]))
            cap1Voltage.append(float(row[cap1]))
            cap2Voltage.append(float(row[cap2]))
            capsVoltage.append(float(row[ic]))
    
    plt.figure(num=counter, figsize=(10, 8))
    counter = counter + 1

    if ENABLE_TITLE:
        if len(title):
            plt.suptitle(title)
    numberOfPlots = 2
    if threePlots:
        plt.subplot(3, 1, 1)
        plt.plot(time, rfSignal, "r-", label="RF signal")
        plt.tick_params(axis='x', which='both', bottom=False,
                        top=False, labelbottom=False)
        plt.ylabel('volts')
        plt.legend(loc="center right")
        numberOfPlots = 3
    
    plt.subplot(numberOfPlots, 1, numberOfPlots - 1)
    if not threePlots:
        plt.plot(time, rfSignal, "r-", label="RF signal")
    plt.plot(time, cap1Voltage, "y-", label = cap1label)
    plt.plot(time, cap2Voltage, "b-", label = cap2label)
    plt.tick_params(axis='x', which='both', bottom=False,
                    top=False, labelbottom=False)
    plt.ylabel('volts')
    plt.legend(loc="center right")

    plt.subplot(numberOfPlots, 1, numberOfPlots)
    plt.plot(time, capsVoltage, "g-", label = iclabel)
    plt.ylabel('volts')
    plt.xlabel('microseconds')
    plt.legend(loc="center right")
    
    plt.savefig(filename)

##
# Ideal multiplying IQ mixer
#
# @param input input filename
# @param filename output filename
# @param threePlots True if three plots to be produced, False for two 
#
def iqVoltage(input, filename, title, threePlots):
    print("Writing " + filename)
    global counter
    time = []
    rfSignal = []
    i = []
    q = []
    with open(input, 'rt') as csvfile:
        csvreader = csv.reader(csvfile, delimiter=',', quotechar='"')
        for row in csvreader:
            time.append(float(row[IQ_TIME])*1e6)
            rfSignal.append(float(row[IQ_SIGNAL]))
            i.append(float(row[IQ_I]))
            q.append(float(row[IQ_Q]))

    plt.figure(num=counter, figsize=(10, 8))
    counter = counter + 1

    if ENABLE_TITLE:
        if len(title):
            plt.suptitle(title)

    numberOfPlots = 2

    if threePlots:
        plt.subplot(3, 1, 1)
        numberOfPlots = 3
    else:
        plt.subplot(2, 1, 1)
        
    plt.plot(time, rfSignal, "r-", label="RF signal")
    plt.tick_params(axis='x', which='both', bottom=False,
                    top=False, labelbottom=False)
    plt.ylabel('volts')
    plt.legend(loc="center right")

    plt.subplot(numberOfPlots, 1, numberOfPlots - 1)
    plt.plot(time, i, "y-", label = "$I$")
    plt.ylabel('volts')
    plt.legend(loc="center right")
    if not threePlots:
        plt.plot(time, q, "b-", label = "$Q$")
        plt.xlabel('microseconds')
        plt.legend(loc="center right")
    else:
        plt.subplot(3, 1, 3)
        plt.plot(time, q, "b-", label = "$Q$")
        plt.ylabel('volts')
        plt.xlabel('microseconds')
        plt.legend(loc="center right")

    plt.savefig(filename)

##
# Ideal multiplying IQ mixer
#
# @param input input filename
# @param filename output filename
# @param title Plot title
#
def iqLowPass(input, filename, title):
    print("Writing " + filename)
    global counter
    time = []
    rfSignal = []
    modulation = []
    i = []
    q = []
    iLowPass = []
    qLowPass = []
    demod = []
    with open(input, 'rt') as csvfile:
        csvreader = csv.reader(csvfile, delimiter=',', quotechar='"')
        for row in csvreader:
            time.append(float(row[IQ_TIME])*1e6)
            rfSignal.append(float(row[IQ_SIGNAL]))
            modulation.append(float(row[IQ_MODULATION]))
            i.append(float(row[IQ_I]))
            q.append(float(row[IQ_Q]))
            iLowPass.append(float(row[IQ_I_LOW_PASS]))
            qLowPass.append(float(row[IQ_Q_LOW_PASS]))
            demod.append(float(row[IQ_AM_DEMOD]))

    plt.figure(num=counter, figsize=(10, 8))
    counter = counter + 1

    if ENABLE_TITLE:
        if len(title):
            plt.suptitle(title)

    plt.subplot(5, 1, 1)
    plt.plot(time, modulation, "c-", label="Modulation")
    plt.tick_params(axis='x', which='both', bottom=False,
                    top=False, labelbottom=False)
    plt.ylabel('volts')
    plt.legend(loc="center right")

    plt.subplot(5, 1, 2)
    plt.plot(time, rfSignal, "r-", label="RF signal")
    plt.tick_params(axis='x', which='both', bottom=False,
                    top=False, labelbottom=False)
    plt.ylabel('volts')
    plt.legend(loc="center right")

    plt.subplot(5, 1, 3)
    plt.plot(time, i, "y-", label = "$I$")
    plt.plot(time, q, "b-", label = "$Q$")
    plt.tick_params(axis='x', which='both', bottom=False,
                    top=False, labelbottom=False)
    plt.ylabel('volts')
    plt.legend(loc="center right")

    plt.subplot(5, 1, 4)
    plt.plot(time, iLowPass, "y-", label = "low pass$(I)$")
    plt.plot(time, qLowPass, "b-", label = "low pass$(Q)$")
    # Plot zero axis as it makes I^2 + q^2 clearer
    plt.axhline(0, color='black', lw=1)
    plt.tick_params(axis='x', which='both', bottom=False,
                    top=False, labelbottom=False)
    plt.ylabel('volts')
    plt.legend(loc="center right")

    plt.subplot(5, 1, 5)
    plt.plot(time, demod, "c-", label = "sqrt($I^2 + Q^2$)")
    plt.ylabel('volts')
    plt.xlabel('microseconds')
    plt.legend(loc="center right")

    plt.savefig(filename)

##
# Tayloe with demodulation
#
# @param input input filename
# @param filename output filename
# @param title Plot title
#
def tayloeLowPass(input, filename, title):
    print("Writing " + filename)
    global counter
    time = []
    rfSignal = []
    modulation = []
    i = []
    q = []
    iLowPass = []
    qLowPass = []
    demod = []
    with open(input, 'rt') as csvfile:
        csvreader = csv.reader(csvfile, delimiter=',', quotechar='"')
        for row in csvreader:            
            time.append(float(row[T_TIME])*1e6)
            rfSignal.append(float(row[T_SIGNAL]))
            modulation.append(float(row[T_MODULATION]))
            i.append(float(row[T_IC2A_IN]))
            q.append(float(row[T_IC2B_IN]))
            iLowPass.append(float(row[T_I_LOW_PASS]))
            qLowPass.append(float(row[T_Q_LOW_PASS]))
            demod.append(float(row[T_AM_DEMOD]))

    plt.figure(num=counter, figsize=(10, 8))
    counter = counter + 1

    if ENABLE_TITLE:
        if len(title):
            plt.suptitle(title)

    plt.subplot(5, 1, 1)
    plt.plot(time, modulation, "c-", label="Modulation")
    plt.tick_params(axis='x', which='both', bottom=False,
                    top=False, labelbottom=False)
    plt.ylabel('volts')
    plt.legend(loc="center right")
    
    plt.subplot(5, 1, 2)
    plt.plot(time, rfSignal, "r-", label="RF signal")
    plt.tick_params(axis='x', which='both', bottom=False,
                    top=False, labelbottom=False)
    plt.ylabel('volts')
    plt.legend(loc="center right")

    plt.subplot(5, 1, 3)
    plt.plot(time, i, "y-", label = "$I$")
    plt.plot(time, q, "b-", label = "$Q$")
    plt.tick_params(axis='x', which='both', bottom=False,
                    top=False, labelbottom=False)
    plt.ylabel('volts')
    plt.legend(loc="center right")

    plt.subplot(5, 1, 4)
    plt.plot(time, iLowPass, "y-", label = "low pass$(I)$")
    plt.plot(time, qLowPass, "b-", label = "low pass$(Q)$")
    # Plot zero axis as it makes I^2 + q^2 clearer
    plt.axhline(0, color='black', lw=1)
    plt.tick_params(axis='x', which='both', bottom=False,
                    top=False, labelbottom=False)
    plt.ylabel('volts')
    plt.legend(loc="center right")

    plt.subplot(5, 1, 5)
    plt.plot(time, demod, "c-", label = "sqrt($I^2 + Q^2$)")
    plt.ylabel('volts')
    plt.xlabel('microseconds')
    plt.legend(loc="center right")

    plt.savefig(filename)

# Plot C2 & C3 voltage with no modulation and no phase shift between
# local oscillator and carrier
def c2c3Voltage():
    tayloeVoltage("tayloe_4_0_0.txt",
                  "c2c3Voltage.png",
                  "Tayloe, no modulation",
                  T_CAP_C2, T_CAP_C3,
                  T_IC2A_IN, "C2", "C3", "C2 - C3", False)
    
# Plot C2 & C3 voltage with no modulation and 35 degree phase shift between
# local oscillator and carrier.  As the local oscillator is running at 4x
# the carrier, the phase shift is actually the difference between the initial
# phase or the local oscillator and the carrier when the program starts
def c2c3PhaseVoltage():
    tayloeVoltage("tayloe_4_0_35.txt",
                  "c2c3VoltagePhase.png",
                  "35 degree phase angle",
                  T_CAP_C2, T_CAP_C3,
                  T_IC2A_IN, "C2", "C3", "C2 - C3", False)
    
def c4c5PhaseVoltage():
    tayloeVoltage("tayloe_4_0_35.txt",
                  "c4c5VoltagePhase.png",
                  "35 degree phase angle",
                  T_CAP_C4, T_CAP_C5,
                  T_IC2B_IN, "C4", "C5", "C4 - C5", False)
    
# Plot C4 & C5 voltage with no modulation and no phase shift between
# local oscillator and carrier
def c4c5Voltage():
    tayloeVoltage("tayloe_4_0_0.txt",
                  "c4c5Voltage.png",
                  "Tayloe, no modulation",
                  T_CAP_C4, T_CAP_C5,
                  T_IC2B_IN, "C4", "C5", "C4 - C5", False)
    
# Plot C2 & C3 voltage with modulation of the carrier but no phase difference
# between carrier and local oscillator
def c2c3ModVoltage():
    tayloeVoltage("tayloe_80_200000_0.txt",
                  "c2c3ModVoltage.png",
                  "Tayloe, 7 MHz, 200 kHz modulation",
                  T_CAP_C2, T_CAP_C3,
                  T_IC2A_IN, "C2", "C3", "C2 - C3", True)

# Plot C4 & C5 voltage with modulation of the carrier but no phase difference
# between carrier and local oscillator
def c4c5ModVoltage():
    tayloeVoltage("tayloe_80_200000_0.txt",
                  "c4c5ModVoltage.png",
                  "Tayloe, 7 MHz, 200 kHz modulation",
                  T_CAP_C4, T_CAP_C5,
                  T_IC2B_IN, "C4", "C5", "C4 - C5", True)

# Plot C2 & C3 voltage with modulation of the carrier and 35 degree phase
# difference between initial state of carrier and local oscillator
def c2c3ModPhaseVoltage():
    tayloeVoltage("tayloe_80_200000_35.txt",
                  "c2c3ModVoltagePhase.png",
                  "Tayloe, 7 MHz, 200 kHz modulation, 35 degrees phase",
                  T_CAP_C2, T_CAP_C3,
                  T_IC2A_IN, "C2", "C3", "C2 - C3", True)

# Plot C4 & C5 voltage with modulation of the carrier and 35 degree phase
# difference between initial state of carrier and local oscillator
def c4c5ModPhaseVoltage():
    tayloeVoltage("tayloe_80_200000_35.txt",
                  "c4c5ModVoltagePhase.png",
                  "Tayloe, 7 MHz, 200 kHz modulation, 35 degrees phase angle",
                  T_CAP_C4, T_CAP_C5,
                  T_IC2B_IN, "C4", "C5", "C4 - C5", True)

def tayloeModPhaseVoltage():
    tayloeLowPass("tayloe_80_200000_35.txt", "tayloeLowPassPhase.png",
                  "Tayloe, 7 MHz, 200 kHz modulation, 35 degrees phase angle")
    
def iqModVoltage():
    iqVoltage("iq_80_200000_0.txt", "iqModVoltage.png",
              "I/Q, 7 MHz, 200 kHz modulation",
              True)
        
def iqModPhaseVoltage():
    iqVoltage("iq_80_200000_35.txt", "iqModVoltagePhase.png",
              "I/Q, 7 MHz, 200 kHz modulation, 35 degrees phase angle",
              True)

def iqModLowPass():
    iqLowPass("iq_80_200000_0.txt", "iqModLowPass.png",
              "I/Q, 7 MHz, 200 kHz modulation")

def iqModPhaseLowPass():
    iqLowPass("iq_80_200000_35.txt", "iqModLowPassPhase.png",
              "I/Q, 7 MHz, 200 kHz modulation, 35 degrees phase angle")

def main():
    # Tayloe
    c2c3Voltage()
    c4c5Voltage()
    c2c3PhaseVoltage()
    c4c5PhaseVoltage()
    c2c3ModVoltage()
    c4c5ModVoltage()
    c2c3ModPhaseVoltage()
    c4c5ModPhaseVoltage()
    tayloeModPhaseVoltage()

    # Multiplying IQ mixer
    iqModVoltage()
    iqModPhaseVoltage()
    iqModLowPass();
    iqModPhaseLowPass();
    
if __name__ == '__main__':
    main()
    
