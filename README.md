//IF_ID_latch
uint32_t IF_ID_INST; - fetched instruction
uint32_t IF_ID_NPC; - pass PC information for branch in future

//ID_EX_latch
uint32_t ID_EX_NPC; - pass PC information for branch in future
uint32_t ID_EX_REG1; - first value fetched from register file
uint32_t ID_EX_REG2; - second value fetched from register file
short ID_EX_IMM; - immediate value from instruction
unsigned char ID_EX_DEST; - register to write memory or alu result, if needed

//EX_MEM_latch
uint32_t EX_MEM_ALU_OUT; - result of the ALU unit
uint32_t EX_MEM_W_VALUE; - value to write to memory
uint32_t EX_MEM_BR_TARGET; - target to branch if branch taken
uint32_t EX_MEM_BR_TAKE; - whether to branch
unsigned char EX_MEM_DEST; - register to write memory or alu result, if needed

//MEM_WB_latch
uint32_t MEM_WB_ALU_OUT; - result of the ALU unit
uint32_t MEM_WB_MEM_OUT; - value to write to memory
unsigned char MEM_WB_DEST; - register to write memory or alu result, if needed

//Forwarding
unsigned char ID_EX_RS; - rs value to be used in ID stage
unsigned char ID_EX_RT; - rt value to be used in ID stage
unsigned char EX_MEM_RegWrite; - whether the instruction in EX stage is going to write to register
unsigned char MEM_WB_RegWrite; - whether the instruction in MEM stage is going to write to register

unsigned char IF_ID_RT; - rt value to be used in IF stage
unsigned char IF_ID_RS; - rs value to be used in IF stage
unsigned char ID_EX_MemRead; - whether instruction in id stage reads from memory