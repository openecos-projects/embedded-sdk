"""Console progress rendering shared by ECOS installers."""

from __future__ import annotations

import sys
import os
from dataclasses import dataclass
from typing import Optional, TextIO


@dataclass(frozen=True)
class DownloadStatus:
    downloaded: int
    total: Optional[int]
    elapsed: float
    done: bool = False
    failed: bool = False


def _format_bytes(value: float) -> str:
    units = ("B", "KiB", "MiB", "GiB", "TiB")
    size = float(value)
    for unit in units:
        if abs(size) < 1024 or unit == units[-1]:
            return f"{size:.1f} {unit}" if unit != "B" else f"{size:.0f} {unit}"
        size /= 1024
    return f"{size:.1f} TiB"


def _format_duration(seconds: float) -> str:
    remaining = max(0, round(seconds))
    minutes, secs = divmod(remaining, 60)
    hours, minutes = divmod(minutes, 60)
    if hours:
        return f"{hours}h {minutes:02d}m"
    if minutes:
        return f"{minutes}m {secs:02d}s"
    return f"{secs}s"


def format_download_status(status: DownloadStatus, width: int = 24) -> str:
    elapsed = max(status.elapsed, 0.001)
    speed = status.downloaded / elapsed
    speed_text = f"{_format_bytes(speed)}/s"
    if status.total and status.total > 0:
        ratio = min(status.downloaded / status.total, 1.0)
        filled = min(round(ratio * width), width)
        bar = "#" * filled + "-" * (width - filled)
        remaining = max(status.total - status.downloaded, 0)
        eta = _format_duration(remaining / speed) if speed > 0 else "--"
        return (
            f"Downloading [{bar}] {ratio * 100:5.1f}%  "
            f"{_format_bytes(status.downloaded)} / {_format_bytes(status.total)}  "
            f"{speed_text}  ETA {eta}"
        )
    return f"Downloading {_format_bytes(status.downloaded)}  {speed_text}"


class ConsoleProgress:
    """Render live progress on a TTY and sparse updates in redirected logs."""

    def __init__(
        self,
        stream: Optional[TextIO] = None,
        *,
        enabled: bool = True,
        log_interval: float = 5.0,
    ) -> None:
        self.stream = stream or sys.stderr
        self.enabled = enabled
        self.log_interval = log_interval
        self._interactive = bool(getattr(self.stream, "isatty", lambda: False)())
        self._color = (
            self._interactive
            and "NO_COLOR" not in os.environ
            and os.environ.get("TERM", "") != "dumb"
        )
        self._download_active = False
        self._last_log_elapsed = -log_interval

    def message(self, message: str) -> None:
        self.info(message)

    def info(self, message: str) -> None:
        self._log("INFO", message, "\033[1;32m")

    def warning(self, message: str) -> None:
        self._log("WARN", message, "\033[1;33m")

    def error(self, message: str) -> None:
        self._log("ERR", message, "\033[1;31m")

    def _log(self, level: str, message: str, color: str) -> None:
        if not self.enabled:
            return
        self._end_live_line()
        print(f"{self._prefix(level, color)} {message}", file=self.stream, flush=True)

    def download(self, status: DownloadStatus) -> None:
        if not self.enabled:
            return
        should_render = (
            self._interactive
            or status.done
            or status.elapsed - self._last_log_elapsed >= self.log_interval
        )
        if not should_render:
            return
        line = f"{self._prefix('INFO', '\033[1;32m')} {format_download_status(status)}"
        if status.failed:
            line += "  failed"
        if self._interactive:
            print(f"\r{line}\033[K", end="", file=self.stream, flush=True)
            self._download_active = True
            if status.done:
                print(file=self.stream, flush=True)
                self._download_active = False
        else:
            print(line, file=self.stream, flush=True)
        self._last_log_elapsed = status.elapsed

    def _prefix(self, level: str, color: str) -> str:
        prefix = f"【ECOS-{level}】"
        return f"{color}{prefix}\033[0m" if self._color else prefix

    def _end_live_line(self) -> None:
        if self._interactive and self._download_active:
            print(file=self.stream, flush=True)
            self._download_active = False
