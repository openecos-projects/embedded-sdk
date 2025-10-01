.section .rodata
.align 3
.global app_start, app_end
app_start:
#ifdef BIN_PATH
.incbin BIN_PATH
#endif
app_end:
