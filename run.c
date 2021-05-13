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
        case J:
        case JAL:
            return 'J';
        default:
            return 0;
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
            //printf("--------------------------------------------0x%010x, 0x%010x\n", a, b);
            return a | b;
	    case 0xb:		//SLTIU
            return (a < b) ? 1 : 0;
	    case 0x4:		//BEQ
            return (a == b) ? 1 : 0;
	    case 0x5:		//BNE
            // printf("--------------------------------------------0x%010x, 0x%010x\n", a, b);
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
                    return b << instr.r_t.r_i.r_i.r.shamt;
                case 0x02: //SRL
                    return b >> instr.r_t.r_i.r_i.r.shamt;
                case 0x23: //SUBU
                    return a - b;
                default:
                    return 0;
            }

    	    //J format
	    case 0x2:		//J
            return 0;
	    case 0x3:		//JAL
            return a; // PC + 4 value is passed through CURRENT_STATE.ID_EX_REG1 latch
        default:
            return 0;
    }
}

unsigned char RegWrite(instruction instr) {
    if ((type(instr)=='R' && instr.opcode!=JR) || (type(instr)=='I' && instr.opcode!=SW && instr.opcode!=BEQ && instr.opcode!=BNE) || instr.opcode==JAL) return 1;
    else return 0;
}

void IF() {
    instruction instr = *get_inst_info(CURRENT_STATE.PIPE[0]);
    CURRENT_STATE.IF_ID_INST = instr.value;
    CURRENT_STATE.IF_ID_NPC = CURRENT_STATE.PC;
}

void ID() {
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

        if (instr.opcode==ANDI || instr.opcode==ORI) CURRENT_STATE.ID_EX_IMM = ZeroExtImm(imm);
        else CURRENT_STATE.ID_EX_IMM = SignExtImm(imm);

        if (type(instr)=='R') CURRENT_STATE.ID_EX_DEST = rd;
        else if (instr.opcode==JAL) CURRENT_STATE.ID_EX_DEST = 31;
        else CURRENT_STATE.ID_EX_DEST = rt;

        CURRENT_STATE.ID_EX_RS = rs;
        CURRENT_STATE.ID_EX_RT = rt;
    }

}

void EX() {
    CURRENT_STATE.EX_MEM_BR_TARGET = CURRENT_STATE.ID_EX_NPC + (CURRENT_STATE.ID_EX_IMM << 2);

    instruction instr = *get_inst_info(CURRENT_STATE.PIPE[2]);

    if (CURRENT_STATE.EX_MEM_RegWrite && (CURRENT_STATE.EX_MEM_DEST!=0) && (CURRENT_STATE.EX_MEM_DEST==CURRENT_STATE.ID_EX_RS)) {
        CURRENT_STATE.ID_EX_REG1 = CURRENT_STATE.EX_MEM_ALU_OUT;
        //printf("Forwarded rs from EX_MEM to EX\n");
    }
    if (CURRENT_STATE.EX_MEM_RegWrite && (CURRENT_STATE.EX_MEM_DEST!=0) && (CURRENT_STATE.EX_MEM_DEST==CURRENT_STATE.ID_EX_RT)) {
        CURRENT_STATE.ID_EX_REG2 = CURRENT_STATE.EX_MEM_ALU_OUT;
        //printf("Forwarded rt from EX_MEM to EX\n");
    }
    if (CURRENT_STATE.MEM_WB_RegWrite && (CURRENT_STATE.MEM_WB_DEST!=0) && (CURRENT_STATE.EX_MEM_DEST!=CURRENT_STATE.ID_EX_RS) && (CURRENT_STATE.MEM_WB_DEST==CURRENT_STATE.ID_EX_RS)) {
        CURRENT_STATE.ID_EX_REG1 = CURRENT_STATE.MEM_WB_MEM_OUT;
        //printf("Forwarded rs from MEM_WB to EX\n");
    }
    if (CURRENT_STATE.MEM_WB_RegWrite && (CURRENT_STATE.MEM_WB_DEST!=0) && (CURRENT_STATE.EX_MEM_DEST!=CURRENT_STATE.ID_EX_RT) && (CURRENT_STATE.MEM_WB_DEST==CURRENT_STATE.ID_EX_RT)) {
        CURRENT_STATE.ID_EX_REG2 = CURRENT_STATE.MEM_WB_MEM_OUT;
        //printf("Forwarded rt from MEM_WB to EX\n");
    }

    uint32_t rtOrImm = (type(instr)=='R' || instr.opcode==BEQ || instr.opcode==BNE) ? CURRENT_STATE.ID_EX_REG2 : CURRENT_STATE.ID_EX_IMM;
    CURRENT_STATE.EX_MEM_ALU_OUT = ALU(instr, CURRENT_STATE.ID_EX_REG1, rtOrImm);
    CURRENT_STATE.EX_MEM_BR_TAKE = CURRENT_STATE.EX_MEM_ALU_OUT;
    //printf("EX Stage -- ALU out: 0x%08x", CURRENT_STATE.EX_MEM_ALU_OUT);

    CURRENT_STATE.EX_MEM_W_VALUE = CURRENT_STATE.ID_EX_DEST;
    CURRENT_STATE.EX_MEM_DEST = CURRENT_STATE.ID_EX_DEST;

    CURRENT_STATE.EX_MEM_RegWrite = RegWrite(instr);

}

