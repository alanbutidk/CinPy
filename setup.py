import sys
from setuptools import setup, Extension, find_packages
from pathlib import Path

cinpy_core = Extension(
    "cinpy_core",
    sources=[
        "src/cinpy_core.c",
        "src/clang_bridge.c",
        "src/ir.c",
        "src/parser.c",
        "src/emitter.c",
        "src/loader.c",
        "src/cache.c",
        "src/marshal.c",
        "src/call.c",
    ],
    include_dirs=[
        "src",
        "ClangHeaders",
    ],
    libraries=["ffi"],
    extra_compile_args=["-std=c11", "-O2", "-Wall"],
)

clanglib_files = []
if sys.platform == "win32":
    clanglib_files = [
        (
            "ClangLib/ClangDLLs",
            [
                "ClangLib/ClangDLLs/libclang.dll",
                "ClangLib/ClangDLLs/libclang-cpp.dll",
                "ClangLib/ClangDLLs/libffi-8.dll",
            ],
        ),
    ]
elif sys.platform == "darwin":
    clanglib_files = [
        ("ClangLib/ClangDYLibs", []),
    ]
else:
    clanglib_files = [
        ("ClangLib/ClangSOs", []),
    ]

clang_headers = []
for p in Path("ClangHeaders").rglob("*"):
    if p.is_file():
        clang_headers.append(str(p))

setup(
    name="cinpy",
    version="0.1.0",
    description="Call C functions from Python with near-zero overhead",
    long_description=open("README.md").read(),
    long_description_content_type="text/markdown",
    license="GPL-3.0-or-later",
    packages=find_packages(),
    ext_modules=[cinpy_core],
    data_files=clanglib_files
    + [
        (
            "ClangHeaders/clang-c",
            [str(p) for p in Path("ClangHeaders/clang-c").glob("*.h")],
        ),
        (
            "ClangHeaders/clang",
            [str(p) for p in Path("ClangHeaders/clang").glob("*.h")]
            if Path("ClangHeaders/clang").exists()
            else [],
        ),
    ],
    python_requires=">=3.12",
    classifiers=[
        "Programming Language :: Python :: 3",
        "Programming Language :: C",
        "License :: OSI Approved :: GNU General Public License v3 or later (GPLv3+)",
        "Operating System :: OS Independent",
    ],
)
