ROOT   = .
VM_ROOT = ../../..

#OPT = 0
NO_JIT = 0
WITH_SDL = 1

ifneq ($(NO_JIT), 1)
JIT_OBJ = $(OBJ)/jit$(OBJ_SUFF)
else
JIT_OBJ =
endif

USER_OBJ = \
   $(JIT_OBJ) \
   $(OBJ)/main$(OBJ_SUFF) \
   $(OBJ)/translator_impl$(OBJ_SUFF) \
   $(OBJ)/pretty_printer$(OBJ_SUFF) \
   $(OBJ)/bytecode_translator$(OBJ_SUFF) \
   $(OBJ)/bytecode_interpreter$(OBJ_SUFF) 


include $(VM_ROOT)/common.mk

INCLUDE += -I$(ROOT)/include
CFLAGS += -std=c++11 -O3

MATHVM = $(BIN)/mvm

all: $(MATHVM)

$(MATHVM): $(OUT) $(USER_OBJ) $(MATHVM_OBJ)
	$(CXX) -o $@ $(USER_OBJ) $(MATHVM_OBJ) $(LIBS)
