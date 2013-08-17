.global target_mov_Ev_Iz
target_mov_Ev_Iz:
	movl $0x5,0x18(%rsi)

.global target_mov_Gv_Ev
target_mov_Gv_Ev:
	mov    0x8(%rbp),%rdx

.global target_pop_rAX_r8
target_pop_rAX_r8:
	pop    %rax

.global target_pop_rBP_r13
target_pop_rBP_r13:
	pop    %rbp
