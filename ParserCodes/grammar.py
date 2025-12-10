grammar = """program → stmt_list

stmt_list → stmt
          | stmt stmt_list

stmt → for_stmt
     | call_stmt
     
for_stmt → "for" ID "=" NUMBER "to" NUMBER ";"

call_stmt → ID "(" args? ")" ";"
args → arg ("," arg)*
arg → NUMBER
arg → STRING
"""
