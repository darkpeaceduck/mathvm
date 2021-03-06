#include <ostream>
#include "astPrinter.h"

namespace mathvm {

    void AstPrinterVisitor::visitBinaryOpNode(BinaryOpNode *node) {
        _os << std::string(_curOffset, ' ') << "(";
        size_t temp = _curOffset;
        _curOffset = 0;
        node->left()->visit(this);
        _os << " " << tokenOp(node->kind()) << " ";
        node->right()->visit(this);
        _os << ")";
        _curOffset = temp;
    }

    void AstPrinterVisitor::visitUnaryOpNode(UnaryOpNode *node) {
        _os << std::string(_curOffset, ' ') << tokenOp(node->kind());
        size_t temp = _curOffset;
        _curOffset = 0;
        node->operand()->visit(this);
        _curOffset = temp;
    }

    void AstPrinterVisitor::visitStringLiteralNode(StringLiteralNode *node) {
        std::string rawStr;
        auto escape = [](char ch) {
            switch (ch) {
                case '\n': return "\\n";
                case '\r': return "\\r";
                case '\\': return "\\\\";
                case '\t': return "\\t";
                case '\f': return "\\f";
                case '\v': return "\\v";
                default: return "";
            }
        };
        for (auto ch : node->literal()) {
            if (isprint(ch)) {
                rawStr.push_back(ch);
            } else {
                rawStr.append(escape(ch));
            }
        }
        _os << std::string(_curOffset, ' ') << '\'' << rawStr << '\'';
    }

    void AstPrinterVisitor::visitIntLiteralNode(IntLiteralNode *node) {
        _os << std::string(_curOffset, ' ') << node->literal();
    }

    void AstPrinterVisitor::visitDoubleLiteralNode(DoubleLiteralNode *node) {
        _os << std::string(_curOffset, ' ') << node->literal();
    }

    void AstPrinterVisitor::visitLoadNode(LoadNode *node) {
        _os << std::string(_curOffset, ' ') << node->var()->name();
    }

    void AstPrinterVisitor::visitStoreNode(StoreNode *node) {
        _os << std::string(_curOffset, ' ') << node->var()->name()
            << " " << tokenOp(node->op()) << " ";
        size_t temp = _curOffset;
        _curOffset = 0;
        node->value()->visit(this);
        _curOffset = temp;
    }

    void AstPrinterVisitor::visitBlockNode(BlockNode *node) {
        visitScope(node->scope());
        for (size_t i = 0; i < node->nodes(); ++i) {
            node->nodeAt(i)->visit(this);
            if (checkForSemicolon(node->nodeAt(i))) {
                _os << ";";
            }
            _os << std::endl;
        }
    }

    void AstPrinterVisitor::visitNativeCallNode(NativeCallNode *node) {
        _os << std::string(_curOffset, ' ') << "native \'" << node->nativeName() << '\'';
    }

    void AstPrinterVisitor::visitForNode(ForNode *node) {
        _os << std::string(_curOffset, ' ') << "for (";
        size_t temp = _curOffset;
        _curOffset = 0;
        _os << node->var()->name() << " in ";
        node->inExpr()->visit(this);
        _os << ") {" << std::endl;
        _curOffset = temp + _offset;
        node->body()->visit(this);
        _curOffset -= _offset;
        _os << std::string(_curOffset, ' ') << "}";
    }

    void AstPrinterVisitor::visitWhileNode(WhileNode *node) {
        _os << std::string(_curOffset, ' ') << "while (";
        size_t temp = _curOffset;
        _curOffset = 0;
        node->whileExpr()->visit(this);
        _os << ") {" << std::endl;
        _curOffset = temp + _offset;
        node->loopBlock()->visit(this);
        _curOffset -= _offset;
        _os << std::string(_curOffset, ' ') << "}";
    }

