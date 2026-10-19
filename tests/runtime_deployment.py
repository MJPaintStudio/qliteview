"""A missing or invalid app-local core must not fall back to PATH, CWD or cache."""
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

executable, module, runtime = map(Path, sys.argv[1:])
environment = os.environ.copy()
environment['QT_QPA_PLATFORM'] = 'offscreen'
environment['QT_FORCE_STDERR_LOGGING'] = '1'
environment['PATH'] = str(runtime) + os.pathsep + environment['PATH']

with tempfile.TemporaryDirectory(prefix='qliteview-runtime-') as temporary:
    directory = Path(temporary)
    child = directory / executable.name
    shutil.copy2(executable, child)
    shutil.copy2(module, directory / 'qliteview.dll')
    for invalid in (False, True):
        if invalid:
            (directory / 'livicore.dll').write_bytes(b'not a Windows DLL')
        result = subprocess.run(
            [str(child), '--check-runtime'], cwd=runtime, env=environment,
            capture_output=True, text=True, encoding='utf-8', errors='replace', timeout=20)
        output = result.stdout + result.stderr
        expected = 'Windows error 193' if invalid else 'Missing LiteView runtime:'
        if result.returncode == 0 or expected not in output or str(directory).lower() not in output.lower():
            raise AssertionError(f'Unexpected loader result ({result.returncode}): {output}')
        print(f'PASS: {"invalid" if invalid else "missing"} app-local runtime is rejected')
    shutil.copy2(runtime / 'livicore.dll', directory / 'livicore.dll')
    result = subprocess.run(
        [str(child), '--check-runtime'], cwd=runtime, env=environment,
        capture_output=True, text=True, encoding='utf-8', errors='replace', timeout=20)
    if result.returncode != 0:
        raise AssertionError(f'Relocated runtime failed: {result.stdout}{result.stderr}')
    print('PASS: relocated application loads its app-local runtime')
