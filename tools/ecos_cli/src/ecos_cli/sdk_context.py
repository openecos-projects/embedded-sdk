"""Validated SDK context shared by ECOS commands."""

from __future__ import annotations

from dataclasses import dataclass, field
from pathlib import Path
from typing import Any, Optional

from . import user_bsp


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
    user_bsp_root: Path = field(default_factory=user_bsp.default_root)

    def resource(self, name: str) -> Path:
        try:
            relative = self.manifest["layout"][name]
        except KeyError as exc:
            raise KeyError(f"SDK layout does not define resource: {name}") from exc
        return self.root / Path(*relative.split("/"))

    def board_roots(self) -> tuple[Path, ...]:
        sdk_boards = self.resource("boards")
        if sdk_boards.resolve() == self.user_bsp_root.resolve():
            return (sdk_boards,)
        return sdk_boards, self.user_bsp_root

    def board_manifests(self) -> tuple[Path, ...]:
        return tuple(
            sorted(
                path
                for root in self.board_roots()
                for path in root.glob("*/ecos-board.yml")
            )
        )

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
