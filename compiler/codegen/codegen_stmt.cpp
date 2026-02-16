#include "codegen_impl.h"

namespace aym {

void CodeGenImpl::emitStmt(const Stmt *stmt,
                           const std::unordered_map<std::string,int> *locals,
                           const std::string &endLabel) {
    if (!stmt) return;
    if (emitStmtBasic(stmt, locals, endLabel)) return;
    if (emitStmtControl(stmt, locals, endLabel)) return;
    if (emitStmtException(stmt, locals, endLabel)) return;
}

} // namespace aym
