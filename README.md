# CinPy

Call C functions from Python with near-zero overhead.

CinPy uses libclang to parse C headers at import time (or ahead-of-time via `.cpyc` files), then dispatches calls directly through libffi, bypassing CPython's ctypes and cffi overhead entirely.

## Install

```
pip install cinpy
```

Linux requires libclang:
```
sudo apt install libclang-dev
```

macOS requires LLVM:
```
brew install llvm
```

## Usage

```python
from cinpy import LoadHeader, COnly

stdioh = LoadHeader("stdio.h")
stdioh.puts("Hello from C!")

stdioh = LoadHeader("stdio.h", aot=True)
stdioh.puts("AOT parsed and cached to .cpyc")

x = COnly.int("x", 42)
stdioh.puts(f"x = {x.value()}")
```

## AOT caching

`aot=True` forces a full libclang parse and writes a `.cpyc` file to `__cpyc__/`. Subsequent calls to `LoadHeader` without `aot=True` load from the cache instantly.

## COnly types

```python
COnly.int        COnly.uint       COnly.long
COnly.float      COnly.double     COnly.char
COnly.charptr    COnly.voidptr    COnly.size_t
COnly.struct     COnly.enum       COnly.array
```

## LoadLibrary

```python
from cinpy import LoadLibrary

mylib = LoadLibrary("mylib.dll")
mylib.my_function(1, 2, 3)
```

## License

GPL3, Build anything but keep open-source...
