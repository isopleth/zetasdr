# zetasdr

C++ simulation of the ZetaSDR radio (http://www.qrz.lt/ly1gp/SDR/), a
simple software defined radio receiver based on a Tayloe quadrature
product detector.  The radio was designed by a Lithuanian radio
amateur, call sign LY1GP, and is popular amongst enthusiasts as a way
of getting started with building SDRs because of the very small part
count.  This program plots out different electronic signals in
detail, to help people understand how it works. My motivation was
that I was running electronics tutorials for a class of students
who were tasked with building their own versions of software defined 
radios loosely based on the ZetaSDR, and so the plots from the
program were both for my benefit and theirs.

The results are written
up in https://github.com/isopleth/zetasdr/blob/master/zetasdr.pdf

## Dependencies

Written for Linux.  It needs:
1. Make
1. g++, with support for C++ 2017
1. Rt filter, a digital filter library for C++
1. Python 3
1. Matplotlib, a plotting library for Python
1. Latex

In addition, the circuit schematic was drawn using gschem, although it is not needed to regenerate the PDF document.

On Ubuntu Linux, these dependencies can be installed using the command:

```sudo apt-get install make g++ librtfilter-dev python3 python3-matplotlib texlive-full geda-gschem```

