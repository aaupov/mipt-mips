/*
 * func_instr.h - instruction parser for mips
 * @author Pavel Kryukov pavel.kryukov@phystech.edu
 * Copyright 2014 MIPT-MIPS
 */

/** Edited by Ladin Oleg. */

#ifndef FUNC_INSTR_H
#define FUNC_INSTR_H

// Generic C++
#include <string>
#include <cassert>

#include <boost/utility/string_ref.hpp>

// MIPT-MIPS modules
#include <common/types.h>
#include <common/macro.h>

enum RegNum
{
    REG_NUM_ZERO = 0,
    REG_NUM_AT,
    REG_NUM_V0,
    REG_NUM_V1,
    REG_NUM_A0,
    REG_NUM_A1,
    REG_NUM_A2,
    REG_NUM_A3,
    REG_NUM_T0,
    REG_NUM_T1,
    REG_NUM_T2,
    REG_NUM_T3,
    REG_NUM_T4,
    REG_NUM_T5,
    REG_NUM_T6,
    REG_NUM_T7,
    REG_NUM_S0,
    REG_NUM_S1,
    REG_NUM_S2,
    REG_NUM_S3,
    REG_NUM_S4,
    REG_NUM_S5,
    REG_NUM_S6,
    REG_NUM_S7,
    REG_NUM_T8,
    REG_NUM_T9,
    REG_NUM_K0,
    REG_NUM_K1,
    REG_NUM_GP,
    REG_NUM_SP,
    REG_NUM_FP,
    REG_NUM_RA,
    REG_NUM_MAX
};

class FuncInstr
{
    private:
        enum Format
        {
            FORMAT_R,
            FORMAT_I,
            FORMAT_J,
            FORMAT_UNKNOWN
        } format = FORMAT_UNKNOWN;

        enum OperationType
        {
            OUT_R_ARITHM,
            OUT_R_SHIFT,
            OUT_R_SHAMT,
            OUT_R_JUMP,
            OUT_R_JUMP_LINK,
            OUT_R_SPECIAL,
            OUT_I_ARITHM,
            OUT_I_BRANCH,
            OUT_I_BRANCH_0,
            OUT_I_LOAD,
            OUT_I_LOADU,
            OUT_I_CONST,
            OUT_I_STORE,
            OUT_J_JUMP,
            OUT_J_JUMP_LINK,
            OUT_J_SPECIAL,
            OUT_UNKNOWN
        } operation = OUT_UNKNOWN;

        union _instr
        {
            struct
            {
                unsigned funct  :6;
                unsigned shamt  :5;
                unsigned rd     :5;
                unsigned rt     :5;
                unsigned rs     :5;
                unsigned opcode :6;
            } asR;
            struct
            {
                unsigned imm    :16;
                unsigned rt     :5;
                unsigned rs     :5;
                unsigned opcode :6;
            } asI;
            struct
            {
                unsigned imm    :26;
                unsigned opcode :6;
            } asJ;
            uint32 raw;

            _instr() {} // constructor w/o arguments for ports
            _instr(uint32 bytes) {
                 raw = bytes;
            }
        } instr = NO_VAL32;

        using Execute = void (FuncInstr::*)(void);

        struct ISAEntry
        {
            std::string name;

            uint8 opcode;

            Format format;
            OperationType operation;

            uint8 mem_size;

            FuncInstr::Execute function;

            uint8 mips_version;
        };

        static const ISAEntry isaTable[];
        static const char *regTableName(RegNum);
        static const char *regTable[];

        boost::string_ref name = {};

        RegNum src1 = REG_NUM_ZERO;
        RegNum src2 = REG_NUM_ZERO;
        RegNum dst = REG_NUM_ZERO;

        uint32 v_imm = NO_VAL32;
        uint32 v_src1 = NO_VAL32;
        uint32 v_src2 = NO_VAL32;
        uint32 v_dst = NO_VAL32;
        Addr mem_addr = NO_VAL32;
        uint32 mem_size = NO_VAL32;

        bool complete = false;

        /* info for branch misprediction unit */
        bool predicted_taken;        // Predicted direction
        Addr predicted_target;     // PC, predicted by BPU
        bool _is_jump_taken = false; // actual result

        Addr PC = NO_VAL32; // removing "const" keyword to supporting ports
        Addr new_PC = NO_VAL32;

        std::string disasm = "";

        void initFormat();
        void initR();
        void initI();
        void initJ();
        void initUnknown();

        void execute_add()   { v_dst = static_cast<int32>(v_src1) + static_cast<int32>(v_src2); }
        void execute_addu()  { v_dst = v_src1 + v_src2; }
        void execute_sub()   { v_dst = static_cast<int32>(v_src1) - static_cast<int32>(v_src2); }
        void execute_subu()  { v_dst = v_src1 - v_src2; }
        void execute_addi()  { v_dst = static_cast<int32>(v_src1) + static_cast<int16>(v_imm); }
        void execute_addiu() { v_dst = v_src1 + v_imm; }

