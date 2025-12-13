from ctypes import (
    CDLL,
    POINTER,
    Structure,
    byref,
    c_char_p,
    c_int,
    c_void_p,
    cast,
    sizeof,
)
import subprocess
import sysconfig
import tempfile
import shutil
from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent
BUNDLE_DIR = ROOT / "build"  # where we store copied libs for loading
SHLIB_SUFFIX = sysconfig.get_config_var("SHLIB_SUFFIX") or ".so"
TARGET_SOURCES = {
    "machine": ROOT / "src" / "machine",
    "addworker": ROOT / "src" / "worker" / "add",
}


def ensure_target(target: str) -> Path:
    """Configure/build a single target in a temp dir, copy the .so/.dylib locally."""
    source_dir = TARGET_SOURCES[target]
    temp_build = Path(tempfile.gettempdir()) / f"simpler_{target}_build"
    temp_build.mkdir(parents=True, exist_ok=True)
    cache = temp_build / "CMakeCache.txt"
    if not cache.exists():
        subprocess.check_call(["cmake", "-S", str(source_dir), "-B", str(temp_build)])
    subprocess.check_call(["cmake", "--build", str(temp_build)])
    suffixes = []
    suffixes.append(SHLIB_SUFFIX)
    for alt in (".dylib", ".so", ".dll"):
        if alt not in suffixes:
            suffixes.append(alt)
    built_lib: Path | None = None
    for suf in suffixes:
        candidate = temp_build / f"lib{target}{suf}"
        if candidate.exists():
            built_lib = candidate
            break
    if built_lib is None:
        raise FileNotFoundError(f"built library for {target} not found in {temp_build}")

    dest = BUNDLE_DIR / built_lib.name
    BUNDLE_DIR.mkdir(exist_ok=True)
    if not dest.exists() or built_lib.stat().st_mtime > dest.stat().st_mtime:
        shutil.copy2(built_lib, dest)
    return dest


# JIT build only what is needed.
lib_machine_path = ensure_target("machine")
lib = CDLL(str(lib_machine_path))


class MachineHandle(Structure):
    pass


class Runable(Structure):
    _fields_ = [
        ("data_length", c_int),
        ("data", c_void_p),
        ("binary_length", c_int),
        ("binary", c_void_p),
        ("entry_length", c_int),
        ("entry_point", c_char_p),
    ]


class Task(Structure):
    _fields_ = [
        ("id", c_int),
        ("runable", Runable),
    ]


class AddPayload(Structure):
    _fields_ = [("a", c_int), ("b", c_int), ("result", c_int)]


# C API bindings
lib.CreateMachine.restype = POINTER(MachineHandle)
lib.DestroyMachine.argtypes = (POINTER(MachineHandle),)
lib.PushTaskC.argtypes = (POINTER(MachineHandle), POINTER(Task))
lib.PushTaskC.restype = c_int
lib.PopTaskC.argtypes = (POINTER(MachineHandle), POINTER(Task))
lib.PopTaskC.restype = c_int


def main() -> None:
    machine = lib.CreateMachine()
    # Prepare payload buffer owned by caller.
    payload = AddPayload(3, 4, 0)
    # Build worker on demand.
    lib_add_path = ensure_target("addworker")
    lib_path_bytes = str(lib_add_path).encode()
    entry_bytes = b"AddWorker"
    lib_path_c = c_char_p(lib_path_bytes)
    entry_c = c_char_p(entry_bytes)

    runable = Runable(
        data_length=sizeof(payload),
        data=cast(byref(payload), c_void_p),
        binary_length=len(lib_path_bytes) + 1,
        binary=cast(lib_path_c, c_void_p),
        entry_length=len(entry_bytes) + 1,
        entry_point=entry_c,
    )
    task = Task(id=1, runable=runable)

    lib.PushTaskC(machine, byref(task))

    finished = Task()
    ok = lib.PopTaskC(machine, byref(finished))
    if ok:
        finished_payload = cast(finished.runable.data, POINTER(AddPayload)).contents
        print(f"task {finished.id} result: {finished_payload.result}")

    lib.DestroyMachine(machine)


if __name__ == "__main__":
    main()