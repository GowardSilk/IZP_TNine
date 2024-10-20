# T9 Project

- Supported compilers (tested on push: c12ed61)
    + Windows: clang (version 18.1.8)
    + Windows: Cygwin's gcc (version 13.2.1 20240426)
    + Windows: cl (version 19.41.34120)
- Compile with: 
<code>
gcc -std=c11 -Wall -Wextra -Werror tnine.c -o tnine
</code>
- Or you may use the cmake...
<code>
cmake -B build -S .
</code>

- NOTE: if you want to use a debug version of the application, then define "DEBUG" as a macro either inside the tnine.c itself or as a part of the build tool you use, then '-d' optional parameter can be used to see debug info

## Description
- basic replica of phone lookup search (with T9 algorithm)

## Usage
- launch the compiled binary <code>./tnine [-s(optional)] [t9_keyboard_input(optional)] [-d(optional-debug-only)] <[input_file_name]</code>
