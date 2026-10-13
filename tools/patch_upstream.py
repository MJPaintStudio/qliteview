"""Apply a byte-preserving upstream edit and retain its diff for redistribution."""
from pathlib import Path
import difflib

ROOT = Path(__file__).resolve().parents[1]

def edit(relative, transform):
    path = ROOT / 'liteview' / relative
    before = path.read_bytes()
    after = transform(before)
    if after == before:
        raise ValueError(f'No change: {relative}')
    diff = difflib.diff_bytes(difflib.unified_diff,
        before.splitlines(keepends=True), after.splitlines(keepends=True),
        fromfile=f'a/{relative}'.encode(), tofile=f'b/{relative}'.encode())
    with (ROOT / 'patches/windows-platform.patch').open('ab') as out:
        for line in diff:
            out.write(line)
            if not line.endswith(b'\n'):
                out.write(b'\n\\ No newline at end of file\n')
    path.write_bytes(after)

def replace(data, old, new):
    old = old.replace('\n', '\r\n').encode()
    new = new.replace('\n', '\r\n').encode()
    if old not in data:
        raise ValueError(f'Expected text missing: {old[:100]!r}')
    return data.replace(old, new, 1)
