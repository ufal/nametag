DIR=../morphodita/doc

all:
	@echo "make [html | txt | pdf | web | manual]"

html: nametag.html
txt: nametag.txt nametag_install.txt
pdf: nametag.pdf
web: web.html web_install.html web_user.html web_api.html web_about.html
manual: nametag.html nametag.txt nametag_install.txt nametag_readme.txt
	cp nametag.txt ../MANUAL
	cp nametag.html ../MANUAL.html
	cp nametag_install.txt ../INSTALL
	cp nametag_readme.txt ../README

nametag.html: nametag.t2t *.t2t
	$(DIR)/txt2tags -t html -C $(DIR)/not_web.conf $<

nametag_%.html: nametag_%.t2t *.t2t
	$(DIR)/txt2tags -t html -C $(DIR)/not_web.conf -C $(DIR)/strip_level.conf -o- $< | sed '/^<H2>=*<\/H2>$$/d' >$@

web.html: nametag.t2t *.t2t
	$(DIR)/txt2tags -t html -C $(DIR)/web.conf -o $@ $<

web_%.html: nametag_%.t2t *.t2t
	$(DIR)/txt2tags -t html -C $(DIR)/web.conf -C $(DIR)/strip_level.conf -o- $< | sed '/^<H2>=*<\/H2>$$/d' >$@

nametag.txt: nametag.t2t *.t2t
	$(DIR)/txt2tags -t txt -C $(DIR)/not_web.conf --no-toc --no-enum-title -o- $< | sed '/^$$/{3d;$$d}' >$@

nametag_%.txt: nametag_%.t2t *.t2t
	$(DIR)/txt2tags -t txt -C $(DIR)/not_web.conf -C $(DIR)/strip_level.conf --no-toc --no-enum-title -o- $< | sed '/^$$/{3d;$$d}' >$@
nametag.tex: nametag.t2t *.t2t
	$(DIR)/txt2tags -t tex -C $(DIR)/not_web.conf $<

nametag.pdf: nametag.tex $(DIR)/*.sty
	TEXINPUTS=.:$(DIR): pdflatex $<
	TEXINPUTS=.:$(DIR): pdflatex $<

.PHONY: clean
clean:
	rm -f *.aux *.dvi *.html *.log *.pdf *.out *.tex *.toc *.txt
