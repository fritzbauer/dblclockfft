# A Double-Clocked FFT Core Generator

The Double Clocked FFT project contains all of the software necessary to
create the IP to generate an arbitrary sized FFT that will clock two samples
in at each clock cycle, and after some pipeline delay it will clock two
samples out at every clock cycle.

The FFT generated by this approach is very configurable.  By simple adjustment
of a command line parameter, the FFT may be made to be a forward FFT or an
inverse FFT.  The number of bits processed, kept, and maintained by this
FFT are also configurable.  Even the number of bits used for the twiddle
factors, or whether or not to bit reverse the outputs, are all configurable
parts to this FFT core.

These features make the Double Clocked FFT very different and unique among the
other open HDL cores you may fine.

For those who wish to get started right away, please download the package,
change into the ``sw`` directory and run ``make``.  There is no need to
run a configure script, ``fftgen`` is completely portable C++.  Then, once
built, go ahead and run ``fftgen`` without any arguments.  This will cause
``fftgen`` to print a usage statement to the screen.  Review the usage
statement, and run ``fftgen`` a second time with the arguments you need.

Alternatively, you _could_ read the specification.

## Genesis
This FFT comes from my attempts to design and implement a signal processing
algorithm inside a generic FPGA, but only on a limited budget.  As such,
I don't yet have the FPGA board I wish to place this algorithm onto, neither
do I have any expensive modeling or simulation capabilities.  I'm using
Verilator for my modeling and simulation needs.  This makes
using a vendor supplied IP core, such as an FFT, difficult if not impossible
to use.

My problem was made worse when I learned that the published maximum clock
speed for a device wasn't necessarily the maximum clock speed that I could
achieve.  My design needed to process the incoming signal at 500 MHz to be
commercially viable.  500 MHz is not necessarily a clock speed
that can be easily achieved.  250 MHz, on the other hand, is much more within
the realm of possibility.  Achieving a 500 MHz performance with a 250 MHz
clock, however, requires an FFT that accepts two samples per clock.

This, then, was and is the genesis of this project.

# Commercial Applications

Should you find the GPLv3 license insufficient for your needs, other licenses
can be purchased from Gisselquist Technology, LLC.

Likewise, please contact us should you wish to fund the further development
of this core.
