#ifndef test_disassembler_target_h
#define test_disassembler_target_h

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

void target_mov_Ev_Iz(void);
void target_mov_Gv_Ev(void);
void target_pop_rAX_r8(void);
void target_pop_rBP_r13(void);

#ifdef __cplusplus
}
#endif // __cplusplus


#endif // test_disassembler_target_h
