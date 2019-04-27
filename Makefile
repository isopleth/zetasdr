
.PHONY: plots release clean cleanjunk

DEPDIR := dep
$(shell mkdir -p $(DEPDIR))
DEPFLAGS = -MMD -MP -MF $(DEPDIR)/$*.Td

# Only list one of each type of tile, to prevent the generation programs
# from being run multiple times, as they create multiple targets by
# running once. This is a bit of a bodge.
CSV_FILES = zetasdr_unmodulated_0.txt
PLOTS = c2c3ModVoltagePhase.png

# Default target. Make the plots, but not the pdf
$(PLOTS): program $(CSV_FILES) plot.py
	python3 plot.py

# Make everything, including the pdf, from scratch and then get rid of the junk
release: $(PLOTS) zetasdr.pdf cleanjunk
	rm -f program

# Delete everything except source fies
clean:
	rm -f *~ *.bak *.txt *.png *.log *.aux \#*
	rm -f debug program  *.o zetasdr.pdf *-converted-to.pdf
	rm -rf dep/

# Get rid of anything that isn't a source file
cleanjunk:
	rm -f *~ *.bak *.txt *.png *.log *.aux \#* debug *.o

$(CSV_FILES): program
	./program

program: program.o IqMixer.o Mixer.o Signal.o ZetaSdr.o
	g++ --std=c++17 -g -Wall $^ -o $@ -lrtfilter

%.o: %.cpp
%.o: %.cpp $(DEPDIR)/%.d
	g++ --std=c++17 -c -g $(DEPFLAGS) -Wall $<
	mv -f $(DEPDIR)/$*.Td $(DEPDIR)/$*.d && touch $@

# Latex document.  Run several times to do bibliography
zetasdr.pdf: zetasdr.tex $(PLOTS) program
	pdflatex zetasdr.tex
	pdflatex zetasdr.tex
	pdflatex zetasdr.tex

$(DEPDIR)/%.d:
	-mv -f $(DEPDIR)/$*.Td $(DEPDIR)/$*.d && touch $@

.PRECIOUS: $(DEPDIR)/%.d

include $(wildcard $(patsubst %,$(DEPDIR)/%.d,$(basename $(SRCS))))

