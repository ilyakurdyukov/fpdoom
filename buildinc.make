ifeq ($(PCTEST), 1)
include ../build_pc.make
else ifeq ($(T117), 1)
include ../build_t117.make
else
include ../build_sc6531.make
endif
