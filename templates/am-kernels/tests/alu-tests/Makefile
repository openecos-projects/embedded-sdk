NAME = alutest
SRCS = build/alu_test.c
include configs/abstract-machine.mk
include $(ECOS_SDK_HOME)/environments/abstract-machine/build/application.mk

GENERATOR = build/gen_alu_test

$(GENERATOR): gen_alu_test.c
	gcc -O2 -Wall -Werror $^ -o $@

$(SRCS): $(GENERATOR)
	$^ > $@
