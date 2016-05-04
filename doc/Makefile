include ../src/Makefile.builtem
include t2t_docsys/Makefile.include

.PHONY: all doc web install
all: doc
doc: nametag.html nametag.txt nametag_readme.txt nametag_readme_md.md nametag_install.txt
doc: nametag_bindings_csharp_readme.txt nametag_bindings_java_readme.txt nametag_bindings_perl_readme.txt nametag_bindings_python_readme.txt
web: web.html web_install.html web_user.html web_api.html web_about.html web_online.html
install: doc
	$(call cp,nametag.txt,../MANUAL)
	$(call cp,nametag.html,../MANUAL.html)
	$(call cp,nametag_readme.txt,../README)
	$(call cp,nametag_readme_md.md,../README.md)
	$(call cp,nametag_install.txt,../INSTALL)
	$(call cp,nametag_bindings_csharp_readme.txt,../bindings/README.CS)
	$(call cp,nametag_bindings_java_readme.txt,../bindings/README.JAVA)
	$(call cp,nametag_bindings_perl_readme.txt,../bindings/README.PERL)
	$(call cp,nametag_bindings_python_readme.txt,../bindings/README.PYTHON)

%.html: %.t2t $(wildcard *.t2t)
	$(call t2t_docsys_html,$@,$<,manual $(if $(findstring _,$*),striplevel))

web%html: nametag%t2t $(wildcard *.t2t)
	$(call t2t_docsys_html,$@,$<,web html_addlevel $(if $(findstring _,$*),striplevel))

%.txt: %.t2t $(wildcard *.t2t)
	$(call t2t_docsys_txt,$@,$<,manual $(if $(findstring _,$*),striplevel))

%.md: %.t2t $(wildcard *.t2t)
	$(call t2t_docsys_md,$@,$<,manual $(if $(findstring _,$*),striplevel))

%.pdf: %.t2t $(wildcard *.t2t)
	$(call t2t_docsys_pdf,$@,$<,manual $(if $(findstring _,$*),striplevel))

.PHONY: clean
clean:
	@$(call rm,$(T2T_DOCSYS_OUTPUT_WILDCARDS))
