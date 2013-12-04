all:
	@echo "make [html | pdf | txt | web | manual]"

html pdf tex txt:%:nametag.%

nametag.html nametag.tex nametag.txt: nametag.%: nametag.t2t *.t2t
	./txt2tags -t $* -C not_web.conf $<

nametag.pdf: nametag.tex *.sty
	pdflatex $<
	pdflatex $<

web: nametag.t2t nametag_user.t2t nametag_api.t2t
	./txt2tags -t html -C web.conf -o web.html nametag.t2t
	./txt2tags -t html -C web.conf -C web_subpage.conf -o web_users_guide.html nametag_user.t2t
	./txt2tags -t html -C web.conf -C web_subpage.conf -o web_api.html nametag_api.t2t

manual: html pdf txt
	$(foreach fmt,$^,cp nametag.$(fmt) ../MANUAL.$(fmt); )

.PHONY: clean
clean:
	rm -f *.aux *.dvi *.html *.log *.pdf *.out *.tex *.toc *.txt
