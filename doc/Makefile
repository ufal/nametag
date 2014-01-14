DIR=../morphodita/doc

all:
	@echo "make [html | pdf | txt | web | manual]"

html pdf tex txt:%:nametag.%

nametag.html nametag.tex nametag.txt: nametag.%: nametag.t2t *.t2t
	$(DIR)/txt2tags -t $* -C $(DIR)/not_web.conf $<

nametag.pdf: nametag.tex
	TEXINPUTS=.:$(DIR): pdflatex $<
	TEXINPUTS=.:$(DIR): pdflatex $<

web: nametag.t2t nametag_user.t2t nametag_api.t2t
	$(DIR)/txt2tags -t html -C $(DIR)/web.conf -o web.html nametag.t2t
	$(DIR)/txt2tags -t html -C $(DIR)/web.conf -C $(DIR)/web_subpage.conf -o web_users_guide.html nametag_user.t2t
	$(DIR)/txt2tags -t html -C $(DIR)/web.conf -C $(DIR)/web_subpage.conf -o web_api.html nametag_api.t2t

manual: html pdf txt
	$(foreach fmt,$^,cp nametag.$(fmt) ../MANUAL.$(fmt); )

.PHONY: clean veryclean
clean:
	rm -f *.aux *.dvi *.html *.log *.pdf *.out *.tex *.toc *.txt

veryclean: clean
	rm -f ../MANUAL.*
