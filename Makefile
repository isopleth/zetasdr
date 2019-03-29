
.PHONY: plots release clean cleanjunk

# Only list one of each type of tile, to prevent the generation programs
# from being run multiple times, as they create multiple targets by
# running once. This is a bit of a bodge.
CSV_FILES = tayloe_4_0_0.txt
PLOTS = c2c3ModVoltagePhase.png

# Default target. Make the plots, but not the pdf
$(PLOTS): program $(CSV_FILES) plot.py
	python3 plot.py

# Make everything, including the pdf, from scratch and then get rid of the junk
release: clean zetasdr.pdf cleanjunk
	rm -f program

# Delete everything except source fies
clean:
	rm -f *~ *.bak *.txt *.png *.log *.aux \#* debug program zetasdr.pdf

# Get rid of anything that isn't a source file
cleanjunk:
	rm -f *~ *.bak *.txt *.png *.log *.aux \#* debug

$(CSV_FILES): program
	./program

# Normal version of the program
program: program.cpp
	g++ --std=c++17 -Wall $^ -o $@ -lrtfilter

# Debug version of the program to be used with gdb
debug: program.cpp
	g++ --std=c++17 -g -Wall $^ -o $@ -lrtfilter

# Latex document.  Run several times to do bibliography
zetasdr.pdf: zetasdr.tex $(PLOTS)
	pdflatex zetasdr.tex
	pdflatex zetasdr.tex
	pdflatex zetasdr.tex



