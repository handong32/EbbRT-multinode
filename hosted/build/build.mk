MYDIR := $(dir $(lastword $(MAKEFILE_LIST)))

app_sources := \
	test.cc \
	MultiEbb.cc

target := MultiEbb

include $(abspath $(EBBRT_SRCDIR)/apps/ebbrthosted.mk)
