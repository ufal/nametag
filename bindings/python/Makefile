include ../common/Makefile.common

ifdef OSX
  BINDING_MODULE=ufal_nametag.so
else
  BINDING_MODULE=$(call dynlib,ufal_nametag)
endif
BINDING_C_FLAGS+=-w

all: $(BINDING_MODULE)

include ../common/Makefile.rules

$(BINDING_MODULE): nametag_python.cpp $(BINDING_NAMETAG_LIB)
	@$(if $(PYTHON_INCLUDE),,$(error PYTHON_INCLUDE must be set to the directory containing Python.h))
	$(CXX) -I../../src/include -I$(PYTHON_INCLUDE) $(DYN_C_FLAGS) $(DYN_LD_FLAGS) $(BINDING_C_FLAGS) $(BINDING_LD_FLAGS) $(call version_script,nametag_python.map) $^ -o $@

nametag_python.cpp: nametag_python.i ../common/nametag.i
	@$(call mkdir,ufal)
	@$(call touch,ufal/__init__.py)
	swig $(SWIG_FLAGS) -python -outdir ufal -o $@ -interface ufal_nametag -builtin $<

.PHONY: clean
clean:
	@$(call rmdir,ufal) $(call rm,$(BINDING_MODULE) nametag_python.cpp)