        void execute_mult()  { uint64 mult_res = v_src1 * v_src2; lo = mult_res & 0xFFFFFFFF; hi = mult_res >> 0x20; };
        void execute_multu() { uint64 mult_res = v_src1 * v_src2; lo = mult_res & 0xFFFFFFFF; hi = mult_res >> 0x20; };
        void execute_div()   { lo = v_src2 / v_src1; hi = v_src2 % v_src1; };
        void execute_divu()  { lo = v_src2 / v_src1; hi = v_src2 % v_src1; };
        void execute_mfhi()  { v_dst = hi; };
        void execute_mthi()  { hi = v_src2; };
        void execute_mflo()  { v_dst = lo; };
        void execute_mtlo()  { lo = v_src2;};

        void execute_sll()   { v_dst = v_src1 << v_imm; }
        void execute_srl()   { v_dst = v_src1 >> v_imm; }
        void execute_sra()   { v_dst = v_src1 >> v_imm; };
        void execute_sllv()  { v_dst = v_src1 << v_src2; };
        void execute_srlv()  { v_dst = v_src1 >> v_src2; };
        void execute_srav()  { v_dst = v_src1 >> v_src2; };
        void execute_lui()   { v_dst = v_imm  << 0x10; }
        void execute_slt()   { v_dst = v_src2 < v_src1; };
        void execute_sltu()  { v_dst = v_src2 < v_src1; };
        void execute_slti()  { v_dst = v_src2 < instr.asI.imm; };
        void execute_sltiu() { v_dst = v_src2 < instr.asI.imm; };

        void execute_and()   { v_dst = v_src1 & v_src2; }
        void execute_or()    { v_dst = v_src1 | v_src2; }
        void execute_xor()   { v_dst = v_src1 ^ v_src2; }
        void execute_nor()   { v_dst = ~( v_src1 | v_src2); }

        void execute_andi()  { v_dst = v_src1 & v_imm; }
        void execute_ori()   { v_dst = v_src1 | v_imm; }
        void execute_xori()  { v_dst = v_src1 ^ v_imm; }
    
        void execute_movn()  { }
        void execute_movz()  { }

        void execute_beq()
        {
            if (v_src1 == v_src2)
            {
                new_PC += static_cast<int16>(v_imm) << 2;
                _is_jump_taken = true;
            }
        }

        void execute_bne()
        {
            if (v_src1 != v_src2)
            {
                new_PC += static_cast<int16>(v_imm) << 2;
                _is_jump_taken = true;
            }
        }

        void execute_blez()
        {
            if (v_src1 <= 0)
            {
                new_PC += static_cast<int16>(v_imm) << 2;
                _is_jump_taken = true;
            }
        }

        void execute_bgtz()
        {
            if (v_src1 <= v_src2)
            {
                new_PC += static_cast<int16>(v_imm) << 2;
                _is_jump_taken = true;
            }
        }

        void execute_jal()    { _is_jump_taken = true; v_dst = new_PC; new_PC = (PC & 0xF0000000) | (v_imm << 2); };

        void execute_j()      { _is_jump_taken = true; new_PC = (PC & 0xf0000000) | (v_imm << 2); }
        void execute_jr()     { _is_jump_taken = true; new_PC = v_src1; }
        void execute_jalr()   { _is_jump_taken = true; v_dst = new_PC; new_PC = v_src2; };

        void execute_syscall(){ };
        void execute_break()  { };
        void execute_trap()   { };

        void execute_unknown();

        void calculate_load_addr()  { mem_addr = v_src1 + v_imm; }
        void calculate_store_addr() { mem_addr = v_src1 + v_imm; }

        Execute function = &FuncInstr::execute_unknown;
    public:
        uint32 hi = NO_VAL32;
        uint32 lo = NO_VAL32;

        FuncInstr() {} // constructor w/o arguments for ports

        FuncInstr( uint32 bytes, Addr PC = 0,
                   bool predicted_taken = 0,
                   Addr predicted_target = 0);

        const std::string& Dump() const { return disasm; }

        RegNum get_src1_num() const { return src1; }
        RegNum get_src2_num() const { return src2; }
        RegNum get_dst_num()  const { return dst;  }

        /* Checks if instruction can change PC in unusual way. */
        bool isJump() const { return operation == OUT_J_JUMP      ||
                                     operation == OUT_J_JUMP_LINK ||
                                     operation == OUT_R_JUMP      ||
                                     operation == OUT_R_JUMP_LINK ||
                                     operation == OUT_I_BRANCH; }
        bool is_jump_taken() const { return  _is_jump_taken; }
        bool is_misprediction() const { return predicted_taken != is_jump_taken() || predicted_target != new_PC; }
        bool is_load()  const { return operation == OUT_I_LOAD || operation == OUT_I_LOADU; }
        bool is_store() const { return operation == OUT_I_STORE; }

        void set_v_src1(uint32 value) { v_src1 = value; }
        void set_v_src2(uint32 value) { v_src2 = value; }

        uint32 get_v_dst() const { return v_dst; }

        Addr get_mem_addr() const { return mem_addr; }
        uint32 get_mem_size() const { return mem_size; }
        Addr get_new_PC() const { return new_PC; }
        Addr get_PC() const { return PC; }

        void set_v_dst(uint32 value)  { v_dst  = value; } // for loads
        uint32 get_v_src2() const { return v_src2; } // for stores

        void execute();
};

static inline std::ostream& operator<<( std::ostream& out, const FuncInstr& instr)
{
    return out << instr.Dump();
}

#endif //FUNC_INSTR_H
