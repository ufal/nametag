include ../common/Makefile.common

ifdef LINUX
  BINDING_MODULE=$(call dynlib,libnametag_java)
  BINDING_C_FLAGS+=-I$(JAVA_HOME)/include -I$(JAVA_HOME)/include/linux
else ifdef WIN
  BINDING_MODULE=$(call dynlib,nametag_java)
  BINDING_C_FLAGS+=-I"$(JAVA_HOME)"/include -I"$(JAVA_HOME)"/include/win32
  BINDING_LD_FLAGS+=-Wl,--kill-at
else ifdef OSX
  BINDING_MODULE=$(call dynlib,libnametag_java)
  BINDING_C_FLAGS+=-I$(JAVA_HOME)/include -I$(JAVA_HOME)/include/darwin $(patsubst %/Home,-I%/Headers,$(filter %/Home,$(JAVA_HOME)))
endif
BINDING_C_FLAGS += -fno-strict-aliasing

all: $(BINDING_MODULE) nametag.jar

include ../common/Makefile.rules

$(BINDING_MODULE): nametag_java.cpp $(BINDING_NAMETAG_LIB)
	@$(if $(JAVA_HOME),,$(error JAVA_HOME must be set))
	$(CXX) -I../../src/include $(DYN_C_FLAGS) $(DYN_LD_FLAGS) $(BINDING_C_FLAGS) $(BINDING_LD_FLAGS) $(call version_script,nametag_java.map) $^ -o $@

nametag_java.cpp: nametag_java.i ../common/nametag.i
	@$(call mkdir,cz/cuni/mff/ufal/nametag)
	swig $(SWIG_FLAGS) -java -package cz.cuni.mff.ufal.nametag -outdir cz/cuni/mff/ufal/nametag -o $@ $<

nametag.jar: nametag_java.cpp
	javac -source 1.4 -Xlint:-options $(wildcard cz/cuni/mff/ufal/nametag/*.java)
	jar cf $@ $(patsubst %.java,%.class,$(wildcard cz/cuni/mff/ufal/nametag/*.java))

.PHONY: clean
clean:
	@$(call rmdir,cz) $(call rm,$(BINDING_MODULE) nametag.jar nametag_java.cpp $(wildcard examples/*.class))
