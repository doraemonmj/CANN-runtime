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
from pathlib import Path


# Resolve library path
lib_path = Path(__file__).resolve().parent.parent / "libmachine.dylib"  # Linux: libmachine.so
lib = CDLL(str(lib_path))


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
    lib_path_bytes = b"libadd.dylib"
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