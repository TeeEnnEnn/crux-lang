# Operators

Operators are listed from higher to lower precedence. Parentheses override precedence.

| Group | Operators | Notes |
|---|---|---|
| Cast | `as` | explicit checked coercion |
| Postfix | `()` `[]` `.` `::` `?` | call, index, member/static access, unwrap |
| Unary | `-` `not` `~` | numeric negation, logical not, bitwise not |
| Multiplicative | `*` `/` `\` `%` `**` | multiply, float division, integer division, remainder, power |
| Additive | `+` `-` | numeric arithmetic; supported object arithmetic is type-directed |
| Shift | `<<` `>>` | integer shifts |
| Membership | `in` | collection membership |
| Comparison | `<` `<=` `>` `>=` | ordering |
| Equality | `==` `!=` | value equality where implemented |
| Bitwise AND | `&` | integer bitwise and |
| Bitwise XOR | `^` | integer bitwise exclusive or |
| Bitwise OR | `|` | integer bitwise or |
| Logical AND | `and` | short-circuit conjunction |
| Logical OR | `or` | short-circuit disjunction |
| Assignment | `=` `+=` `-=` `*=` `/=` `\=` `%=` | right-associative statement expression |

## Arithmetic

`Int` operands remain integers where an integer operation exists. Mixing `Int` and
`Float` promotes the operation to `Float`. `/` performs numeric division; `\` performs
integer division. `**` currently has the same precedence as the other multiplicative
operators and associates left-to-right. Division and remainder by zero fail at runtime.

Vectors, matrices, and complex values support selected arithmetic operators in addition
to their named methods. Dimensions must be compatible.

## Equality and ordering

Primitive values compare by value. Arrays, tables, tuples, and other objects do not all
share one deep-equality rule; use a type's documented `equals()` method when provided.
Ordering is intended for compatible numeric or string operands.

## Logical operators and truth

`not`, `and`, and `or` are keywords. `and` and `or` short-circuit, so the right operand is
evaluated only when needed. Conditions are type checked as `Bool`; Crux does not define a
general truthiness conversion.

## Membership

`value in collection` is supported for iterable or collection types with membership
semantics. For explicit and fallible lookup, prefer methods such as `contains()` or
`has_key()`.

## Assignment

Compound assignment is available for locals, globals, captured variables, struct fields,
and indexed values where the corresponding binary operation is supported.

See [Types](types.md), [Control Flow](control-flow.md), and the per-type
[standard-library reference](../stdlib/README.md).
