grammar = """Program → StmtList

StmtList → Stmt StmtList
         | ε

Stmt → Decl ;
     | Assign ;
     | ForLoop
     | PrintStmt ;
     | NewlineStmt ;

Decl → let ID = Expr

Assign → ID = Expr

ForLoop → for ID = Expr to Expr { StmtList }

PrintStmt → print Expr

NewlineStmt → newline

Expr → Term
     | Term + Expr
     | Term - Expr

Term → Factor
     | Factor * Term
     | Factor / Term
     | Factor % Term

Factor → NUMBER
       | ID
       | ( Expr )
"""