void MEM() {
    instruction instr = *get_inst_info(CURRENT_STATE.PIPE[3]);

    if ((instr.opcode==BEQ || instr.opcode==BNE) && CURRENT_STATE.EX_MEM_ALU_OUT) { // if branch instruction and should take
        CURRENT_STATE.PIPE[0] = 0; // flush
        CURRENT_STATE.PIPE[1] = 0;
        CURRENT_STATE.PIPE[2] = 0;
        CURRENT_STATE.PC = CURRENT_STATE.EX_MEM_BR_TARGET;  // and set PC for next instruction
    } else {
        if (instr.opcode==LW) {
            // printf("Loaded from the address: %08x\n", CURRENT_STATE.EX_MEM_ALU_OUT);
            CURRENT_STATE.MEM_WB_MEM_OUT = mem_read_32(CURRENT_STATE.EX_MEM_ALU_OUT);
        }
        else if (instr.opcode==SW) mem_write_32(CURRENT_STATE.EX_MEM_ALU_OUT, CURRENT_STATE.EX_MEM_W_VALUE);

        CURRENT_STATE.MEM_WB_ALU_OUT = CURRENT_STATE.EX_MEM_ALU_OUT;
        CURRENT_STATE.MEM_WB_DEST = CURRENT_STATE.EX_MEM_DEST;

        CURRENT_STATE.MEM_WB_RegWrite = RegWrite(instr);
    }
}

void WB() {
    instruction instr = *get_inst_info(CURRENT_STATE.PIPE[4]);
    if ((type(instr)=='R' && instr.opcode!=JR) || (type(instr)=='I' && instr.opcode!=SW && instr.opcode!=BEQ && instr.opcode!=BNE) || instr.opcode==JAL) {
        //printf("WB stage -- MEM out: 0x%08x,  ALU out: 0x%08x", CURRENT_STATE.MEM_WB_MEM_OUT, CURRENT_STATE.MEM_WB_ALU_OUT);
        if (instr.opcode==LW) CURRENT_STATE.REGS[CURRENT_STATE.MEM_WB_DEST] = CURRENT_STATE.MEM_WB_MEM_OUT;
        else CURRENT_STATE.REGS[CURRENT_STATE.MEM_WB_DEST] = CURRENT_STATE.MEM_WB_ALU_OUT;
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

    if (CURRENT_STATE.PIPE[4]) {
        WB();
        INSTRUCTION_COUNT += 1;
    }
    if (CURRENT_STATE.PIPE[3]) MEM();
    if (CURRENT_STATE.PIPE[2]) EX();
    if (CURRENT_STATE.PIPE[1]) ID();
    if (CURRENT_STATE.PIPE[0]) IF();

    // int k;
    // printf("Current register values :\n");
    // printf("-------------------------------------\n");
    // printf("PC: 0x%08x\n", CURRENT_STATE.PC);
    // printf("Registers:\n");
    // for (k = 0; k < MIPS_REGS; k++)
	// printf("R%d: 0x%08x\n", k, CURRENT_STATE.REGS[k]);
    // printf("\n");

    // for(int k = 0; k < 5; k++)
    // {
    //     if(CURRENT_STATE.PIPE[k]){
    //         instruction instr = *get_inst_info(CURRENT_STATE.PIPE[k]);
    //         printf("0x%06x, 0x%06x", instr.opcode, instr.func_code);
    //     }
            
    //     else
    //         printf("          ");

    //     if( k != PIPE_STAGE - 1 )
    //         printf("|");
    // }
    // printf("\n\n");
   
    // mdump(0x10000000, 0x100000f0);
    
}