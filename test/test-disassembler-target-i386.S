.global target_mov_Ev_Iz
target_mov_Ev_Iz:
	movl $0x5,0x18(%esi)

.global target_mov_Gv_Ev
target_mov_Gv_Ev:
	mov    0x8(%ebp),%edx

.global target_pop_rAX_r8
target_pop_rAX_r8:
	pop    %eax

.global target_pop_rBP_r13
target_pop_rBP_r13:
	pop    %ebp
