
.globl cm_call_smi

cm_call_smi:
	stmdb   sp!, {r1-r12,lr}

	ldr   r1,  [r0, #4]
	ldr   r2,  [r0, #8]
	ldr   r0,  [r0, #0]

#if (__GNUC__ >= 4 && __GNUC_MINOR__ >= 5)
    .arch_extension sec
#endif
	smc #0
	ldmia   sp!, {r1-r12,pc}

