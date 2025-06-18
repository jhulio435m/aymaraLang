#include "interpreter.h"
#include "../builtins/builtins.h"
#include <iostream>

namespace aym {

void Interpreter::pushScope() {
    scopes.emplace_back();
}

void Interpreter::popScope() {
    if (scopes.size() > 1)
        scopes.pop_back();
}

Value Interpreter::lookup(const std::string &name) {
    for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
        auto f = it->find(name);
        if (f != it->end()) return f->second;
    }
    return Value::Int(0);
}

void Interpreter::assign(const std::string &name, const Value &val) {
    for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
        auto f = it->find(name);
        if (f != it->end()) { f->second = val; return; }
    }
    scopes.front()[name] = val;
}

void Interpreter::declare(const std::string &name, const Value &val) {
    scopes.back()[name] = val;
}

Value Interpreter::eval(Expr *expr) {
    expr->accept(*this);
    return lastValue;
}

Value Interpreter::callFunction(const std::string &name, const std::vector<Value>& args) {
    auto it = functions.find(name);
    if (it != functions.end()) {
        pushScope();
        const auto &params = it->second->getParams();
        size_t idx = 0;
        for (const auto &p : params) {
            if (idx < args.size())
                declare(p, args[idx]);
            else
                declare(p, Value::Int(0));
            ++idx;
        }
        it->second->getBody()->accept(*this);
        popScope();
        returnFlag = false;
        return returnValue;
    }
    if (name == BUILTIN_PRINT) {
        if (!args.empty()) {
            if (args[0].type == Value::Type::String)
                std::cout << args[0].s << std::endl;
            else if (args[0].type == Value::Type::Int)
                std::cout << args[0].i << std::endl;
        }
        return Value::Void();
    }
    if (name == BUILTIN_INPUT) {
        std::string line;
        std::getline(std::cin, line);
        return Value::String(line);
    }
    if (name == BUILTIN_LENGTH) {
        if (!args.empty() && args[0].type == Value::Type::String)
            return Value::Int(args[0].s.size());
        return Value::Int(0);
    }
    return Value::Void();
}

void Interpreter::execute(const std::vector<std::unique_ptr<Node>> &nodes) {
    for (const auto &n : nodes) {
        n->accept(*this);
    }
}

void Interpreter::visit(NumberExpr &n) {
    lastValue = Value::Int(n.getValue());
}

void Interpreter::visit(StringExpr &s) {
    lastValue = Value::String(s.getValue());
}

void Interpreter::visit(VariableExpr &v) {
    lastValue = lookup(v.getName());
}

void Interpreter::visit(BinaryExpr &b) {
    Value l = eval(b.getLeft());
    Value r = eval(b.getRight());
    switch (b.getOp()) {
    case '+':
        if (l.type == Value::Type::String || r.type == Value::Type::String) {
            std::string ls = (l.type == Value::Type::String) ? l.s : std::to_string(l.i);
            std::string rs = (r.type == Value::Type::String) ? r.s : std::to_string(r.i);
            lastValue = Value::String(ls + rs);
        } else {
            lastValue = Value::Int(l.i + r.i);
        }
        break;
    case '-': lastValue = Value::Int(l.i - r.i); break;
    case '*': lastValue = Value::Int(l.i * r.i); break;
    case '/': lastValue = Value::Int(r.i ? l.i / r.i : 0); break;
    case '%': lastValue = Value::Int(r.i ? l.i % r.i : 0); break;
    case '^': {
        long res = 1; for (long i=0;i<r.i;++i) res*=l.i; lastValue = Value::Int(res); break; }
    case '<': lastValue = Value::Bool(l.i < r.i); break;
    case '>': lastValue = Value::Bool(l.i > r.i); break;
    case 'l': lastValue = Value::Bool(l.i <= r.i); break;
    case 'g': lastValue = Value::Bool(l.i >= r.i); break;
    case 's': lastValue = Value::Bool(l.i == r.i); break;
    case 'd': lastValue = Value::Bool(l.i != r.i); break;
    case '&': lastValue = Value::Bool((l.i!=0) && (r.i!=0)); break;
    case '|': lastValue = Value::Bool((l.i!=0) || (r.i!=0)); break;
    default: lastValue = Value::Int(0); break;
    }
}

void Interpreter::visit(UnaryExpr &u) {
    Value v = eval(u.getExpr());
    if (u.getOp()=='!') lastValue = Value::Bool(!(v.i!=0));
    else lastValue = Value::Int(0);
}

void Interpreter::visit(CallExpr &c) {
    std::vector<Value> args;
    for (const auto &a : c.getArgs()) args.push_back(eval(a.get()));
    lastValue = callFunction(c.getName(), args);
}

void Interpreter::visit(PrintStmt &p) {
    Value v = eval(p.getExpr());
    if (v.type == Value::Type::String)
        std::cout << v.s << std::endl;
    else if (v.type == Value::Type::Int)
        std::cout << v.i << std::endl;
}

void Interpreter::visit(ExprStmt &e) {
    lastValue = eval(e.getExpr());
}

void Interpreter::visit(AssignStmt &a) {
    Value v = eval(a.getValue());
    assign(a.getName(), v);
}

void Interpreter::visit(BlockStmt &b) {
    pushScope();
    for (const auto &s : b.statements) {
        s->accept(*this);
        if (breakFlag || continueFlag || returnFlag) break;
    }
    popScope();
}

void Interpreter::visit(IfStmt &i) {
    Value cond = eval(i.getCondition());
    if (cond.i) i.getThen()->accept(*this);
    else if (i.getElse()) i.getElse()->accept(*this);
}

void Interpreter::visit(ForStmt &f) {
    f.getInit()->accept(*this);
    while (true) {
        Value cond = eval(f.getCondition());
        if (!cond.i) break;
        f.getBody()->accept(*this);
        if (breakFlag) { breakFlag=false; break; }
        if (continueFlag) { continueFlag=false; }
        f.getPost()->accept(*this);
        if (returnFlag) break;
    }
}

void Interpreter::visit(BreakStmt &) {
    breakFlag = true;
}

void Interpreter::visit(ContinueStmt &) {
    continueFlag = true;
}

void Interpreter::visit(ReturnStmt &r) {
    if (r.getValue()) returnValue = eval(r.getValue());
    else returnValue = Value::Void();
    returnFlag = true;
}

void Interpreter::visit(VarDeclStmt &v) {
    Value val = Value::Int(0);
    if (v.getInit()) val = eval(v.getInit());
    declare(v.getName(), val);
}

void Interpreter::visit(FunctionStmt &fn) {
    functions[fn.getName()] = &fn;
}

void Interpreter::visit(WhileStmt &w) {
    while (true) {
        Value cond = eval(w.getCondition());
        if (!cond.i) break;
        w.getBody()->accept(*this);
        if (breakFlag) { breakFlag=false; break; }
        if (continueFlag) { continueFlag=false; }
        if (returnFlag) break;
    }
}

void Interpreter::visit(DoWhileStmt &dw) {
    do {
        dw.getBody()->accept(*this);
        if (breakFlag) { breakFlag=false; break; }
        if (continueFlag) { continueFlag=false; }
        if (returnFlag) break;
    } while (eval(dw.getCondition()).i);
}

void Interpreter::visit(SwitchStmt &sw) {
    Value val = eval(sw.getExpr());
    bool matched = false;
    for (const auto &c : sw.getCases()) {
        if (eval(c.first.get()).i == val.i) {
            c.second->accept(*this);
            matched = true; break;
        }
    }
    if (!matched && sw.getDefault()) {
        sw.getDefault()->accept(*this);
    }
}

} // namespace aym
