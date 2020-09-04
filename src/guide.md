# Minote code guidelines
- Document globals with Doxygen comments
  - All params are `[in]` unless specified otherwise
  - Pointer arguments do not accept `null` unless specified otherwise
  - Use `/**` style for functions and longer explanations, `///` otherwise
- Use `u8` for all string literals
- `assert()` argument preconditions and return postconditions
- Initialize every variable on declaration line
- Use the init/cleanup and create/destroy idioms for lifecycle management,
  converting other mechanisms whenever possible
- When an object is destroyed and its pointer is invalidated, set it to `null`
  immediately, even if it is completely redundant at the time
- Most functions should either succeed or never return, unless failure
  is expected
  - Create and use error-checking wrappers for library functions and macros
    in `util.h`
- Init and cleanup functions must be tolerant to being called twice or in wrong
  order. Cleanup must never fail
- Typedef structs and enums
  - Do not hide their definitions. Document member access instead of using
    accessor functions, unless there is a good reason such as providing
    thread-safety
- Naming guideline:
  - Functions, arguments, variables and fundamental types are camelCase
  - Object types and constants are TitleCase
    - Basic compound types and standard structures like containers are lowercase
  - Syntax-like constructs are snake_case
  - Compile-time flags are UPPER_SNAKE_CASE
  - Preprocessor definitions follow the guideline of whatever they are
    pretending to be
  - Functions that interface with object type `Foo` should start with
    `foo` (OOP-like convention)
