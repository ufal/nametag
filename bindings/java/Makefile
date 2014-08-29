include ../common/Makefile.common

ifdef WIN
  JAVA_NAMETAG_LIB=$(call lib,../../src/libnametag)
  JAVA_C_FLAGS=-DNAMETAG_STATIC -I"$(JAVA_HOME)"/include -I"$(JAVA_HOME)"/include/win32
  JAVA_LD_FLAGS=-Wl,--kill-at
  JAVA_MODULE=$(call dynlib,nametag_java)
else
  JAVA_NAMETAG_LIB=$(call lib,../../src/libnametag.dyn)
  JAVA_C_FLAGS=-I$(JAVA_HOME)/include -I$(JAVA_HOME)/include/linux
  JAVA_MODULE=$(call dynlib,libnametag_java)
endif

TARGETS = $(JAVA_MODULE) nametag.jar

all: $(TARGETS)

include ../common/Makefile.rules

$(JAVA_MODULE): nametag_java.cpp $(JAVA_NAMETAG_LIB)
	@$(if $(JAVA_HOME),,$(error JAVA_HOME must be set))
	$(CXX) -I../../src/include $(DYN_C_FLAGS) $(DYN_LD_FLAGS) $(JAVA_C_FLAGS) $(JAVA_LD_FLAGS) -fno-strict-aliasing -Wl,--version-script=nametag_java.map $^ -o $@

nametag_java.cpp: nametag_java.i ../common/nametag.i
	@$(call mkdir,cz/cuni/mff/ufal/nametag)
	swig $(SWIG_FLAGS) -java -package cz.cuni.mff.ufal.nametag -outdir cz/cuni/mff/ufal/nametag -o $@ $<

nametag.jar: nametag_java.cpp
	javac -source 1.4 -Xlint:-options $(wildcard cz/cuni/mff/ufal/nametag/*.java)
	jar cf $@ $(patsubst %.java,%.class,$(wildcard cz/cuni/mff/ufal/nametag/*.java))

.PHONY: clean
clean:
	@$(call rmdir,cz) $(call rm,$(TARGETS) $(wildcard *.cpp)) $(call rm,$(wildcard examples/*.class))
