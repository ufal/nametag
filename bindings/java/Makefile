include ../Makefile.common

TARGETS = $(call dynlib,libnametag_java)

all: $(TARGETS)

include ../Makefile.rules

$(call dynlib,libnametag_java): nametag_java.cpp $(call lib,../../src/libnametag.dyn)
	@$(if $(JAVA_JNI),,$(error JAVA_JNI must be set to the directory containing jni.h))
	$(CXX) -I../../src/include -I$(JAVA_JNI) $(DYN_C_FLAGS) $(DYN_LD_FLAGS) -fno-strict-aliasing -Wl,--version-script=nametag_java.map $^ -o $@

nametag_java.cpp: nametag_java.i ../nametag.i
	mkdir -p cz/cuni/mff/ufal/nametag
	swig $(SWIG_FLAGS) -java -package cz.cuni.mff.ufal.nametag -outdir cz/cuni/mff/ufal/nametag -o $@ $<

.PHONY: clean
clean:
	rm -rf $(TARGETS) cz/ *.cpp
