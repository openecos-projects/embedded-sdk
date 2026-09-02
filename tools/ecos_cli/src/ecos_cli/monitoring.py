"""Cross-platform serial monitoring for Board console resources."""

from __future__ import annotations

import sys
import time
from pathlib import Path
from typing import Any, Optional, TextIO

from .project_model import ProjectModelError, resolve_project
from .sdk_context import SdkContext


class MonitorError(RuntimeError):
    """Base error for serial monitoring."""


class MonitorConfigurationError(MonitorError):
    """The selected Board has no usable monitor configuration."""


class MonitorDependencyError(MonitorError):
    """PySerial is unavailable."""


class MonitorPortError(MonitorError):
    """A serial port cannot be selected or opened."""


class MonitorExpectationError(MonitorError):
    """Expected output was not observed before the timeout."""


def _serial_modules() -> tuple[Any, Any]:
    try:
        import serial  # type: ignore
        from serial.tools import list_ports  # type: ignore
    except ImportError as exc:
        raise MonitorDependencyError(
            "PySerial is unavailable; run 'tools/install.py' to install the "
            "SDK's pinned Python dependencies"
        ) from exc
    return serial, list_ports


def discover_ports(config: dict[str, Any]) -> list[dict[str, Any]]:
    _, list_ports = _serial_modules()
    expected_vid = config.get("vid")
    expected_pid = config.get("pid")
    expected_serial = config.get("serial_number")
    results: list[dict[str, Any]] = []
    for item in list_ports.comports():
        if expected_vid is not None and item.vid != expected_vid:
            continue
        if expected_pid is not None and item.pid != expected_pid:
            continue
        if expected_serial is not None and item.serial_number != expected_serial:
            continue
        results.append(
            {
                "device": item.device,
                "description": item.description,
                "vid": item.vid,
                "pid": item.pid,
                "serial_number": item.serial_number,
            }
        )
    return sorted(results, key=lambda value: value["device"])


def _select_port(
    config: dict[str, Any], explicit: Optional[str]
) -> tuple[str, list[dict[str, Any]]]:
    if explicit:
        return explicit, []
    ports = discover_ports(config)
    if not ports:
        raise MonitorPortError(
            "no matching serial port was found; connect the board or pass --port PORT"
        )
    if len(ports) > 1:
        names = ", ".join(item["device"] for item in ports)
        raise MonitorPortError(
            f"multiple matching serial ports were found: {names}; pass --port PORT"
        )
    return ports[0]["device"], ports


def monitor_project(
    context: SdkContext,
    *,
    project_root: Optional[Path] = None,
    port: Optional[str] = None,
    baudrate: Optional[int] = None,
    timeout: Optional[float] = None,
    expect: Optional[str] = None,
    stream: Optional[TextIO] = sys.stdout,
) -> dict[str, Any]:
    root = (project_root or Path.cwd()).expanduser().resolve()
    try:
        resolved = resolve_project(context, project_root=root)
    except ProjectModelError as exc:
        raise MonitorConfigurationError(str(exc)) from exc
    board = resolved.get("board")
    if not isinstance(board, dict):
        raise MonitorConfigurationError(
            "serial monitoring requires a selected Board; Target-only projects "
            "have no console port contract"
        )
    config = board.get("monitor")
    if not isinstance(config, dict):
        raise MonitorConfigurationError(
            f"Board {board['id']!r} does not declare serial monitor settings"
        )
    selected_baudrate = (
        config.get("baudrate", 115200) if baudrate is None else baudrate
    )
    if selected_baudrate < 1:
        raise MonitorConfigurationError("monitor baudrate must be positive")
    if timeout is not None and timeout <= 0:
        raise MonitorConfigurationError("monitor timeout must be positive")
    selected_port, discovered = _select_port(config, port)
    serial, _ = _serial_modules()
    try:
        connection = serial.Serial(
            port=selected_port,
            baudrate=selected_baudrate,
            timeout=0.1,
        )
    except (OSError, serial.SerialException) as exc:
        raise MonitorPortError(
            f"cannot open serial port {selected_port} at {selected_baudrate} baud: {exc}"
        ) from exc

    started = time.monotonic()
    deadline = started + timeout if timeout is not None else None
    expected = expect.encode("utf-8") if expect is not None else None
    captured = bytearray()
    bytes_received = 0
    matched = False
    interrupted = False
    try:
        while True:
            try:
                chunk = connection.read(4096)
            except (OSError, serial.SerialException) as exc:
                raise MonitorPortError(
                    f"serial read failed on {selected_port}: {exc}"
                ) from exc
            if chunk:
                bytes_received += len(chunk)
                captured.extend(chunk)
                if len(captured) > 65536:
                    del captured[:-65536]
                if stream is not None:
                    stream.write(chunk.decode("utf-8", errors="replace"))
                    stream.flush()
                if expected is not None and expected in captured:
                    matched = True
                    break
            if deadline is not None and time.monotonic() >= deadline:
                break
    except KeyboardInterrupt:
        interrupted = True
    finally:
        connection.close()
    elapsed = time.monotonic() - started
    if expected is not None and not matched:
        raise MonitorExpectationError(
            f"serial output did not contain {expect!r} within {timeout} seconds"
        )
    return {
        "path": str(root),
        "board": board["id"],
        "target": resolved["project"]["target"],
        "port": selected_port,
        "baudrate": selected_baudrate,
        "discovered": discovered,
        "bytes_received": bytes_received,
        "elapsed_seconds": round(elapsed, 3),
        "expect": expect,
        "matched": matched if expect is not None else None,
        "interrupted": interrupted,
        "output": captured.decode("utf-8", errors="replace"),
    }
