.section .rodata
.align 3
.global app_start, app_end
app_start:
.incbin "build/app.bin"
app_end:
