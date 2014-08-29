include ../common/Makefile.common

ifdef WIN
  PERL_NAMETAG_LIB=$(call lib,../../src/libnametag)
  PERL_C_FLAGS=-DNAMETAG_STATIC
else
  PERL_NAMETAG_LIB=$(call lib,../../src/libnametag.dyn)
endif
PERL_C_FLAGS+=-w

TARGETS = $(call dynlib,auto/Ufal/NameTag/NameTag)

all: $(TARGETS)

include ../common/Makefile.rules

$(call dynlib,auto/Ufal/NameTag/NameTag): nametag_perl.cpp $(PERL_NAMETAG_LIB)
	@$(if $(PERL_INCLUDE),,$(error PERL_INCLUDE must be set to the directory containing perl.h))
	@$(call mkdir,$(dir $@))
	@$(call touch,$(dir $@)NameTag.bs)
	$(CXX) -I../../src/include -I$(PERL_INCLUDE) $(DYN_C_FLAGS) $(DYN_LD_FLAGS) $(PERL_C_FLAGS) $(PERL_LD_FLAGS) -Wl,--version-script=nametag_perl.map $^ -o $@

nametag_perl.cpp: nametag_perl.i ../common/nametag.i
	@$(call mkdir,Ufal)
	swig $(SWIG_FLAGS) -perl -outdir Ufal -o $@ $<

.PHONY: clean
clean:
	@$(call rmdir,Ufal) $(call rmdir,auto) $(call rm,$(wildcard *.cpp))
