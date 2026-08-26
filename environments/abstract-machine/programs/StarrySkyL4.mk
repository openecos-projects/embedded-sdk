# StarrySkyL4 当前只允许清单中声明该板卡的程序参与能力校验。
AM_L4_PROGRAMS := $(foreach program,$(AM_PROGRAMS),\
	$(if $(filter starrysky-l4,$(PROGRAM_$(program)_BOARDS)),$(program)))
