# Token Reference

The scanner's `CruxTokenType` enum in `src/_headers/scanner.h` is the implementation
inventory. This page groups those tokens by their language role.

## Delimiters and operators

`(` `)` `{` `}` `[` `]` `$[` `,` `.` `..` `;` `:` `::` `?` `->` `=>`

`-` `+` `/` `\` `*` `**` `%` `!=` `=` `==` `>` `>=` `<` `<=` `<<` `>>` `&`
`^` `|` `~` `+=` `-=` `*=` `/=` `\=` `%=`

## Literals
- `IDENTIFIER` `[A-Za-z_][A-Za-z0-9_]*`
- `STRING` `'"` both `"`, escapes `\n \t \\ \" \'`
- `INT` decimal, `BINARY_INT` `0b`, `HEX_INT` `0x`, `FLOAT` `3.14`
- `STRING_TYPE` `INT_TYPE` etc type tokens

## Keywords `scanner.c:167`
`and` `not` `else` `false` `for` `fn` `if` `nil` `or` `return` `true` `var` `while` `break` `continue` `use` `from` `pub` `as` `match` `Ok` `Err` `None` `Some` `default` `give` `typeof` `new` `panic` `struct` `shape` `impl` `type` `in` `native` `static`

## Type keywords `compiler_helpers.c:54`
`Nil` `Bool` `Int` `Float` `String` `Array` `Table` `Error` `Result` `Random` `File` `Struct` `Vector` `Complex` `Matrix` `Tuple` `Buffer` `Range` `Any` `Never` `Iterator` `Option`

The scanner also emits internal `ERROR` and `EOF` tokens.

## Notes

- Cross-linked from [docs/README.md](../README.md) and [`src/README.md`](../../src/README.md).
