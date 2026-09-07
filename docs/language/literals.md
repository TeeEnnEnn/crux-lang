# Literals

## Primitive literals

```crux
var nothing = nil;
var enabled = true;
var disabled = false;
var decimal = 42;
var binary = 0b101010;
var hexadecimal = 0x2a;
var fraction = 3.5;
var text = "Crux";
var other_text = 'also Crux';
```

Integers are signed 32-bit values. Floats use double precision. Scientific notation is
not currently accepted. Both string quote styles create a `String`; supported escapes
include `\n`, `\t`, `\\`, `\"`, and `\'`.

## Collection literals

```crux
var values: Array[Int] = [1, 2, 3];
var lookup: Table[String, Int] = {"one": 1, "two": 2};
var pair: Tuple[Int, String] = $[1, "one"];
```

Array elements must be compatible with the inferred or annotated element type. Table keys
must be hashable: `Nil`, `Bool`, `Int`, `Float`, or `String`.

## Ranges

```crux
var forward = 0..5;       // 0, 1, 2, 3, 4
var stepped = 0..2..10;   // 0, 2, 4, 6, 8
```

The end is exclusive. In the three-part form, the middle expression is the step. A step
must not be zero. See [Ranges and slices](slices.md).

## Result and option constructors

```crux
var success: Result[Int] = Ok(42);
var failure: Result[Int] = Err(error("not available"));
var present: Option[Int] = Some(42);
var absent: Option[Int] = None;
```

`Ok`, `Err`, `Some`, and `None` are language forms used by matching and `?`; they are not
ordinary imported constructors.

See [Types](types.md) and [Syntax](syntax.md).
