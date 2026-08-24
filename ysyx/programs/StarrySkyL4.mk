# StarrySkyL4 当前只允许清单中声明该板卡的程序参与能力校验。
YSYX_L4_PROGRAMS := $(foreach program,$(YSYX_PROGRAMS),\
	$(if $(filter starrysky-l4,$(PROGRAM_$(program)_BOARDS)),$(program)))
