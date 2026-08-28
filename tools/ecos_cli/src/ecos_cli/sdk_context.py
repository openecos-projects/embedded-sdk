"""Validated SDK context shared by ECOS commands."""

from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
from typing import Any, Optional


@dataclass(frozen=True)
class SdkContext:
    root: Path
    sdk_id: str
    version: str
    channel: str
    kind: str
    source: str
    manifest: dict[str, Any]
    registration_name: Optional[str] = None

    def resource(self, name: str) -> Path:
        try:
            relative = self.manifest["layout"][name]
        except KeyError as exc:
            raise KeyError(f"SDK layout does not define resource: {name}") from exc
        return self.root / Path(*relative.split("/"))

    def as_dict(self) -> dict[str, Any]:
        return {
            "sdk_id": self.sdk_id,
            "sdk_version": self.version,
            "channel": self.channel,
            "kind": self.kind,
            "root": str(self.root),
            "source": self.source,
            "registration_name": self.registration_name,
        }
