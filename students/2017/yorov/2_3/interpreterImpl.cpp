#include "interpreter.h"
#include "mathvm.h"
#include "ast.h"
#include "asmjit/base/globals.h"
#include <cmath>

namespace mathvm {

    extern "C" {
        int nativeInt(void*, void*, const void*);
        char* nativeString(void*, void*, const void*);
        double nativeDouble(void*, void*, const void*);
    }

    double Interpreter::epsilon = 1e-11;

    Interpreter::Interpreter(std::ostream& os)
        : _os(os)
    {}

    Status* Interpreter::execute(std::vector<Var*>& vars) {
        try {
            executeFunction(static_cast<BytecodeFunction*>(functionByName(AstFunction::top_name)));
        } catch(const std::exception& e) {
            return Status::Error(e.what());
        }
        return Status::Ok();
    }

    void Interpreter::executeFunction(BytecodeFunction* function) {
        std::stack<std::pair<uint32_t, BytecodeFunction*>> scope;
        scope.push(std::make_pair(0, function));
        _variables.pushScope(function->id(), function->localsNumber());

        while (!scope.empty()) {
            auto p = scope.top();
            scope.pop();
            BytecodeFunction* func = p.second;
            Bytecode* bytecode = func->bytecode();
            uint32_t pointer = p.first;

            while (pointer < bytecode->length()) {
                Instruction insn = bytecode->getInsn(pointer);
                ++pointer;
                switch (insn) {
                    case BC_DLOAD:
                        _stack.pushDouble(bytecode->getDouble(pointer));
                        pointer += sizeof(double);
                        break;
                    case BC_ILOAD:
                        _stack.pushInt(bytecode->getInt64(pointer));
                        pointer += sizeof(int64_t);
                        break;
                    case BC_SLOAD:
                        _stack.pushString(constantById(bytecode->getUInt16(pointer)).data());
                        pointer += sizeof(uint16_t);
                        break;
                    case BC_DLOAD0:
                        _stack.pushDouble(0.);
                        break;
                    case BC_ILOAD0:
                        _stack.pushInt(0);
                        break;
                    case BC_SLOAD0:
                        _stack.pushString(std::string("").data());
                        break;
                    case BC_DLOAD1:
                        _stack.pushDouble(1.);
                        break;
                    case BC_ILOAD1:
                        _stack.pushInt(1);
                        break;
                    case BC_DLOADM1:
                        _stack.pushDouble(-1.);
                        break;
                    case BC_ILOADM1:
                        _stack.pushInt(-1);
                        break;
                    case BC_DADD: {
                        double upper = _stack.popDouble();
                        double lower = _stack.popDouble();
                        _stack.pushDouble(lower + upper);
                        break;
                    }
                    case BC_IADD: {
                        int64_t upper = _stack.popInt();
                        int64_t lower = _stack.popInt();
                        _stack.pushInt(lower + upper);
                        break;
                    }
                    case BC_DSUB: {
                        double upper = _stack.popDouble();
                        double lower = _stack.popDouble();
                        _stack.pushDouble(lower - upper);
                        break;
                    }
                    case BC_ISUB: {
                        int64_t upper = _stack.popInt();
                        int64_t lower = _stack.popInt();
                        _stack.pushInt(lower - upper);
                        break;
                    }
                    case BC_DMUL: {
                        double upper = _stack.popDouble();
                        double lower = _stack.popDouble();
                        _stack.pushDouble(upper * lower);
                        break;
                    }
                    case BC_IMUL: {
                        int64_t upper = _stack.popInt();
                        int64_t lower = _stack.popInt();
                        _stack.pushInt(upper * lower);
                        break;
                    }
                    case BC_DDIV: {
                        double upper = _stack.popDouble();
                        double lower = _stack.popDouble();
                        _stack.pushDouble(lower / upper);
                        break;
                    }
                    case BC_IDIV: {
                        int64_t upper = _stack.popInt();
                        int64_t lower = _stack.popInt();
                        _stack.pushInt(lower / upper);
                        break;
                    }
                    case BC_IMOD: {
                        int64_t upper = _stack.popInt();
                        int64_t lower = _stack.popInt();
                        _stack.pushInt(lower % upper);
                        break;
                    }
                    case BC_DNEG:
                        _stack.pushDouble(-1 * _stack.popDouble());
                        break;
                    case BC_INEG:
                        _stack.pushInt(-1 * _stack.popInt());
                        break;
                    case BC_IAOR: {
                        int64_t upper = _stack.popInt();
                        int64_t lower = _stack.popInt();
                        _stack.pushInt(lower | upper);
                        break;
                    }
                    case BC_IAAND: {
                        int64_t upper = _stack.popInt();
                        int64_t lower = _stack.popInt();
                        _stack.pushInt(lower & upper);
                        break;
                    }
                    case BC_IAXOR: {
                        int64_t upper = _stack.popInt();
                        int64_t lower = _stack.popInt();
                        _stack.pushInt(lower ^ upper);
                        break;
                    }
                    case BC_IPRINT:
                        _os << _stack.popInt();
                        break;
                    case BC_DPRINT:
                        _os << _stack.popDouble();
                        break;
                    case BC_SPRINT: {
                        // uint16_t id =
                        _os << _stack.popString();
                        break;
                    }
                    case BC_I2D:
                        _stack.pushDouble(static_cast<double>(_stack.popInt()));
                        break;
                    case BC_D2I:
                        _stack.pushInt(static_cast<int64_t>(_stack.popDouble()));
                        break;
                    case BC_S2I:
                        _stack.pushInt(strlen(_stack.popString()) != 0);
                        break;
                    case BC_SWAP:
                        _stack.swap();
                        break;
                    case BC_POP:
                        _stack.pop();
                        break;
                    case BC_LOADDVAR0:
                        _stack.pushDouble(_variables.getCachedDouble(0));
                        break;
                    case BC_LOADDVAR1:
                        _stack.pushDouble(_variables.getCachedDouble(1));
                        break;
                    case BC_LOADDVAR2:
                        _stack.pushDouble(_variables.getCachedDouble(2));
                        break;
                    case BC_LOADDVAR3:
                        _stack.pushDouble(_variables.getCachedDouble(3));
                        break;
                    case BC_LOADIVAR0:
                        _stack.pushInt(_variables.getCachedInt(0));
                        break;
                    case BC_LOADIVAR1:
                        _stack.pushInt(_variables.getCachedInt(1));
                        break;
                    case BC_LOADIVAR2:
                        _stack.pushInt(_variables.getCachedInt(2));
                        break;
                    case BC_LOADIVAR3:
                        _stack.pushInt(_variables.getCachedInt(3));
                        break;
                    case BC_LOADSVAR0:
                        _stack.pushString(_variables.getString(0));
                        break;
                    case BC_LOADSVAR1:
                        _stack.pushString(_variables.getString(1));
                        break;
                    case BC_LOADSVAR2:
                        _stack.pushString(_variables.getString(2));
                        break;
                    case BC_LOADSVAR3:
                        _stack.pushString(_variables.getString(3));
                        break;
                    case BC_STOREDVAR0:
                        _variables.setCachedDouble(0, _stack.popDouble());
                        break;
                    case BC_STOREDVAR1:
                        _variables.setCachedDouble(1, _stack.popDouble());
                        break;
                    case BC_STOREDVAR2:
                        _variables.setCachedDouble(2, _stack.popDouble());
                        break;
                    case BC_STOREDVAR3:
                        _variables.setCachedDouble(3, _stack.popDouble());
                        break;
                    case BC_STOREIVAR0:
                        _variables.setCachedInt(0, _stack.popInt());
                        break;
                    case BC_STOREIVAR1:
                        _variables.setCachedInt(1, _stack.popInt());
                        break;
                    case BC_STOREIVAR2:
                        _variables.setCachedInt(2, _stack.popInt());
                        break;
                    case BC_STOREIVAR3:
                        _variables.setCachedInt(3, _stack.popInt());
                        break;
                    case BC_STORESVAR0:
                        _variables.setCachedString(0, _stack.popString());
                        break;
                    case BC_STORESVAR1:
                        _variables.setCachedString(1, _stack.popString());
                        break;
                    case BC_STORESVAR2:
                        _variables.setCachedString(2, _stack.popString());
                        break;
                    case BC_STORESVAR3:
                        _variables.setCachedString(3, _stack.popString());
                        break;
                    case BC_LOADDVAR: {
                        _stack.pushDouble(_variables.getDouble(bytecode->getUInt16(pointer)));
                        pointer += sizeof(uint16_t);
                        break;
                    }
                    case BC_LOADIVAR: {
                        _stack.pushInt(_variables.getInt(bytecode->getUInt16(pointer)));
                        pointer += sizeof(uint16_t);
                        break;
                    }
                    case BC_LOADSVAR: {
                        _stack.pushString(_variables.getString(bytecode->getUInt16(pointer)));
                        pointer += sizeof(uint16_t);
                        break;
                    }
                    case BC_STOREDVAR: {
                        _variables.setDouble(bytecode->getUInt16(pointer), _stack.popDouble());
                        pointer += sizeof(uint16_t);
                        break;
                    }
                    case BC_STOREIVAR: {
                        _variables.setInt(bytecode->getUInt16(pointer), _stack.popInt());
                        pointer += sizeof(uint16_t);
                        break;
                    }
                    case BC_STORESVAR: {
                        _variables.setString(bytecode->getUInt16(pointer), _stack.popString());
                        pointer += sizeof(uint16_t);
                        break;
                    }
                    case BC_LOADCTXDVAR: {
                        uint16_t contextId = bytecode->getUInt16(pointer);
                        pointer += sizeof(uint16_t);
                        uint16_t varId = bytecode->getUInt16(pointer);
                        pointer += sizeof(uint16_t);
                        _stack.pushDouble(_variables.getDouble(contextId, varId));
                        break;
                    }
                    case BC_LOADCTXIVAR: {
                        uint16_t contextId = bytecode->getUInt16(pointer);
                        pointer += sizeof(uint16_t);
                        uint16_t varId = bytecode->getUInt16(pointer);
                        pointer += sizeof(uint16_t);
                        _stack.pushInt(_variables.getInt(contextId, varId));
                        break;
                    }
                    case BC_LOADCTXSVAR: {
                        uint16_t contextId = bytecode->getUInt16(pointer);
                        pointer += sizeof(uint16_t);
                        uint16_t varId = bytecode->getUInt16(pointer);
                        pointer += sizeof(uint16_t);
                        _stack.pushString(_variables.getString(contextId, varId));
                        break;
                    }
                    case BC_STORECTXDVAR: {
                        uint16_t contextId = bytecode->getUInt16(pointer);
                        pointer += sizeof(uint16_t);
                        uint16_t varId = bytecode->getUInt16(pointer);
                        pointer += sizeof(uint16_t);
                        _variables.setDouble(contextId, varId, _stack.popDouble());
                        break;
                    }
                    case BC_STORECTXIVAR: {
                        uint16_t contextId = bytecode->getUInt16(pointer);
                        pointer += sizeof(uint16_t);
                        uint16_t varId = bytecode->getUInt16(pointer);
                        pointer += sizeof(uint16_t);
                        _variables.setInt(contextId, varId, _stack.popInt());
                        break;
                    }
                    case BC_STORECTXSVAR: {
                        uint16_t contextId = bytecode->getUInt16(pointer);
                        pointer += sizeof(uint16_t);
                        uint16_t varId = bytecode->getUInt16(pointer);
                        pointer += sizeof(uint16_t);
                        _variables.setString(contextId, varId, _stack.popString());
                        break;
                    }
                    case BC_DCMP: {
                        double upper = _stack.popDouble();
                        double lower = _stack.popDouble();
                        int64_t res = fabs(lower - upper) < epsilon ? 0
                        : (lower - upper < 0 ? -1 : 1);
                        _stack.pushDouble(lower);
                        _stack.pushDouble(upper);
                        _stack.pushInt(res);
                        break;
                    }
                    case BC_ICMP: {
                        int64_t upper = _stack.popInt();
                        int64_t lower = _stack.popInt();
                        int64_t res = (lower - upper);
                        res /= abs(res) ? abs(res) : 1;
                        _stack.pushInt(lower);
                        _stack.pushInt(upper);
                        _stack.pushInt(res);
                        break;
                    }
                    case BC_JA: {
                        int16_t offset = bytecode->getInt16(pointer);
                        pointer += offset;
                        break;
                    }
                    case BC_IFICMPE: {
                        int64_t upper = _stack.popInt();
                        int64_t lower = _stack.popInt();
                        _stack.pushInt(lower);
                        _stack.pushInt(upper);
                        pointer += lower == upper ? bytecode->getInt16(pointer) : sizeof(int16_t);
                        break;
                    }
                    case BC_IFICMPNE: {
                        int64_t upper = _stack.popInt();
                        int64_t lower = _stack.popInt();
                        _stack.pushInt(lower);
                        _stack.pushInt(upper);
                        pointer += lower != upper ? bytecode->getInt16(pointer) : sizeof(int16_t);
                        break;
                    }
                    case BC_IFICMPG: {
                        int64_t upper = _stack.popInt();
                        int64_t lower = _stack.popInt();
                        _stack.pushInt(lower);
                        _stack.pushInt(upper);
                        pointer += lower > upper ? bytecode->getInt16(pointer) : sizeof(int16_t);
                        break;
                    }
                    case BC_IFICMPGE: {
                        int64_t upper = _stack.popInt();
                        int64_t lower = _stack.popInt();
                        _stack.pushInt(lower);
                        _stack.pushInt(upper);
                        pointer += lower >= upper ? bytecode->getInt16(pointer) : sizeof(int16_t);
                        break;
                    }
                    case BC_IFICMPL: {
                        int64_t upper = _stack.popInt();
                        int64_t lower = _stack.popInt();
                        _stack.pushInt(lower);
                        _stack.pushInt(upper);
                        pointer += lower < upper ? bytecode->getInt16(pointer) : sizeof(int16_t);
                        break;
                    }
                    case BC_IFICMPLE: {
                        int64_t upper = _stack.popInt();
                        int64_t lower = _stack.popInt();
                        _stack.pushInt(lower);
                        _stack.pushInt(upper);
                        pointer += lower <= upper ? bytecode->getInt16(pointer) : sizeof(int16_t);
                        break;
                    }
                    case BC_DUMP: {
                        // _os << _stack.pop();
                        break;
                    }
                    case BC_STOP:
                      break;
                    case BC_CALL: {
                        uint16_t functionId = bytecode->getUInt16(pointer);
                        pointer += sizeof(uint16_t);
                        scope.push(std::make_pair(pointer, func));
                        func = static_cast<BytecodeFunction*>(functionById(functionId));
                        bytecode = func->bytecode();
                        pointer = 0;
                        _variables.pushScope(func->id(), func->localsNumber());
                        break;
                    }
                    case BC_CALLNATIVE: {
                        uint16_t nativeId = bytecode->getUInt16(pointer);
                        pointer += sizeof(uint16_t);
                        const std::string* name;
                        const Signature* signature;
                        const void* nativeCode = nativeById(nativeId, &signature, &name);
                        intptr_t iReg[6]{};
                        size_t ip = 0;
                        double dReg[6]{};
                        size_t dp = 0;

                        for (int i = signature->size() - 1; i > 0; --i) {
                            switch (signature->at(i).first) {
                                case VT_DOUBLE:
                                    dReg[dp++] = _stack.popDouble();
                                    break;
                                case VT_INT:
                                    iReg[ip++] = _stack.popInt();
                                    break;
                                case VT_STRING:
                                    iReg[ip++] = (intptr_t) _stack.popString();
                                    break;
                                default:
                                    throw std::logic_error("unexpected arg type");
                            }
                        }

                        switch (signature->at(0).first) {
                            case VT_DOUBLE:
                                _stack.pushDouble(nativeDouble(dReg, iReg, nativeCode));
                                break;
                            case VT_INT:
                                _stack.pushInt(nativeInt(dReg, iReg, nativeCode));
                                break;
                            case VT_STRING:
                                _stack.pushString(nativeString(dReg, iReg, nativeCode));
                                break;
                            default: break;
                        }
                        break;
                    }
                    case BC_RETURN:
                        pointer = bytecode->length();
                        break;
                    case BC_BREAK:
                        break;
                    case BC_LAST:
                        throw std::logic_error("unexpected instruction");
                    case BC_INVALID:
                        throw std::logic_error("invalid instruction");
                }
            }
            _variables.popScope();
        }
    }
}
