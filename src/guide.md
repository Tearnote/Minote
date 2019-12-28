# Minote code guidelines
- Document all globals with Doxygen comments
  - All params are `[in]` unless specified otherwise
  - Pointer arguments do not accept `null` unless specified otherwise
  - Use `/**` style for functions and longer explanations, `///` otherwise
- Use `u8` for all string literals
- `assert()` argument preconditions and return postconditions
- Initialize every variable on declaration line or immediately
  afterwards
- When an object is destroyed and its pointer is invalidated, set it to
  `null` immediately
- Most functions should either succeed or never return, unless failure
  is expected
  - Create and use error-checking wrappers for stdlib functions and macros
  in `util.h`
- Init and cleanup functions must be tolerant to being called twice or
  in wrong order. Cleanup must never fail
- Typedef structs and enums
  - Hide struct implementation in `.c` file when possible and provide
    documented interfaces to query the members instead
- Naming guideline:
  - Functions, arguments and variables are camelCase
  - Types and constants are TitleCase
  - Syntax-like constructs are snake_case
  - Compile-time flags are UPPER_SNAKE_CASE
  - Preprocessor definitions follow the guideline of whatever they are
    pretending to be
  - Functions that interface with object type `Foo` should start with
    `foo`
