#include "specializeParenOpExprs.h"
#include "expr.h"
#include "stmt.h"
#include "stringutil.h"
#include "symtab.h"


static void
decomposeStmtFunction(ParenOpExpr* parenOpExpr, char* newFunctionName) {
  for (Expr* arg = parenOpExpr->argList->first(); arg; arg = parenOpExpr->argList->next()) {
    Variable* function = new Variable(new UnresolvedSymbol(newFunctionName));
    Stmt* replacement = new ExprStmt(new ParenOpExpr(function, new AList<Expr>(arg->copy())));
    parenOpExpr->parentStmt->insertBefore(replacement);
  }
}


void SpecializeParenOpExprs::postProcessStmt(Stmt* stmt) {
  if (ExprStmt* exprStmt = dynamic_cast<ExprStmt*>(stmt)) {
    if (ParenOpExpr* parenOpExpr = dynamic_cast<ParenOpExpr*>(exprStmt->expr)) {
      if (Variable* baseVar = dynamic_cast<Variable*>(parenOpExpr->baseExpr)) {
        if (strcmp(baseVar->var->name, "assert") == 0) {
          BlockStmt* thenStmt = Symboltable::startCompoundStmt();
          Expr* write = new Variable(new UnresolvedSymbol("write"));
          StringLiteral* assertFailed = new StringLiteral("***Error:  Assert "
                                                          "failed***");
          AList<Expr>* errorMessage = new AList<Expr>(assertFailed);
          Expr* callWrite = new ParenOpExpr(write, errorMessage);
          AList<Stmt>* thenBody = new AList<Stmt>(new ExprStmt(callWrite));
          Expr* writeln = new Variable(new UnresolvedSymbol("writeln"));
          Expr* callWriteln = new ParenOpExpr(writeln);
          thenBody->insertAtTail(new ExprStmt(callWriteln));
          Expr* rtexit = new Variable(new UnresolvedSymbol("exit"));
          IntLiteral* exitZero = new IntLiteral("0", 0);
          AList<Expr>* status = new AList<Expr>(exitZero);
          Expr* callRtexit = new ParenOpExpr(rtexit, status);
          thenBody->insertAtTail(new ExprStmt(callRtexit));
          thenStmt = Symboltable::finishCompoundStmt(thenStmt, thenBody);
          int length = parenOpExpr->argList->length();
          if (length != 1) {
            USR_FATAL(parenOpExpr->argList, "Assert takes exactly one "
                      "expression; you've given it %d.", length);
          }
          Expr* test = parenOpExpr->argList->only();
          Expr* notTest = new UnOp(UNOP_LOGNOT, test->copy());
          Stmt* assertIfStmt = new CondStmt(notTest, thenStmt);
          parenOpExpr->parentStmt->insertBefore(assertIfStmt);
          parenOpExpr->parentStmt->remove();          

        } else if (strcmp(baseVar->var->name, "halt") == 0) {
          decomposeStmtFunction(parenOpExpr, "write");
          Expr* writeln = new Variable(new UnresolvedSymbol("writeln"));
          Expr* callWriteln = new ParenOpExpr(writeln);
          parenOpExpr->parentStmt->insertBefore(new ExprStmt(callWriteln));

          Expr* rtexit = new Variable(new UnresolvedSymbol("exit"));
          IntLiteral* exitZero = new IntLiteral("0", 0);
          AList<Expr>* status = new AList<Expr>(exitZero);
          Expr* callRtexit = new ParenOpExpr(rtexit, status);
          parenOpExpr->parentStmt->insertBefore(new ExprStmt(callRtexit));


          parenOpExpr->parentStmt->remove();
        } else if (strcmp(baseVar->var->name, "read") == 0) {
          decomposeStmtFunction(parenOpExpr, "read");
          parenOpExpr->parentStmt->remove();
        } else if (strcmp(baseVar->var->name, "write") == 0) {
          decomposeStmtFunction(parenOpExpr, "write");
          parenOpExpr->parentStmt->remove();
        } else if (strcmp(baseVar->var->name, "writeln") == 0) {
          decomposeStmtFunction(parenOpExpr, "write");
          Expr* writeln = new Variable(new UnresolvedSymbol("writeln"));
          Expr* callWriteln = new ParenOpExpr(writeln);
          parenOpExpr->parentStmt->insertBefore(new ExprStmt(callWriteln));
          parenOpExpr->parentStmt->remove();
        }
      }
    }
  }
}


void SpecializeParenOpExprs::postProcessExpr(Expr* expr) {
  Expr* paren_replacement = NULL;
  if (ParenOpExpr* paren = dynamic_cast<ParenOpExpr*>(expr)) {
    if (dynamic_cast<ArrayType*>(paren->baseExpr->typeInfo())) {
      if (!analyzeAST)
        paren_replacement = new ArrayRef(paren->baseExpr, paren->argList);
    } else if (dynamic_cast<TupleType*>(paren->baseExpr->typeInfo())) {
      if (!analyzeAST)
        paren_replacement = new TupleSelect(paren->baseExpr, paren->argList);
    } else if (Variable* baseVar = dynamic_cast<Variable*>(paren->baseExpr)) {
      if (!dynamic_cast<TypeSymbol*>(baseVar->var) && 
          dynamic_cast<TupleType*>(baseVar->typeInfo())) {
        if (!analyzeAST)
          paren_replacement = new TupleSelect(baseVar, paren->argList);
      } else if (StructuralType* ctype = dynamic_cast<StructuralType*>(baseVar->var->type)) {
        if (dynamic_cast<TypeSymbol*>(baseVar->var)) {
          if (ctype->defaultConstructor) {
            paren_replacement = new ParenOpExpr(new Variable(new UnresolvedSymbol(ctype->defaultConstructor->name)), paren->argList);
          } else {
            INT_FATAL(expr, "constructor does not have a DefStmt");
          }
        }
      } else if (dynamic_cast<FnSymbol*>(baseVar->var)) {
        paren_replacement = new FnCall(baseVar, paren->argList);
      }
    }
  }
  if (paren_replacement) {
    expr->replace(paren_replacement);
  }
}

