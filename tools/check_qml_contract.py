"""Compare Qt moc metadata against the frozen qwebkit QML surface.

This checks declarations and enum definitions, not runtime behavior.
"""
import argparse
import difflib
import json
import re
from pathlib import Path


def normalize(value):
    if isinstance(value, dict):
        return {key: normalize(item) for key, item in value.items()
                if key not in {"lineNumber", "index"}}
    if isinstance(value, list):
        return [normalize(item) for item in value]
    return value


def surface(path):
    units = json.loads(path.read_text(encoding="utf-8"))
    if isinstance(units, dict):
        units = [units]
    result = {}
    for unit in units:
        for cls in unit.get("classes", []):
            if cls.get("className", "").startswith("WebEngine"):
                result[cls["className"]] = normalize(cls)
    if not result:
        raise ValueError(f"No WebEngine QML types found in {path}")
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("metadata", type=Path)
    parser.add_argument("--baseline", type=Path, default=Path(__file__).resolve().parents[1]
                        / "compatibility/qwebkit-metatypes.json")
    args = parser.parse_args()
    expected, actual = surface(args.baseline), surface(args.metadata)
    if expected != actual:
        print("".join(difflib.unified_diff(
            json.dumps(expected, indent=2, sort_keys=True).splitlines(keepends=True),
            json.dumps(actual, indent=2, sort_keys=True).splitlines(keepends=True),
            fromfile="qwebkit API baseline", tofile=str(args.metadata))))
        return 1
    root = Path(__file__).resolve().parents[1]
    def enums(path):
        source = path.read_text(encoding="utf-8")
        return {name: re.sub(r"\s+", "", body) for name, body in
                re.findall(r"enum\s+(\w+)\s*\{(.*?)\}", source, re.DOTALL)}
    if enums(root / "compatibility/webengineview.h") != enums(root / "src/QWebKit/webengineview.h"):
        print("FAIL: WebEngineView enum definitions differ from qwebkit.")
        return 1
    print(f"PASS: {len(expected)} QML types match the qwebkit declaration baseline.")
    print("PASS: WebEngineView enum order and explicit values match qwebkit.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
