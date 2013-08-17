.global target_mov_Ev_Iz
target_mov_Ev_Iz:
	movl $0x5,0x18(%esi)

.global target_mov_Gv_Ev
target_mov_Gv_Ev:
	mov    0x8(%ebp),%edx
