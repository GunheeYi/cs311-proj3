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

char type(instruction instr) {
    switch (instr.opcode) {
        case ADDIU:
        case ANDI:
        case LUI:
        case ORI:
        case SLTIU:
        case LW:
        case SW:
        case BEQ:
        case BNE:
            return 'I';
        case 0:
            return 'R';
        case 0x2:
        case 0x3:
            return 'J';
    }
}

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

uint32_t ZeroExtImm(short imm) {
    return 0x0000ffff & imm;
}

uint32_t SignExtImm(short imm) {
    return (imm >> 15) ? imm : ZeroExtImm(imm);
}

uint32_t BranchAddr(short imm) {
    return SignExtImm(imm) << 2;
}

uint32_t JumpAddr(int address) {
    return ( (CURRENT_STATE.PC+4) & 0xf0000000 ) | (address << 2);
}

uint32_t ALU(instruction instr, uint32_t a, uint32_t b) {
    switch(instr.opcode) {
        	    //I format
	    case 0x9:		//ADDIU
        case 0x23:		//LW
        case 0x2b:		//SW
            return a + b;
	    case 0xc:		//ANDI
            return a & b;
	    case 0xf:		//LUI	
            return (b << 16) & 0xffff0000;
	    case 0xd:		//ORI
            return a | b;
	    case 0xb:		//SLTIU
            return (a < b) ? 1 : 0;
	    case 0x4:		//BEQ
            return (a == b) ? 1 : 0;
	    case 0x5:		//BNE
            return (a != b) ? 1 : 0;
    	    //R format
	    case 0x0:		//ADDU, AND, NOR, OR, SLTU, SLL, SRL, SUBU if JR
            switch(instr.func_code) {
                case 0x21:  //ADDU
                    return a + b;
                case 0x24: //AND
                    return a & b;
                case 0x08: //JR
                    return 0;
                case 0x27: //NOR
                    return ~ (a | b);
                case 0x25: //OR
                    return a | b;
                case 0x2b: //SLTU
                    return (a < b) ? 1 : 0;
                case 0x00: //SLL
                    return b << instr.shamt;
                case 0x02: //SRL
                    return b >> instr.shamt;
                case 0x23: //SUBU
                    return a - b;
            }

    	    //J format
	    case 0x2:		//J
            return 0;
	    case 0x3:		//JAL
            return a; // PC + 4 value is passed through CURRENT_STATE.ID_EX_REG1 latch
    }
}

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

void IF_Stage() {
    instruction instr = *get_inst_info(CURRENT_STATE.PIPE[0]);
    CURRENT_STATE.IF_ID_INST = instr.value;
    CURRENT_STATE.IF_ID_NPC = CURRENT_STATE.PC + 4;
}

void ID_Stage() {
    CURRENT_STATE.ID_EX_NPC = CURRENT_STATE.IF_ID_NPC;

    instruction instr = *get_inst_info(CURRENT_STATE.PIPE[1]);
    unsigned char rs = instr.r_t.r_i.rs;
    unsigned char rt = instr.r_t.r_i.rt;
    short imm = instr.r_t.r_i.r_i.imm;
    unsigned char rd = instr.r_t.r_i.r_i.r.rd;
    unsigned char shamt = instr.r_t.r_i.r_i.r.shamt;
    uint32_t target = instr.r_t.target;

    if (instr.opcode==0 && instr.func_code==JR) {
        CURRENT_STATE.PIPE[0] = 0; // flush
        CURRENT_STATE.PC = JumpAddr(CURRENT_STATE.REGS[31]);
    } else if (instr.opcode==J) {
        CURRENT_STATE.PIPE[0] = 0; // flush
        CURRENT_STATE.PC = JumpAddr(target);
    } else if (instr.opcode==JAL) {
        CURRENT_STATE.PIPE[0] = 0; // flush
        CURRENT_STATE.PC = JumpAddr(target);
        CURRENT_STATE.ID_EX_REG1 = CURRENT_STATE.PIPE[1] + 4;
    } else { // if not jump
        CURRENT_STATE.ID_EX_REG1 = CURRENT_STATE.REGS[rs];
        CURRENT_STATE.ID_EX_REG2 = CURRENT_STATE.REGS[rt];

        if (instr.opcode==ANDI || instr.opode==ORI) CURRENT_STATE.ID_EX_IMM = ZeroExtImm(imm);
        else CURRENT_STATE.ID_EX_IMM = SignExtImm(imm);

        if (type(instr)=='R') CURRENT_STATE.ID_EX_DEST = rd;
        else if (instr.opcode==JAL) CURRENT_STATE.ID_EX_DEST = 31;
        else CURRENT_STATE.ID_EX_DEST = rt;
    }

}

