#!/usr/bin/env python3
import argparse,json,subprocess
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument("source",type=Path);a=p.parse_args()
root=Path(__file__).resolve().parents[1];expected=json.loads((root/"UPSTREAM.json").read_text())["commit"]
actual=subprocess.check_output(["git","-C",str(a.source),"rev-parse","HEAD"],text=True).strip()
if actual!=expected:raise SystemExit("Upstream commit mismatch; use UPSTREAM.json")
patch=str(root/"patches/switch-runtime.patch")
subprocess.run(["git","-C",str(a.source),"apply","--check",patch],check=True)
subprocess.run(["git","-C",str(a.source),"apply",patch],check=True)
print("Runtime patch applied; prepare private generated game inputs locally")
