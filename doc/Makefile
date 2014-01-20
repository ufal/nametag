DIR=../morphodita/doc

all:
	@echo "make [html | txt | pdf | web | manual]"

html: nametag.html
txt: nametag.txt nametag_install.txt
pdf: nametag.pdf

nametag.html: nametag.t2t *.t2t
	$(DIR)/txt2tags -t html -C $(DIR)/not_web.conf $<

nametag.txt: nametag.t2t *.t2t
	$(DIR)/txt2tags -t txt -C $(DIR)/not_web.conf --no-toc --no-enum-title $<

nametag_install.txt: nametag_install.t2t
	$(DIR)/txt2tags -t txt -C $(DIR)/not_web.conf -C $(DIR)/strip_level.conf --no-toc --no-enum-title $<

nametag.tex: nametag.t2t *.t2t
	$(DIR)/txt2tags -t tex -C $(DIR)/not_web.conf $<

nametag.pdf: nametag.tex $(DIR)/*.sty
	TEXINPUTS=.:$(DIR): pdflatex $<
	TEXINPUTS=.:$(DIR): pdflatex $<

web: nametag.t2t nametag_install.t2t nametag_user.t2t nametag_api.t2t
	$(DIR)/txt2tags -t html -C $(DIR)/web.conf -o web.html nametag.t2t
	$(DIR)/txt2tags -t html -C $(DIR)/web.conf -C $(DIR)/strip_level.conf -o web_install.html nametag_install.t2t
	$(DIR)/txt2tags -t html -C $(DIR)/web.conf -C $(DIR)/strip_level.conf -o web_users_guide.html nametag_user.t2t
	$(DIR)/txt2tags -t html -C $(DIR)/web.conf -C $(DIR)/strip_level.conf -o web_api.html nametag_api.t2t

manual: html txt
	cp nametag.txt ../MANUAL
	cp nametag.html ../MANUAL.html
	cp nametag_install.txt ../INSTALL

.PHONY: clean
clean:
	rm -f *.aux *.dvi *.html *.log *.pdf *.out *.tex *.toc *.txt
