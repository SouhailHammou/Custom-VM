/*
VM by Souhail Hammou : custom instruction set
data space and stack space are customizable.
Important : In calculations the VM is using unsigned values.
*/
#include <stdio.h>
#include <stdint.h>
#include <conio.h>
#define TRUE 1
#define FALSE 0
typedef unsigned char boolean;
typedef uint8_t BYTE;
typedef uint16_t WORD;
typedef uint32_t DWORD;
typedef struct
{
    /*data has also the code*/
    BYTE data[4096];
    /*stack space , size of one element is WORD in order to be able to push addresses*/
    WORD stack[256];
}ADDRESS_SPACE,*PADDRESS_SPACE;
typedef struct
{
    /*General Purpose Registers R0 -> R3*/
    WORD GPRs[4];
    union
    {
        unsigned char Flags;
        struct
        {
            unsigned char ZF:1;
            unsigned char CF:1;
            unsigned char Unused:6;
        };
    };
    WORD IP;
    WORD SP;
}REGS,*PREGS;
void VmLoop(PADDRESS_SPACE AS,PREGS Regs)
{
    int i;
    boolean exit = FALSE;
    BYTE opcode,byte_val,byte_val2,byte_val3;
    WORD word_val,word_val2;
    while(!exit)
    {
        /*read byte (opcode)*/
        //printf("[+] IP : %.4X => ",Regs->IP);
        opcode = AS->data[Regs->IP++];
        /*opcodes switch*/
        switch(opcode)
        {
            case 0x90 :
                //printf("NOP\n");
                break;
            /*
            Each nibble of the operand represents a General purpose register (GPR)
            the highest nibble is the destination , the lowest one is the source.
            Example:
            10 12 => MOV R1,R2
            10 11 => MOV R1,R1
            10 01 => MOV R0,R1
            */
            case 0x10 :
                byte_val = AS->data[Regs->IP++];
                if((byte_val & 0xF0) <= 0x30 && (byte_val & 0x0F) <= 3)
                {
                    Regs->GPRs[(byte_val & 0xF0)>>4] = Regs->GPRs[byte_val & 0x0F];
                    //printf("MOV R%d,R%d\n",(byte_val & 0xF0)>>4,byte_val & 0x0F);
                }
                else
                    goto exception;
                break;
            /*
            Move and extend byte from memory to register
            Example:
            12 03 50 00 => MOVX R3,BYTE [0050]
            12 00 00 01 => MOVX R0,BYTE [0100]
            */
            case 0x12 :
                byte_val = AS->data[Regs->IP++];
                if(byte_val > 3)
                    goto exception;
                word_val = *(WORD*)&AS->data[Regs->IP];
                Regs->IP += 2;
                if(word_val >= sizeof(AS->data))
                    goto exception;
                Regs->GPRs[byte_val] = 0;
                *(BYTE*)&Regs->GPRs[byte_val] = AS->data[word_val];
                //printf("MOVX R%d, BYTE [%.4X]\n",byte_val,word_val);
                break;
            /*
            Move word from memory to register
            14 03 50 00 => MOV R3,WORD [0050]
            14 00 00 01 => MOV R0,WORD [0100]
            */
            case 0x14 :
                byte_val = AS->data[Regs->IP++];
                if(byte_val > 3)
                    goto exception;
                word_val = *(WORD*)&AS->data[Regs->IP];
                Regs->IP += 2;
                if(word_val >= sizeof(AS->data))
                    goto exception;
                Regs->GPRs[byte_val] = *(WORD*)&AS->data[word_val];
                //printf("MOV R%d, WORD [%.4X]\n",byte_val,word_val);
                break;
            /*
            Move and extend byte to register
            16 01 15 => MOVX R1,15h
            */
            case 0x16 :
                byte_val = AS->data[Regs->IP++];
                if(byte_val > 3)
                    goto exception;
                Regs->GPRs[byte_val] = 0;
                *(BYTE*)&Regs->GPRs[byte_val] = AS->data[Regs->IP++];
                //printf("MOVX R%d,%.2Xh\n",byte_val,AS->data[Regs->IP - 1]);
                break;
            /*
            Move word to register
            18 01 15 28 => MOV R1,2815h
            */
            case 0x18 :
                byte_val = AS->data[Regs->IP++];
                if(byte_val > 3)
                    goto exception;
                Regs->GPRs[byte_val] = 0;
                Regs->GPRs[byte_val] = *(WORD*)&AS->data[Regs->IP];
                //printf("MOV R%d,%.4Xh\n",byte_val,*(WORD*)&AS->data[Regs->IP]);
                Regs->IP += sizeof(WORD);
                break;
            /*
            Move byte from register to memory location
            ex :
            1C 01 20 01 => MOV BYTE [0120],R1
            1C 03 50 03 => MOV BYTE [0350],R3
            */
            case 0x1c :
                byte_val = AS->data[Regs->IP++];
                if(byte_val > 3)
                    goto exception;
                word_val = *(WORD*)&AS->data[Regs->IP];
                Regs->IP += 2;
                if(word_val >= sizeof(AS->data))
                    goto exception;
                AS->data[word_val] = *(BYTE*)&Regs->GPRs[byte_val];
                //printf("MOV BYTE [%.4X],R%d\n",word_val,byte_val);
                break;
            /*
            Move word from register to memory location
            ex :
            1F 01 20 01 => MOV WORD [0120],R1
            1F 03 50 03 => MOV WORD [0350],R3
            */
            case 0x1f :
                byte_val = AS->data[Regs->IP++];
                if(byte_val > 3)
                    goto exception;
                word_val = *(WORD*)&AS->data[Regs->IP];
                Regs->IP += 2;
                if(word_val >= sizeof(AS->data))
                    goto exception;
                *(WORD*)&AS->data[word_val] = Regs->GPRs[byte_val];
                //printf("MOV WORD [%.4X],R%d\n",word_val,byte_val);
                break;
            /*
                Unconditional Jump
                example :
                E0 10 00 => JMP 0010
                E0 54 02 => JMP 0254
            */
            case 0xE0 :
                word_val = *(WORD*)&AS->data[Regs->IP];
                Regs->IP += 2;
                if(word_val > sizeof(AS->data))
                    goto exception;
                Regs->IP = word_val;
                //printf("JMP %.4X\n",word_val);
                break;
            /*
                JZ : Jump if equal
                E2 54 01 =>JNZ 0154
            */
            case 0xE2 :
                word_val = *(WORD*)&AS->data[Regs->IP];
                Regs->IP += 2;
                if(word_val > sizeof(AS->data))
                    goto exception;
                /*Jump if ZF is set*/
                if(Regs->ZF)
                    Regs->IP = word_val;
                //printf("JZ %.4X\n",word_val);
                break;
            /*
                JNZ : Jump if not equal
                E3 54 01 => JNZ 0154
            */
            case 0xE3 :
                word_val = *(WORD*)&AS->data[Regs->IP];
                Regs->IP += 2;
                if(word_val > sizeof(AS->data))
                    goto exception;
                if(! Regs->ZF)
                    Regs->IP = word_val;
                //printf("JNZ %.4X\n",word_val);
                break;
            /*
                JAE : Jump if above or equal
                E4 54 01 : JAE 0154
            */
            case 0xE4 :
                word_val = *(WORD*)&AS->data[Regs->IP];
                Regs->IP += 2;
                if(word_val > sizeof(AS->data))
                    goto exception;
                if(Regs->ZF || ! Regs->CF)
                    Regs->IP = word_val;
                //printf("JAE %.4X\n",word_val);
                break;
            /*
                JBE : Jump if below or equal
                E6 54 01 : JBE 0154
            */
            case 0xE6 :
                word_val = *(WORD*)&AS->data[Regs->IP];
                Regs->IP += 2;
                if(word_val > sizeof(AS->data))
                    goto exception;
                if(Regs->ZF || Regs->CF)
                    Regs->IP = word_val;
                //printf("JBE %.4X\n",word_val);
                break;
            /*
                JB : Jump if below
                    E8 54 01 : JB 0154
            */
            case 0xE8 :
                word_val = *(WORD*)&AS->data[Regs->IP];
                Regs->IP += 2;
                if(word_val > sizeof(AS->data))
                    goto exception;
                if(Regs->CF && ! Regs->ZF)
                    Regs->IP = word_val;
                //printf("JB %.4X\n",word_val);
                break;
            /*
                JA : Jump if above
                EC 54 01 => JA 0154
            */
            case 0xEC :
                word_val = *(WORD*)&AS->data[Regs->IP];
                Regs->IP += 2;
                if(word_val > sizeof(AS->data))
                    goto exception;
                if( ! Regs->CF && ! Regs->ZF)
                    Regs->IP = word_val;
                //printf("JA %.4X\n",word_val);
                break;
            /*=======================================================*/
            /*ARITHMETIC OPERATIONS ON THE WHOLE REGISTER (WORD)*/
            /*
                ADD : Add value to register
                AD 01 15 00 : ADD R1,15h
                AD 01 01 50 : ADD R1,5001h

                Updated flags :
                ZF and CF
            */
            case 0xAD :
                byte_val = AS->data[Regs->IP++];
                if(byte_val > 3)
                    goto exception;
                word_val = *(WORD*)&AS->data[Regs->IP];
                Regs->IP += 2;
                word_val2 = Regs->GPRs[byte_val] + word_val;
                if(word_val2 == 0)
                    Regs->ZF = 1;
                else
                    Regs->ZF = 0;
                if(word_val2 < Regs->GPRs[byte_val])
                    Regs->CF = 1;
                else
                    Regs->CF = 0;
                Regs->GPRs[byte_val] = word_val2;
                //printf("ADD R%d,%.4X\n",byte_val,word_val);
                break;
            /*
                ADD : Add 2 registers
                A5 12  : ADD R1,R2
                A5 30  : ADD R3,R0
            */
            case 0xA5 :
                byte_val = AS->data[Regs->IP++];
                if((byte_val & 0xF0) <= 0x30 && (byte_val & 0x0F) <= 3)
                {
                    word_val = Regs->GPRs[(byte_val & 0xF0)>>4];
                    word_val2 = Regs->GPRs[(byte_val & 0xF0)>>4] += Regs->GPRs[byte_val & 0x0F];
                    if(word_val2 == 0)
                        Regs->ZF = 1;
                    else
                        Regs->ZF = 0;
                    if(word_val2 < word_val)
                        Regs->CF = 1;
                    else
                        Regs->CF = 0;
                }
                else
                    goto exception;
                //printf("ADD R%d,R%d\n",(byte_val & 0xF0)>>4,byte_val & 0x0F);
                break;
            /*
                ADDL : Add 2 registers (low byte)
                    A2 12 => ADDL R1,R2
            */
            case 0xA2 :
                byte_val = AS->data[Regs->IP++];
                if((byte_val & 0xF0) <= 0x30 && (byte_val & 0x0F) <= 3)
                {
                    byte_val2 = *(BYTE*)&Regs->GPRs[(byte_val & 0xF0)>>4];
                    byte_val3 = *(BYTE*)&Regs->GPRs[(byte_val & 0xF0)>>4] += *(BYTE*)&Regs->GPRs[byte_val & 0x0F];
                    if(byte_val3 == 0)
                        Regs->ZF = 1;
                    else
                        Regs->ZF = 0;
                    if(byte_val3 < byte_val2)
                        Regs->CF = 1;
                    else
                        Regs->CF = 0;
                }
                else
                    goto exception;
                //printf("ADDL R%d,R%d\n",(byte_val & 0xF0)>>4,byte_val & 0x0F);
                break;
            /*
                SUB : substract value from register
                5B 01 15 00 : SUB R1,15h
                5B 01 01 50 : SUB R1,5001h

                Updated flags :
                ZF and CF
            */
            case 0x5B :
                byte_val = AS->data[Regs->IP++];
                if(byte_val > 3)
                    goto exception;
                word_val = *(WORD*)&AS->data[Regs->IP];
                Regs->IP += 2;
                word_val2 = Regs->GPRs[byte_val] - word_val;
                if(word_val2 == 0)
                    Regs->ZF = 1;
                else
                    Regs->ZF = 0;
                if(word_val2 > Regs->GPRs[byte_val])
                    Regs->CF = 1;
                else
                    Regs->CF = 0;
                Regs->GPRs[byte_val] = word_val2;
                //printf("SUB R%d,%.4X\n",byte_val,word_val);
                break;
            /*
            SUB : substract registers (word)
                5C 01 => SUB R0,R1
            */
            case 0x5C :
                byte_val = AS->data[Regs->IP++];
                if((byte_val & 0xF0) <= 0x30 && (byte_val & 0x0F) <= 3)
                {
                    word_val = Regs->GPRs[(byte_val & 0xF0)>>4];
                    word_val2 = Regs->GPRs[(byte_val & 0xF0)>>4] -= Regs->GPRs[byte_val & 0x0F];
                    if(word_val2 == 0)
                        Regs->ZF = 1;
                    else
                        Regs->ZF = 0;
                    if(word_val2 > word_val)
                        Regs->CF = 1;
                    else
                        Regs->CF = 0;
                }
                else
                    goto exception;
                //printf("SUB R%d,R%d",(byte_val & 0xF0)>>4,byte_val & 0x0F);
                break;
            /*
                SUBL : Substract 2 registers (low part)
                    5D 12 => SUBL R1,R2
            */
            case 0x5D :
                byte_val = AS->data[Regs->IP++];
                if((byte_val & 0xF0) <= 0x30 && (byte_val & 0x0F) <= 3)
                {
                    byte_val2 = *(BYTE*)&Regs->GPRs[(byte_val & 0xF0)>>4];
                    byte_val3 = *(BYTE*)&Regs->GPRs[(byte_val & 0xF0)>>4] -= *(BYTE*)&Regs->GPRs[byte_val & 0x0F];
                    if(byte_val3 == 0)
                        Regs->ZF = 1;
                    else
                        Regs->ZF = 0;
                    if(byte_val3 > byte_val2)
                        Regs->CF = 1;
                    else
                        Regs->CF = 0;
                }
                else
                    goto exception;
                //printf("SUBL R%d,R%d",(byte_val & 0xF0)>>4,byte_val & 0x0F);
                break;
            /*
                XOR : Xor 2 registers
                (operand uses nibbles : high = dest , low = source)
                F0 12 => XOR R1,R2
                F0 01 => XOR R0,R1
            */
            case 0xF0 :
                byte_val = AS->data[Regs->IP++];
                if((byte_val & 0xF0) <= 0x30 && (byte_val & 0x0F) <= 3)
                {
                    word_val = Regs->GPRs[(byte_val & 0xF0)>>4] ^= Regs->GPRs[byte_val & 0x0F];
                    if(word_val == 0)
                        Regs->ZF = 1;
                    else
                        Regs->ZF = 0;
                    Regs->CF = 0;
                }
                else
                    goto exception;
                //printf("XOR R%d,R%d\n",(byte_val & 0xF0)>>4,byte_val & 0x0F);
                break;
            /*===============================================================*/
            /*ARITHMETIC OPERATIONS ON THE LOWER BYTE OF THE REGISTER*/
            /*
                XORL : Xor the lower bytes of 2 registers
                F1 12  : XORL R1,R2
                F1 01 :  XORL R0,R1
            */
            case 0xF1 :
                byte_val = AS->data[Regs->IP++];
                if((byte_val & 0xF0) <= 0x30 && (byte_val & 0x0F) <= 3)
                {
                    byte_val2 = *(BYTE*)&Regs->GPRs[(byte_val & 0xF0)>>4] ^= *(BYTE*)&Regs->GPRs[byte_val & 0x0F];
                    if(byte_val2 == 0)
                        Regs->ZF = 1;
                    else
                        Regs->ZF = 0;
                    Regs->CF = 0;
                }
                else
                    goto exception;
                //printf("XORL R%d,R%d\n",(byte_val & 0xF0)>>4,byte_val & 0x0F);
                break;
            /*
                ADDL : add only to the lower of the register
                A1 03 20 => ADDL R3,20h
            */
            case 0xA1:
                byte_val = AS->data[Regs->IP++];
                if(byte_val > 3)
                    goto exception;
                byte_val2 = AS->data[Regs->IP++];
                byte_val3 = *(BYTE*)&Regs->GPRs[byte_val] + byte_val2;
                if(byte_val3 == 0)
                    Regs->ZF = 1;
                else
                    Regs->ZF = 0;
                if(byte_val3 < *(BYTE*)&Regs->GPRs[byte_val])
                    Regs->CF = 1;
                else
                    Regs->CF = 0;
                *(BYTE*)&Regs->GPRs[byte_val] = byte_val3;
                //printf("ADDL R%d,%.2X\n",byte_val,byte_val2);
                break;
            /*
                SUBL : Substract only from the lower byte of the register
                51 03 20 => SUBL R3,20h
            */
            case 0x51:
                byte_val = AS->data[Regs->IP++];
                if(byte_val > 3)
                    goto exception;
                byte_val2 = AS->data[Regs->IP++];
                byte_val3 = *(BYTE*)&Regs->GPRs[byte_val] - byte_val2;
                if(byte_val3 == 0)
                    Regs->ZF = 1;
                else
                    Regs->ZF = 0;
                if(byte_val3 > *(BYTE*)&Regs->GPRs[byte_val])
                    Regs->CF = 1;
                else
                    Regs->CF = 0;
                *(BYTE*)&Regs->GPRs[byte_val] = byte_val3;
                //printf("SUBL R%d,%.2X\n",byte_val,byte_val2);
                break;
            /*===============================================================*/
            /*
            Store register (low byte) at [Rx].
            55 21 => MOV BYTE [R2],R1
            */
            case 0x55 :
                byte_val = AS->data[Regs->IP++];
                if((byte_val & 0xF0) <= 0x30 && (byte_val & 0x0F) <= 3 && Regs->GPRs[(byte_val & 0xF0)>>4] < sizeof(AS->data))
                    AS->data[Regs->GPRs[(byte_val & 0xF0)>>4]] = *(BYTE*)&Regs->GPRs[byte_val & 0x0F];
                else
                    goto exception;
                //printf("MOV BYTE [R%d],R%d\n",(byte_val & 0xF0)>>4,byte_val & 0x0F);
                break;
            /*
            Load and extend low byte of register from memory pointed by a register
            56 21 => MOV R2,BYTE [R1]
            */
            case 0x56 :
                byte_val = AS->data[Regs->IP++];
                if((byte_val & 0xF0) <= 0x30 && (byte_val & 0x0F) <= 3 && Regs->GPRs[(byte_val & 0x0F)] < sizeof(AS->data))
                {
                    *(BYTE*)&Regs->GPRs[(byte_val & 0xF0)>>4] = AS->data[Regs->GPRs[byte_val & 0x0F]];
                    Regs->GPRs[(byte_val & 0xF0)>>4] &= 0xFF;
                }
                else
                    goto exception;
                //printf("MOVX R%d, BYTE [R%d]\n",(byte_val & 0xF0)>>4,byte_val & 0x0F);
                break;
            /*
            CMP : Compare 2 registers (word)
                70 12 : CMP R1,R2
            */
            case 0x70 :
                byte_val = AS->data[Regs->IP++];
                if((byte_val & 0xF0) <= 0x30 && (byte_val & 0x0F) <= 3 && Regs->GPRs[(byte_val & 0x0F)] < sizeof(AS->data))
                {
                    word_val = Regs->GPRs[(byte_val & 0xF0)>>4];
                    word_val2 =  Regs->GPRs[byte_val & 0x0F];
                    if(word_val2 == word_val)
                        Regs->ZF = 1;
                    else
                        Regs->ZF = 0;
                    if(word_val2 > word_val)
                        Regs->CF = 1;
                    else
                        Regs->CF = 0;
                }
                else
                    goto exception;
                //printf("CMP R%d,R%d\n",(byte_val & 0xF0)>>4,byte_val & 0x0F);
                break;
            /*
                CMPL : Compare 2 registers (lower byte)
                71 12 : CMPL R1,R2
            */
            case 0x71 :
                byte_val = AS->data[Regs->IP++];
                if((byte_val & 0xF0) <= 0x30 && (byte_val & 0x0F) <= 3 && Regs->GPRs[(byte_val & 0x0F)] < sizeof(AS->data))
                {
                    byte_val2 = *(BYTE*)&Regs->GPRs[(byte_val & 0xF0)>>4];
                    byte_val3 =  *(BYTE*)&Regs->GPRs[byte_val & 0x0F];
                    if(byte_val3 == byte_val2)
                        Regs->ZF = 1;
                    else
                        Regs->ZF = 0;
                    if(byte_val3 > byte_val2)
                        Regs->CF = 1;
                    else
                        Regs->CF = 0;
                }
                else
                    goto exception;
                //printf("CMP R%d,R%d\n",(byte_val & 0xF0)>>4,byte_val & 0x0F);
                break;
            /*
            Push register
            example : AF 01 => PUSH R1
            */
            case 0xAF :
                byte_val = AS->data[Regs->IP++];
                if(byte_val > 3)
                    goto exception;
                /*Decrement the stack pointer to store the new value*/
                Regs->SP--;
                /*Check for stack overflow*/
                if(Regs->SP == 0xFFFF)
                    goto exception;
                /*Push value */
                AS->stack[Regs->SP] = Regs->GPRs[byte_val];
                //printf("PUSH R%d\n",byte_val);
                break;
            /*
            Pop a register
            AE 01 => POP R1
            */
            case 0xAE :
                byte_val = AS->data[Regs->IP++];
                if(byte_val > 3)
                    goto exception;
                /*
                Check for stack underflow
                */
                if(&AS->stack[Regs->SP] == &AS->stack[sizeof(AS->stack)/sizeof(WORD)])
                    goto exception;
                /*Move the value into the register*/
                Regs->GPRs[byte_val] = AS->stack[Regs->SP];
                /*Value popped , increment SP*/
                Regs->SP++;
                //printf("POP R%d\n",byte_val);
                break;
            /*========================================================*/
            /*User interaction operations (print and receive input)*/
            /*
            Print Word to user as integer, the value must be at the top of the stack and it is popped
            C0 => print integer
            */
            case 0xC0 :
                /*read value then pop it*/
                if(&AS->stack[Regs->SP] == &AS->stack[sizeof(AS->stack)/sizeof(WORD)])
                    goto exception;
                word_val = AS->stack[Regs->SP++];
                //printf("Print integer\n");
                printf("%u\n",word_val);
                break;
            /*
            Print string to user, the pointer must be at the top of the stack and it is popped
            C2 => print string
            */
            case 0xC2 :
                if(&AS->stack[Regs->SP] == &AS->stack[sizeof(AS->stack)/sizeof(WORD)])
                    goto exception;
                /*read it and pop it*/
                word_val = AS->stack[Regs->SP++];
                if(word_val > sizeof(AS->data))
                    goto exception;
                //printf("Print string\n");
                printf("%s",&AS->data[word_val]);
                break;
            /*
            Scan string from user, the pointer where to store the integer must be on top of the stack
            89
            */
            case 0x89 :
                if(&AS->stack[Regs->SP] == &AS->stack[sizeof(AS->stack)/sizeof(WORD)])
                    goto exception;
                /*read it and pop it*/
                word_val = AS->stack[Regs->SP++];
                if(word_val > sizeof(AS->data))
                    goto exception;
                //printf("Scan string\n");
                //printf("    [+] Input : ");
                gets((char*)&AS->data[word_val]);
                break;
            /*=======================================================*/
            /*0xDB Debugging Only*/
            /*
            case 0xDB :
                printf("\n===Debug Information Start===\n");
                printf("+ Registers :\n");
                for(i=0;i<=3;i++)
                    printf("    R%d : 0x%.4X\n",i,Regs->GPRs[i]);
                printf("    IP : 0x%.4X\n",Regs->IP);
                printf("    SP : 0x%.4X\n",Regs->SP*sizeof(WORD));
                printf("+ Current Stack : (Top 4 values)\n");
                if(Regs->SP == sizeof(AS->stack)/sizeof(WORD))
                {
                        printf("    The stack is empty.\n");
                        goto loc;
                }
                for(i=0;i<4;i++)
                {
                    if(Regs->SP + i < sizeof(AS->stack)/sizeof(WORD))
                        printf("    SP+%d => 0x%.4X : %.4X\n",i*2,(Regs->SP + i)*2,AS->stack[Regs->SP+i]);
                }
                loc:
                printf("+Flags Information :\n");
                printf("    Flags = 0x%.2X\n",Regs->Flags);
                printf("        ZF = %d\n",Regs->ZF);
                printf("        CF = %d\n",Regs->CF);
                printf("===Debug Information End  ===\n\n");
                break;
                */
            /*======================================================*/
            case 0xED :
                //printf("Exit\n");
                exit = TRUE;
                break;
            default :
                exception:
                //printf("\n==Exception : ...Exiting==\n");
                exit = TRUE;
        }
    }
}
int main()
{
    PADDRESS_SPACE AS;
    PREGS Regs;
    int size;
    FILE* File;
    //printf("DEBUG INFO :");
    //printf("Allocating Address Space\n");
    AS = (PADDRESS_SPACE) malloc(sizeof(ADDRESS_SPACE));
    //printf("Allocating Registers\n");
    Regs = (PREGS) malloc(sizeof(REGS));
    //printf("Initializing Registers\n");
    Regs->IP = 0;
    Regs->SP = sizeof(AS->stack) / sizeof(WORD);
    Regs->Flags = 0;
    /*Open code and data file and read it into */
    File = fopen("vm_file","rb");
    if(!File)
    {
        printf("Found trouble opening the file");
        return 0;
    }
    /*Check the file size*/
    fseek(File,0,SEEK_END);
    size = ftell(File);
    if( size > sizeof(AS->data))
    {
        printf("File size is larger than the storage available for data and code");
        return 0;
    }
    rewind(File);
    /*Copy the file to our VM address space*/
    fread(AS->data,1,size,File);
    fclose(File);
    //printf("Starting Execution\n");
    VmLoop(AS,Regs);
    _getch();
    return 0;
}