void EX_Stage() {
    CURRENT_STATE.EX_MEM_BR_TARGET = CURRENT_STATE.ID_EX_NPC + (CURRENT_STATE.ID_EX_IMM << 2);

    instruction instr = *get_inst_info(CURRENT_STATE.PIPE[2]);
    // 고쳐야하ㅏㅏㅏㅏㅏㅏㅏㅏㅏㅏㅏㅏㅏㅏㅏㅏㅏㅏㅏㅏㅏㅏㅏㅏㅏㅏㅁㅁㅁㅁㅁㅁㅁㅁㅁㅁㅁㅁㅁㅁㅁㅁㅁㅁㅁㅁㅁㅁㅁㅁㅁ
    uint32_t rtOrImm = (type(instr)=='R' || instr.opcode==BEQ || instr.opcode==BNE) ? CURRENT_STATE.ID_EX_REG2 : CURRENT_STATE.ID_EX_IMM;
    CURRENT_STATE.EX_MEM_ALU_OUT = ALU(instr, CURRENT_STATE.ID_EX_REG1, rtOrImm);
    CURRENT_STATE.EX_MEM_BR_TAKE = CURRENT_STATE.EX_MEM_ALU_OUT;

    CURRENT_STATE.EX_MEM_DEST = CURRENT_STATE.ID_EX_DEST;
}

void MEM_Stage() {
    instruction instr = *get_inst_info(CURRENT_STATE.PIPE[3]);

    if((instr.opcode==BEQ || instr.opcode==BNE) && CURRENT_STATE.EX_MEM_ALU_OUT) { // if branch instruction and should take
        CURRENT_STATE.PIPE[0] = 0; // flush
        CURRENT_STATE.PC = JumpAddr(target);  // and set PC for next instruction
    }

    if (instr.opcode==LW) CURRENT_STATE.MEM_WB_MEM_OUT = mem_read_32(CURRENT_STATE.REGS[CURRENT_STATE.EX_MEM_ALU_OUT]);
    else if (instr.opcode==SW) mem_write_32(CURRENT_STATE.REGS[CURRENT_STATE.EX_MEM_ALU_OUT], CURRENT_STATE.EX_MEM_W_VALUE);

    CURRENT_STATE.MEM_WB_ALU_OUT = CURRENT_STATE.EX_MEM_ALU_OUT;

    CURRENT_STATE.MEM_WB_DEST = CURRENT_STATE.EX_MEM_DEST;

    // uint32_t EX_MEM_BR_TARGET;
	// uint32_t EX_MEM_BR_TAKE;
    // uint32_t MEM_WB_BR_TAKE;
}

void WB_Stage() {
    instruction instr = *get_inst_info(CURRENT_STATE.PIPE[4]);
    if ((type(instr)=='R' && instr.opcode!=JR) || (type(instr)=='I' && instr.opcode!=SW && instr.opcode!=BEQ && instr.opcode!=BNE)) {
        if (instr.opcode==LW) CURRENT_STATE.REGS[CURRENT_STATE.MEM_WB_DEST] = CURRENT_STATE.MEM_WB_MEM_OUT;
        else CURRENT_STATE.REGS[CURRENT_STATE.MEM_WB_DEST] = CURRENT_STATE.MEM_WB_ALU_OUT;
    }
}
