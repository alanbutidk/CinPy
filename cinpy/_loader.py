import pathlib

_CPYC_DIR = pathlib.Path.cwd() / "__cpyc__"


def resolve_cpyc(header: str):
    stem = pathlib.Path(header).name.replace(".", "_")
    p = _CPYC_DIR / f"{stem}.cpyc"
    return p if p.exists() else None


def cpyc_path_for(header: str) -> pathlib.Path:
    stem = pathlib.Path(header).name.replace(".", "_")
    _CPYC_DIR.mkdir(exist_ok=True)
    return _CPYC_DIR / f"{stem}.cpyc"
