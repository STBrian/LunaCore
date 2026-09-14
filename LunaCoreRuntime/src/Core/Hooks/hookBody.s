.macro BEGIN_ASM_FUNC name, linkage=global, section=text
    .section        .\section\().\name, "ax", %progbits
    .align          2
    .\linkage       \name
    .type           \name, %function
    .func           \name
    .cfi_sections   .debug_frame
    .cfi_startproc
    \name:
.endm

.macro END_ASM_FUNC
    .cfi_endproc
    .endfunc
.endm


BEGIN_ASM_FUNC CoreLightHookPreCallback
    stmdb sp!, {r0, r1, r2, r3, r4, lr}     // just to clarify, this lr value is not the original one
    mov r0, lr
    bl CoreLightHookGetContext
    mov r4, r0
    bl CoreLightHookCallback
    ldmia sp!, {r0, r1, r2, r3}
    blx r4
    // Then in the light hook context
    // ldmia sp!, {r4, lr}
    // nop  <-  placeholder for original instruction
    // nop  <-  placeholder for original instruction
    // bx lr
END_ASM_FUNC 


BEGIN_ASM_FUNC CoreHookReturnOverride
    // r0 contains hookCtxPtr, r1 returnCallback
    add r2, r0, #16             // discard r0-r3
    ldmia r2, {r4-r12, sp, lr}  // restore registers
    mov r2, r1
    ldr r1, [r0, #4]
    ldr r0, [r0]
    bx r2
END_ASM_FUNC