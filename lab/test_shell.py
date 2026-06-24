"""
Black-box tests for the mini-kernel shell (echo / uptime / alarm).

We don't poke at the kernel internals: instead we boot the kernel under QEMU,
talk to it over the emulated serial port (muxed onto QEMU's stdio) and check
the shell's responses, exactly like a human typing at the console would.

Run with:  pytest lab/test_shell.py
"""

import re
import subprocess
import threading
import time
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parent.parent
BUILD = ROOT / "build"
KERNEL = BUILD / "src" / "kernel.elf"
PROMPT = "> "

QEMU = [
    "qemu-system-riscv64", "-nographic", "-m", "1G", "-machine", "virt",
    "-bios", "none", "-monitor", "none", "-serial", "stdio",
    "-kernel", str(KERNEL),
]


@pytest.fixture(scope="session", autouse=True)
def build_kernel():
    """Build the kernel image once before any test runs."""
    if not (BUILD / "build.ninja").exists():
        subprocess.run(
            ["meson", "setup", "--cross-file",
             str(ROOT / "meson-llvm-riscv.ini"), str(BUILD), str(ROOT)],
            check=True,
        )
    proc = subprocess.run(
        ["ninja", "-C", str(BUILD), "src/kernel.elf"],
        capture_output=True, text=True,
    )
    if proc.returncode != 0 or not KERNEL.exists():
        pytest.fail(
            f"failed to build the kernel image at {KERNEL}\n"
            f"--- ninja output ---\n{proc.stdout}\n{proc.stderr}"
        )


class Shell:
    """Drives one QEMU instance and exposes the serial console."""

    def __init__(self):
        self.proc = subprocess.Popen(
            QEMU, stdin=subprocess.PIPE, stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT, bufsize=0,
        )
        self._buf = bytearray()
        self._lock = threading.Lock()
        self._last = time.time()
        threading.Thread(target=self._reader, daemon=True).start()

    def _reader(self):
        while True:
            b = self.proc.stdout.read(1)
            if not b:
                return
            with self._lock:
                self._buf += b
                self._last = time.time()

    @property
    def text(self):
        with self._lock:
            return self._buf.decode(errors="replace")

    def clear(self):
        with self._lock:
            self._buf.clear()

    def send(self, line):
        """Type `line` and "press Enter" (send carriage return)."""
        self.proc.stdin.write((line + "\r").encode())
        self.proc.stdin.flush()

    def drain(self, idle=0.4, timeout=15):
        """Return console output once QEMU has gone quiet for `idle` seconds."""
        deadline = time.time() + timeout
        while time.time() < deadline:
            with self._lock:
                quiet = time.time() - self._last
                got = len(self._buf)
            if got and quiet >= idle:
                break
            time.sleep(0.05)
        return self.text

    def wait_for(self, pattern, timeout):
        """Poll the console until `pattern` shows up (or we time out)."""
        deadline = time.time() + timeout
        rx = re.compile(pattern)
        while time.time() < deadline:
            if rx.search(self.text):
                break
            time.sleep(0.05)
        return self.text

    def close(self):
        self.proc.kill()
        try:
            self.proc.wait(timeout=5)
        except subprocess.TimeoutExpired:
            pass


def lines(text):
    """Split a console capture into clean lines (serial uses CR/LF)."""
    return [ln.strip() for ln in text.replace("\r", "\n").split("\n") if ln.strip()]


@pytest.fixture
def shell():
    sh = Shell()
    boot = sh.drain()
    if sh.proc.poll() is not None:
        sh.close()
        pytest.fail(f"QEMU exited during boot.\n--- console ---\n{boot}")
    assert boot.rstrip(" ").endswith(PROMPT.rstrip()), (
        "shell never displayed its '> ' prompt after boot; either it did not "
        f"start or it uses a different prompt.\n--- console ---\n{boot}"
    )
    yield sh
    sh.close()


def test_echo(shell):
    """`echo [str]` must print back exactly the supplied string."""
    shell.clear()
    shell.send("echo hello world")
    out = shell.drain()
    assert "hello world" in lines(out), (
        "expected the line 'hello world' echoed back by `echo`, but it was "
        f"missing.\n--- console after sending 'echo hello world' ---\n{out}"
    )


def test_uptime(shell):
    """`uptime` must report the seconds since boot as `<number>s`."""
    shell.clear()
    shell.send("uptime")
    out = shell.drain()
    matches = [ln for ln in lines(out) if re.fullmatch(r"\d+s", ln)]
    assert matches, (
        "expected `uptime` to print the boot time as '<seconds>s' (e.g. "
        f"'1281s'), but found no such line.\n--- console after sending "
        f"'uptime' ---\n{out}"
    )


def test_alarm(shell):
    """`alarm [t]` must print `alarm` roughly `t` seconds later."""
    delay = 2
    shell.clear()
    shell.send(f"alarm {delay}")
    # Consume the echoed command and prompt so the next 'alarm' we see is the
    # timer firing, not the text we just typed.
    shell.drain()
    shell.clear()

    out = shell.wait_for(r"alarm", timeout=delay + 6)
    assert "alarm" in lines(out), (
        f"expected the string 'alarm' to be printed ~{delay}s after sending "
        "'alarm 2' (this requires a working timer interrupt). Nothing was "
        f"printed within the timeout.\n--- console while waiting ---\n{out!r}"
    )
