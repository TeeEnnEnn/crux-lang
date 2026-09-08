# Syntax

## Single-char `src/scanner.c:584`

`(` `)` `{` `}` `[` `]` `,` `.` `..` `-` `+` `;` `/` `\` `*` `**` `%` `:` `'` `"` `$[`

`$[` starts a tuple literal: `$[1, "one"]`. Ordinary `[` starts an array literal.

## One/two-char

`->` return type, `!=` `=` `==` `>` `>=` `<` `<=` `<<` `>>` `&` `^` `|` `~` `+=` `-=` `*=` `/=` `\=` `%=`, `?` unwrap, `::` static invoke, `:` type annotation, `=>` match arm, `|` union.

## Literals `src/scanner.c:167`

- `IDENTIFIER` `[a-zA-Z_][a-zA-Z0-9_]*`
- `STRING` `'single'` or `"double"` both → `CRUX_TOKEN_STRING`, escapes `\n \t \\ \" \'` (`compiler_core.c:8`)
- `INT` decimal `123`, `BINARY_INT` `0b1010`, `HEX_INT` `0xFF`, `FLOAT` `3.14` (no scientific), e.g. `tests/compiler/basic_types.crux:24`
- Comments `// line` only, no `/* */`. Whitespace ignored.

## Keywords `src/scanner.c:167-330`

`and` `not` `else` `false` `for` `fn` `if` `nil` `or` `return` `true` `var` `while` `break` `continue` `use` `from` `pub` `as` `match` `=>` `Ok` `Err` `None` `Some` `default` `give` `typeof` `new` `panic` `struct` `shape` `impl` `type` `in` `native` `static`

*Negation* is `not` (not `!`), `!` only part of `!=`.

## Type keywords `src/compiler/compiler_helpers.c:54`

`Nil` `Bool` `Int` `Float` `String` `Array` `Table` `Error` `Result` `Random` `File` `Struct` `Vector` `Complex` `Matrix` `Tuple` `Buffer` `Range` `Any` `Never` `Iterator` `Option` + `shap`e.

Example:
```crux
var x: Array[Int] = [1,2];
var t: Table[String,Int] = {"a":1};
var v: Vector[3] = Vector(3,[1.0,2.0,3.0])?;
var r: Result[Int] = Ok(5);
```

## Punctuation Rules

- Statements end with `;` (`consume(CRUX_TOKEN_SEMICOLON)` `compiler_statements.c:5`).
- Blocks `{ }`.
- Type annotation `var x: Int = 5;` `fn f(a:Int)->Bool { }`.
- Generic `Array[Int]`, `Table[String,Int]`, `Vector[3]`, `Matrix[2,3]`, `Tuple[Int,String]`, `Result[Ok]`, `Option[Some]`, `(A,B)->Ret`, `A|B`.

See [Literals](literals.md), [Operators](operators.md), and the
[token reference](../reference/tokens.md).
