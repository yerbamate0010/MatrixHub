Import("env")

import pathlib
import subprocess
import sys


if sys.platform == "darwin":
    resource_dir = subprocess.check_output(
        ["xcrun", "clang", "--print-resource-dir"], text=True
    ).strip()
    profile_runtime = (
        pathlib.Path(resource_dir) / "lib" / "darwin" / "libclang_rt.profile_osx.a"
    )

    if not profile_runtime.exists():
        raise FileNotFoundError(
            f"LLVM profile runtime not found: {profile_runtime}"
        )

    env.Append(LINKFLAGS=[str(profile_runtime)])
