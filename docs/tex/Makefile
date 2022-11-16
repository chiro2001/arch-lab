PAPER    = main
TEMPLATE = archreport
PDF      = $(PAPER).pdf
REQUIRE  = $(PAPER).tex $(TEMPLATE).sty
TEMP     = *.aux sections/*.aux *.log *.out *.toc *.xdv *.bbl *.blg *.bcf *.synctex.gz *.run.xml *.markdown.* *_markdown_* dist/ *.tmp

LATEX    = xelatex
BIBER    = biber
NOPDF    = --no-pdf
ARGS     = --shell-escape

ifndef LAB
	LAB := lab1
endif

all: $(PDF)

images:
	cp -R ../lab*/*.assets .

$(PDF): $(REQUIRE) images FORCE
	$(LATEX) $(ARGS) $(PAPER).tex
	cp $(PDF) "../$(LAB)/$(TITLE).pdf"

clean:
	-rm -rf $(TEMP)

FORCE:
.PHONY: all clean images FORCE