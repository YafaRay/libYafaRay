# Formatting
* use tabs, not spaces
* recommended tab width is 4
* 80 character terminals are from last century...
  everyone should be able to view at least 120 character long lines

# Naming conventions
* Namespaces in "lowercase". For example, "yafaray", "yafaray4", etc.
* Defines/Macros in "SCREAMING_SNAKE_CASE"
* Class, Struct, Union and Enum names in "PascalCase" (do not use the old style "_t" suffix)
* Enumerators (within Enums) in "PascalCase"
* Typedef in "PascalCase" with "_t" suffix
* Function parameters in "snake_case"
* Local variables in "snake_case"
* Class / Struct member Functions in "camelCase"
* Class / Struct member variable in "snake_case" with the "_" (underscore) suffix. For example "var_"

# Automatic C++ styling
* Recommended automatic styling using AStyle with the following
  parameters, executed from the root folder of the project:
  
astyle -r --mode=c --style=allman --indent-classes --indent=tab --keep-one-line-blocks --align-pointer=name --align-reference=name --pad-oper --pad-comma --unpad-paren --keep-one-line-statements --keep-one-line-blocks --indent-switches --close-templates --indent-col1-comments --suffix=none *.cc *.h

# Coding conventions
* Codebase is based on the C++11 standard functionality and requires compatible compilers.
* Avoid using Macros as much as possible. For constants, use preferably constexpr. If possible encapsulate them within the class, as static if needed.
* Avoid using typedefs as much as possible. If special plain old data are needed like unsigned char 8 bit, use standard ones like uint8_t, for example and not "yByte" or the like
* Use verbose class, function and variable names. For example avoid short names like "with_a" and use a verbose "with_alpha"
* Do NOT use comments to explain obscure code. Instead write self-explanatory code that is easily understood and verbose enough! Reduce comments to the minimum possible!
* Use Doxygen comments to document non-obvious types, classes, interfaces and functions and their usage.
* NEVER use underscore prefix (for example "_var") because underscore prefixes are reserved for the language keywords!
* Try to reduce states as much as possible, if a static class function can do the job in a "pure functional mode", better.
* Avoid "free" (global) functions as much as possible, embed them in classes, as static functions if possible to provide them with context and better meaning.
* Avoid global and static variables as much as possible. For global constants use constexpr and better if possible embed them in classes as static
* Avoid global free functions as much as possible. Wrap them within namespaces. For example don't use something like fSin_global(float x, float y) but a namespaced math::sin(float x, float y)
* Try using templating for generic functions as long as it does not cause performance problems
* Do NOT use defines as a replacement for Enums!
* Avoid global Enums as much as possible. Embed them within classes.
* Avoid using directly Enums as simple global labels for simple variables of type int, etc. Use Enum class (name) : (type), and provide any variable using enums with the Enum class name, and provide methods to handle operators like &, etc.
* Avoid passing too many parameters to functions, if needed create a new Class or Struct and pass it.
* Always try to add const (or better constexpr) to everything unless it really needs to change its internal state.
* Always try to pass parameters by reference (&) unless it's a plain old data ("POD") like bool, float, int, etc.
* Avoid manual deletes! Use smart pointers to manage object lifetime/ownership. Use std::unique_ptr when possible, and only use shared_ptr when there is actual shared ownership as it has runtime/multithread overheads.
* Pass by reference (or raw pointer if the reference can be null) when there is no transfer of ownership. Use const references/raw pointers whenever possible.
* Try to avoid "c-style" casts like float x = (float) integer; Use float x = static_cast<float>(integer) instead when possible.
* Declare const everything you can, unless it needs to be non-const.
* Keep multithreading in mind, use mutexes when needed, but avoid using them directly and use them with lock_guard if possible.
* Try to embed all OS-specific code in dedicated classes so all #IFDEF, etc, are in that class and not contaminating the entire code.
* Try to avoid "spaghetti code" and create more smaller classes with good defined relationships
* Avoid adding too many dependencies to headers, use classes and enum:type forward declarations whenever possible.
* Avoid using "using" (except for yafaray namespace itself), for example write std::cout and not just cout. That way it's clear when we are using external libraries.
* All struct/class internal variables are to be named with trailing underscore (var_). It's ugly, and should be, because directly accessing class/struct variables should be clearly visible (and discouraged as much as possible). From a design point of view it's better to use inline getters/setters instead, as it allows more flexibility in future changes to the class internals.
* All "free" (global) functions and all global variables are to be named with a trailing underscore followed by "global" (for example: function_global()). It's really ugly, and also should be because we should avoid using them as much as possible, using classes variables and functions (even if static), instead.
* Avoid singletons and static/global variables as much as possible, avoid unexpected hidden dependencies!
* For classes, use the keyword "final" when no further derived classes are expected.
* Use templated functions and classes when necesary to reduce code repetition as much as possible
* Avoid new, delete, alloc, malloc, etc, as much as possible and use the more intelligent standard libraries and containers instead.
* Don't optimize prematurely. Write well designed quality code and profile before using any dirty tricks. Many "dirty tricks" make the code buggy and fragile and do not necessarily improve performance significantly!

