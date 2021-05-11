/***************************************************************/
/*                                                             */
/*   MIPS-32 Instruction Level Simulator                       */
/*                                                             */
/*   CS311 KAIST                                               */
/*   run.c                                                     */
/*                                                             */
/***************************************************************/

#include <stdio.h>

#include "util.h"
#include "run.h"

/***************************************************************/
/*                                                             */
/* Procedure: get_inst_info                                    */
/*                                                             */
/* Purpose: Read insturction information                       */
/*                                                             */
/***************************************************************/
instruction* get_inst_info(uint32_t pc) { 
    return &INST_INFO[(pc - MEM_TEXT_START) >> 2];
}

// char type(instruction instr) {
//     switch (instr.opcode) {
//         case ADDIU:
//         case ANDI:
//         case LUI:
//         case ORI:
//         case SLTIU:
//         case LW:
//         case SW:
//         case BEQ:
//         case BNE:
//             return 'I';
//         case 0:
//             return 'R';
//         case 0x2:
//         case 0x3:
//             return 'J';
//     }
// }

// Controls getControls(uint32_t pc) {
//     instruction instr = *get_inst_info(CURRENT_STATE.PC);
//     Controls controls;
//     controls.ALUSrc = () ? 1 : 0;
// 	controls.ALUOp = () ? 1 : 0;
// 	controls.RegDst = () ? 1 : 0;
//     controls.Jump = (instr.opcode==BEQ || instr.opcode==BNE || ) ? 1 : 0
// 	controls.Branch = (instr.opcode==BEQ || instr.opcode==BNE || ) ? 1 : 0;
// 	controls.MemWrite = (instr.opcode==SW) ? 1 : 0;
// 	controls.MemRead = (instr.opcode==LW) ? 1 : 0;
// 	controls.MemtoReg = (instr.opcode==LW) ? 1 : 0;
//     controls.RegWrite = (type(instr)=='R' || (type(instr)=='I' && instr.opcode!=SW && instr.opcode!=BEQ && instr.opcode!=BNE)) ? 1 : 0;
// }


/***************************************************************/
/*                                                             */
/* Procedure: process_instruction                              */
/*                                                             */
/* Purpose: Process one instrction                             */
/*                                                             */
/***************************************************************/
void process_instruction(){
	/** Your implementation here */

    if(CURRENT_STATE.PC >= MEM_REGIONS[0].start + NUM_INST*4) {
        RUN_BIT = FALSE;
        return;
    }

    // CURRENT_STATE.MEM_WB_NPC = CURRENT_STATE.EX_MEM_NPC;
    // CURRENT_STATE.EX_MEM_NPC = CURRENT_STATE.ID_EX_NPC;
    // CURRENT_STATE.ID_EX_NPC = CURRENT_STATE.IF_ID_NPC;
    // CURRENT_STATE.IF_ID_NPC = CURRENT_STATE.PC;

    CURRENT_STATE.PIPE[4] = CURRENT_STATE.PIPE[3];
    CURRENT_STATE.PIPE[3] = CURRENT_STATE.PIPE[2];
    CURRENT_STATE.PIPE[2] = CURRENT_STATE.PIPE[1];
    CURRENT_STATE.PIPE[1] = CURRENT_STATE.PIPE[0];
    CURRENT_STATE.PIPE[0] = CURRENT_STATE.PC;

    CURRENT_STATE.PC += 4;

    if (CURRENT_STATE.PIPE[4]) WB_STAGE();
    if (CURRENT_STATE.PIPE[3]) MEM_STAGE();
    if (CURRENT_STATE.PIPE[2]) EX_STAGE();
    if (CURRENT_STATE.PIPE[1]) ID_STAGE();
    if (CURRENT_STATE.PIPE[0]) IF_STAGE();
    
}

void		IF_Stage() {
    instruction instr = *get_inst_info(CURRENT_STATE.PIPE[0]);
    unsigned char rs = instr.r_t.r_i.rs;
    unsigned char rt = instr.r_t.r_i.rt;
    short imm = instr.r_t.r_i.r_i.imm;
    unsigned char rd = instr.r_t.r_i.r_i.r.rd;
    unsigned char shamt = instr.r_t.r_i.r_i.r.shamt;
    uint32_t target = instr.r_t.target;

}

void ID_Stage() {
    instruction instr = *get_inst_info(CURRENT_STATE.PIPE[1]);
    
}

void EX_Stage() {
    instruction instr = *get_inst_info(CURRENT_STATE.PIPE[2]);

    CURRENT_STATE.EX_MEM_DEST = CURRENT_STATE.ID_EX_DEST;
}

void MEM_Stage() {
    instruction instr = *get_inst_info(CURRENT_STATE.PIPE[3]);

    if (instr.opcode==LW) CURRENT_STATE.MEM_WB_MEM_OUT = mem_read_32(CURRENT_STATE.REGS[CURRENT_STATE.EX_MEM_ALU_OUT]);
    else if (instr.opcode==SW) mem_write_32(CURRENT_STATE.REGS[CURRENT_STATE.EX_MEM_ALU_OUT], CURRENT_STATE.EX_MEM_W_VALUE);

    CURRENT_STATE.MEM_WB_ALU_OUT = CURRENT_STATE.EX_MEM_ALU_OUT;

    CURRENT_STATE.MEM_WB_DEST = CURRENT_STATE.EX_MEM_DEST;
}

void WB_Stage() {
    instruction instr = *get_inst_info(CURRENT_STATE.PIPE[4]);
    CURRENT_STATE.REGS[CURRENT_STATE.MEM_WB_DEST] = (instr.opcode==LW) ? CURRENT_STATE.MEM_WB_MEM_OUT : CURRENT_STATE.MEM_WB_ALU_OUT;
}
