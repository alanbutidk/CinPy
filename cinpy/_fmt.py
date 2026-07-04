import re
import string

_FIELD_RE = re.compile(r"\{(\w+)\}")


def rewrite_fmt_args(fn_name: str, args: tuple) -> tuple:
    if not args:
        return args

    first = args[0]
    if not isinstance(first, str):
        return args

    fields = _FIELD_RE.findall(first)
    if not fields:
        return args

    fmt_str = _FIELD_RE.sub("%s", first)

    positional = []
    remaining  = list(args[1:])

    for field in fields:
        matched = False
        for i, a in enumerate(remaining):
            positional.append(str(a))
            remaining.pop(i)
            matched = True
            break
        if not matched:
            positional.append(f"{{{field}}}")

    return (fmt_str, *positional, *remaining)
