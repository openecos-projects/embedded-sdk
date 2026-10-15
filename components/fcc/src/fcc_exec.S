.section .fcc_exec,"awx",@nobits
.balign 16
.globl fcc_code_buffer
.type fcc_code_buffer, @object
fcc_code_buffer:
    .skip 32768
.size fcc_code_buffer, 32768
