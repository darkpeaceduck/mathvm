#pragma once

#include <cstdio>
#include <stdint.h>

#include <deque>

#include <sys/mman.h>
#include <string.h>

#include "mathvm.h"
#include "x86.h"

// ================================================================================

namespace mathvm {
  class X86Code : public Bytecode {

    void op_rr(uint8_t op, char r1, char r2) {
      add(x86_rex(r1, r2, 1));
      add(op);
      add(x86_modrm(MOD_RR, r1, r2));
    }

    void op_rr2(uint16_t op, char r1, char r2) {
      add(x86_rex(r1, r2, 1));
      addUInt16(op);
      add(x86_modrm(MOD_RR, r1, r2));
    }

    void op_r(uint8_t op, char r, bool opt_rex = false, char w = 1) {
      if ((opt_rex && r & EX_BITMASK) || !opt_rex) {
        add(x86_rex(0, r, w));
      } 
      add(op + (r & ~EX_BITMASK));
    }

  public:
    ~X86Code() {
      munmap(_x86code, _size);
    }

    void done() {
      _size = _data.size();
      _x86code = mmap(NULL, _size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
      //perror("Error");
      memcpy(_x86code, &_data[0], _size);
      mprotect(_x86code, _size, PROT_READ | PROT_EXEC);
      _data.clear();
    }

    void* x86code() {
      return _x86code;
    }

    void mov_rr(char dst, char src) {
      op_rr(MOV_R_RM, dst, src);
    }

    void mov_rm(char dst, char base, int32_t offset) {
      add(x86_rex(dst, base, 1));
      add(MOV_R_RM);
      add(x86_modrm(MOD_RM_D32, dst, base));
      addInt32(offset);
    }

    void mov_mr(char src, char base, uint32_t offset) {
      add(x86_rex(src, base, 1));
      add(MOV_RM_R);
      add(x86_modrm(MOD_RM_D32, src, base));
      addInt32(offset);
    }

    void mov_r_imm(char r, uint64_t val) {
      op_r(MOV_R_IMM, r);
      addInt64(val);
    }

    void add_rr(char dst, char src) {
      op_rr(ADD_R_RM, dst, src);
    }

    void sub_rr(char dst, char src) {
      op_rr(SUB_R_RM, dst, src);
    }

    void mul_rr(char dst, char src) {
      op_rr2(IMUL_R_RM, dst, src);
    }

    void push_r(char r) {
      op_r(PUSH_R, r, true, 1);
    }

    void pop_r(char r) {
      op_r(POP_R, r, true, 1);
    }

    void sub_rm_imm(char r, int32_t v) {
      op_rr(SUB_RM_IMM, 5, r);
      addInt32(v);
    }

    void add_rm_imm(char r, int32_t v) {
      op_rr(ADD_RM_IMM, 0, r);
      addInt32(v);
    }

    void call_r(char r) {
      //add(x86_rex(2, r, 1));
      add(CALL_RM);
      add(x86_modrm(MOD_RR, 2, r));
    }

  private:
    void* _x86code;
    size_t _size;
  };

  typedef X86Code NativeCode;

  // --------------------------------------------------------------------------------

  class NativeFunction : public TranslatedFunction {
  public:
    NativeFunction(AstFunction* node) 
      : TranslatedFunction(node) { }

    NativeCode* code() {
      return &_code;
    }

    void setFirstArg(uint16_t idx) {
      _firstArg = idx;
    }

    void setFirstLocal(uint16_t idx) {
      _firstLocal = idx;
    }

    uint16_t firstLocal() const {
      return _firstLocal;
    }

    uint16_t firstArg() const {
      return _firstArg;
    }

    void disassemble(std::ostream& out) const { }

  private:
    uint16_t _firstArg;
    uint16_t _firstLocal;
    VarType _retType;    

    NativeCode _code;
  };


  class Runtime : public Code {
    typedef std::vector<std::string> Strings;

  public:
    NativeFunction* createFunction(AstFunction* fNode);

    Status* execute(std::vector<mathvm::Var*, std::allocator<Var*> >& args);

    const char* addString(const std::string& s) {
      _strings.push_back(s);
      return _strings.back().c_str();
    }

  private:
    std::deque<NativeFunction*> _functions;
    Strings _strings;
  };
}