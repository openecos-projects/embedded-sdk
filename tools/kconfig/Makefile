NAME = conf
obj := build
SRCS += confdata.c expr.c preprocess.c symbol.c util.c
SRCS += $(obj)/lexer.lex.c $(obj)/parser.tab.c
CC = gcc
CFLAGS += -DYYDEBUG
INC_PATH += .
DISTRO = $(shell cat /etc/os-release | grep PRETTY_NAME | sed 's/PRETTY_NAME=//')
# 动态获取项目根目录路径
ECOS_HOME := $(shell cd $(dir $(lastword $(MAKEFILE_LIST)))/../.. && pwd)

ifeq ($(DISTRO),"Gentoo Linux")
LIBS += -ltinfo
endif

ifeq ($(NAME),conf)
SRCS += conf.c
else ifeq ($(NAME),mconf)
SRCS += mconf.c $(shell find lxdialog/ -name "*.c")
LIBS += -lncurses
else
$(error bad target=$(NAME))
endif

ifneq ($(wildcard ../../scripts/build.mk),)
include ../../scripts/build.mk
else
include $(ECOS_SDK_HOME)/tools/scripts/build.mk
endif

$(obj)/lexer.lex.o: $(obj)/parser.tab.h
$(obj)/lexer.lex.c: lexer.l $(obj)/parser.tab.h	
	@echo + LEX $@
	@flex -o $@ $<

$(obj)/parser.tab.c $(obj)/parser.tab.h: parser.y
	@echo + YACC $@
	@bison -v $< --defines=$(obj)/parser.tab.h -o $(obj)/parser.tab.c

conf:
	@$(MAKE) -s

mconf:
	@$(MAKE) -s NAME=mconf

.PHONY: conf mconf