    void AstPrinterVisitor::visitIfNode(IfNode *node) {
        _os << std::string(_curOffset, ' ') << "if (";
        size_t temp = _curOffset;
        _curOffset = 0;
        node->ifExpr()->visit(this);
        _os << ") {" << std::endl;
        _curOffset = temp + _offset;
        node->thenBlock()->visit(this);
        _curOffset -= _offset;
        if (node->elseBlock() != nullptr) {
            _os << std::string(_curOffset, ' ') << "} else {" << std::endl;
            _curOffset += _offset;
            node->elseBlock()->visit(this);
            _curOffset -= _offset;
        }
        _os << std::string(_curOffset, ' ') << "}";
    }

    void AstPrinterVisitor::visitReturnNode(ReturnNode *node) {
        _os << std::string(_curOffset, ' ') << "return ";
        size_t temp = _curOffset;
        _curOffset = 0;
        if (node->returnExpr() != nullptr) {
            node->returnExpr()->visit(this);
        }
        _curOffset = temp;
    }

    void AstPrinterVisitor::visitFunctionNode(FunctionNode *node) {
        _os << std::string(_curOffset, ' ') << "function "
            << typeToName(node->returnType())
            << " " << node->name() << "(";
        for (size_t i = 0; i < node->parametersNumber(); ++i) {
            _os << typeToName(node->parameterType(i))
                << " " << node->parameterName(i);
            if (i != node->parametersNumber() - 1) {
                _os << ", ";
            }
        }
        _os << ") ";
        if (node->body()->nodes() == 2
            && node->body()->nodeAt(0)->isNativeCallNode()) {
            size_t temp = _curOffset;
            _curOffset = 0;
            node->body()->nodeAt(0)->visit(this);
            _os << ";";
            _curOffset = temp;
            return;
        }
        _os << "{" << std::endl;
        _curOffset += _offset;
        node->body()->visit(this);
        _curOffset -= _offset;
        _os << std::string(_curOffset, ' ') << "}";
    }

    void AstPrinterVisitor::visitCallNode(CallNode *node) {
        _os << std::string(_curOffset, ' ') << node->name() << "(";
        size_t temp = _curOffset;
        _curOffset = 0;
        for (size_t i = 0; i < node->parametersNumber(); ++i) {
            node->parameterAt(i)->visit(this);
            if (i != node->parametersNumber() - 1) {
                _os << ", ";
            }
        }
        _os << ")";
        _curOffset = temp;
    }

    void AstPrinterVisitor::visitPrintNode(PrintNode *node) {
        _os << std::string(_curOffset, ' ') << "print(";
        size_t temp = _curOffset;
        _curOffset = 0;
        for (size_t i = 0; i < node->operands(); ++i) {
            node->operandAt(i)->visit(this);
            if (i != node->operands() - 1) {
                _os << ", ";
            }
        }
        _os << ")";
        _curOffset = temp;
    }

    void AstPrinterVisitor::visitScope(Scope* scope) {
        Scope::VarIterator varIter{scope};
        while (varIter.hasNext()) {
            visitAstVar(varIter.next());
            _os << ";" << std::endl;
        }

        Scope::FunctionIterator funcIter{scope};
        while (funcIter.hasNext()) {
            visitAstFunction(funcIter.next());
            _os << std::endl;
        }
    }

    void AstPrinterVisitor::visitAstVar(const AstVar *var) {
        _os << std::string(_curOffset, ' ') << typeToName(var->type())
            << " " << var->name();
    }

    void AstPrinterVisitor::visitAstFunction(AstFunction* func) {
        func->node()->visit(this);
    }

    bool AstPrinterVisitor::checkForSemicolon(AstNode* node) {
        return !(node->isIfNode()
            || node->isWhileNode()
            || node->isForNode()
            || node->isFunctionNode());
    }

    AstPrinterVisitor::AstPrinterVisitor(std::ostream &os)
        : _os(os)
        , _offset(4)
        , _curOffset(0)
    { }

    AstPrinterVisitor::~AstPrinterVisitor() {}
}
