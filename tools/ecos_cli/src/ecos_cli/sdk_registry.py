"""Persistent, cross-platform registry for installed and checkout SDKs."""

from __future__ import annotations

import json
import os
import re
import stat
import sys
import tempfile
import time
from contextlib import contextmanager
from pathlib import Path
from typing import Any, Iterator, Mapping, Optional

from . import sdk_manifest


REGISTRY_SCHEMA_VERSION = 1
REGISTRATION_NAME_PATTERN = re.compile(r"^[A-Za-z0-9][A-Za-z0-9._-]*$")


class SdkRegistryError(RuntimeError):
    """The SDK registry is invalid or an operation cannot be completed."""


class SdkRegistrationConflict(SdkRegistryError):
    """A registration name already identifies a different SDK."""


class SdkRegistrationNotFound(SdkRegistryError):
    """A requested SDK registration does not exist."""


def default_registry_path(
    *,
    environ: Optional[Mapping[str, str]] = None,
    home: Optional[Path] = None,
    platform_name: Optional[str] = None,
) -> Path:
    env = os.environ if environ is None else environ
    override = env.get("ECOS_SDK_REGISTRY")
    if override:
        return Path(override).expanduser().resolve()
    home_root = (home or Path.home()).expanduser().resolve()
    host = platform_name or sys.platform
    if host == "win32":
        base = Path(env.get("APPDATA", home_root / "AppData" / "Roaming"))
        return base / "ECOS" / "sdks.json"
    if host == "darwin":
        return home_root / "Library" / "Application Support" / "ECOS" / "sdks.json"
    base = Path(env.get("XDG_CONFIG_HOME", home_root / ".config"))
    return base / "ecos" / "sdks.json"


def empty_registry() -> dict[str, Any]:
    return {
        "schema_version": REGISTRY_SCHEMA_VERSION,
        "active": None,
        "sdks": {},
    }


def _validate_entry(name: str, entry: Any) -> dict[str, Any]:
    if not REGISTRATION_NAME_PATTERN.fullmatch(name):
        raise SdkRegistryError(f"invalid SDK registration name in registry: {name!r}")
    if not isinstance(entry, dict):
        raise SdkRegistryError(f"SDK registration must be an object: {name}")
    required = {"sdk_id", "sdk_version", "channel", "kind", "root"}
    missing = sorted(required - entry.keys())
    if missing:
        raise SdkRegistryError(
            f"SDK registration {name} is missing: {', '.join(missing)}"
        )
    if entry["kind"] not in {"checkout", "release"}:
        raise SdkRegistryError(
            f"SDK registration {name} has invalid kind: {entry['kind']!r}"
        )
    for field in ("sdk_id", "sdk_version", "channel", "root"):
        if not isinstance(entry[field], str) or not entry[field]:
            raise SdkRegistryError(
                f"SDK registration {name} field {field} must be a non-empty string"
            )
    return entry


def validate_registry(payload: Any, source: Path) -> dict[str, Any]:
    if not isinstance(payload, dict):
        raise SdkRegistryError(f"SDK registry must contain an object: {source}")
    if payload.get("schema_version") != REGISTRY_SCHEMA_VERSION:
        raise SdkRegistryError(
            f"unsupported SDK registry schema in {source}: "
            f"{payload.get('schema_version')!r}"
        )
    registrations = payload.get("sdks")
    if not isinstance(registrations, dict):
        raise SdkRegistryError(f"SDK registry sdks must be an object: {source}")
    payload["sdks"] = {
        name: _validate_entry(name, entry)
        for name, entry in sorted(registrations.items())
    }
    active = payload.get("active")
    if active is not None and active not in payload["sdks"]:
        raise SdkRegistryError(
            f"active SDK registration does not exist in {source}: {active!r}"
        )
    return payload


class SdkRegistry:
    def __init__(self, path: Optional[Path] = None) -> None:
        self.path = (path or default_registry_path()).expanduser().resolve()
        self.lock_path = self.path.with_name(f".{self.path.name}.lock")

    def load(self) -> dict[str, Any]:
        if not self.path.exists():
            return empty_registry()
        try:
            payload = json.loads(self.path.read_text(encoding="utf-8"))
        except OSError as exc:
            raise SdkRegistryError(f"cannot read SDK registry {self.path}: {exc}") from exc
        except json.JSONDecodeError as exc:
            raise SdkRegistryError(f"cannot parse SDK registry {self.path}: {exc}") from exc
        return validate_registry(payload, self.path)

    @contextmanager
    def _lock(self, timeout: float = 5.0) -> Iterator[None]:
        self.path.parent.mkdir(parents=True, exist_ok=True)
        deadline = time.monotonic() + timeout
        descriptor: Optional[int] = None
        while descriptor is None:
            try:
                descriptor = os.open(
                    self.lock_path,
                    os.O_CREAT | os.O_EXCL | os.O_WRONLY,
                    0o600,
                )
            except FileExistsError:
                try:
                    stale = time.time() - self.lock_path.stat().st_mtime > 60
                except FileNotFoundError:
                    continue
                if stale:
                    try:
                        self.lock_path.unlink()
                    except FileNotFoundError:
                        pass
                    continue
                if time.monotonic() >= deadline:
                    raise SdkRegistryError(
                        f"timed out waiting for SDK registry lock: {self.lock_path}"
                    )
                time.sleep(0.05)
        try:
            os.write(descriptor, f"{os.getpid()}\n".encode("ascii"))
            yield
        finally:
            os.close(descriptor)
            try:
                self.lock_path.unlink()
            except FileNotFoundError:
                pass

    def _write(self, payload: dict[str, Any]) -> None:
        validated = validate_registry(payload, self.path)
        self.path.parent.mkdir(parents=True, exist_ok=True)
        previous_mode = stat.S_IMODE(self.path.stat().st_mode) if self.path.exists() else 0o600
        content = json.dumps(validated, indent=2, ensure_ascii=False, sort_keys=True) + "\n"
        with tempfile.NamedTemporaryFile(
            prefix=f".{self.path.name}.", dir=self.path.parent, delete=False
        ) as temporary:
            temporary_path = Path(temporary.name)
            temporary.write(content.encode("utf-8"))
        try:
            temporary_path.chmod(previous_mode)
            temporary_path.replace(self.path)
        finally:
            temporary_path.unlink(missing_ok=True)

    @staticmethod
    def _entry(root: Path, manifest: dict[str, Any], kind: str) -> dict[str, Any]:
        return {
            "sdk_id": manifest["sdk_id"],
            "sdk_version": manifest["sdk_version"],
            "channel": manifest["channel"],
            "kind": kind,
            "root": str(root),
        }

    def preview_register(
        self,
        root: Path,
        *,
        name: Optional[str] = None,
        kind: str = "checkout",
        activate: bool = False,
        replace: bool = False,
    ) -> dict[str, Any]:
        sdk_root, manifest = sdk_manifest.validate_sdk_root(root)
        return self.preview_registration(
            sdk_root,
            manifest,
            name=name,
            kind=kind,
            activate=activate,
            replace=replace,
        )

    def preview_registration(
        self,
        root: Path,
        manifest: dict[str, Any],
        *,
        name: Optional[str] = None,
        kind: str = "checkout",
        activate: bool = False,
        replace: bool = False,
    ) -> dict[str, Any]:
        sdk_root = root.expanduser().resolve()
        registration_name = name or manifest["sdk_version"]
        if not REGISTRATION_NAME_PATTERN.fullmatch(registration_name):
            raise SdkRegistryError(
                f"invalid SDK registration name: {registration_name!r}"
            )
        if kind not in {"checkout", "release"}:
            raise SdkRegistryError(f"invalid SDK registration kind: {kind!r}")
        entry = self._entry(sdk_root, manifest, kind)
        registry = self.load()
        existing = registry["sdks"].get(registration_name)
        if existing is not None and existing != entry and not replace:
            raise SdkRegistrationConflict(
                f"SDK registration {registration_name!r} already identifies "
                f"{existing['root']} ({existing['sdk_version']})"
            )
        active = registration_name if activate else registry["active"]
        changed = existing != entry or registry["active"] != active
        return {
            "name": registration_name,
            "entry": entry,
            "active": active,
            "changed": changed,
            "registry": str(self.path),
        }

    def register(
        self,
        root: Path,
        *,
        name: Optional[str] = None,
        kind: str = "checkout",
        activate: bool = False,
        replace: bool = False,
    ) -> dict[str, Any]:
        sdk_root, manifest = sdk_manifest.validate_sdk_root(root)
        registration_name = name or manifest["sdk_version"]
        if not REGISTRATION_NAME_PATTERN.fullmatch(registration_name):
            raise SdkRegistryError(
                f"invalid SDK registration name: {registration_name!r}"
            )
        if kind not in {"checkout", "release"}:
            raise SdkRegistryError(f"invalid SDK registration kind: {kind!r}")
        entry = self._entry(sdk_root, manifest, kind)
        with self._lock():
            registry = self.load()
            previous_active = registry["active"]
            existing = registry["sdks"].get(registration_name)
            if existing is not None and existing != entry and not replace:
                raise SdkRegistrationConflict(
                    f"SDK registration {registration_name!r} already identifies "
                    f"{existing['root']} ({existing['sdk_version']})"
                )
            registry["sdks"][registration_name] = entry
            if activate:
                registry["active"] = registration_name
            changed = existing != entry or registry["active"] != previous_active
            self._write(registry)
        return {
            "name": registration_name,
            "entry": entry,
            "active": registry["active"],
            "changed": changed,
            "registry": str(self.path),
        }

    def registrations(self) -> dict[str, dict[str, Any]]:
        return self.load()["sdks"]

    def active_name(self) -> Optional[str]:
        return self.load()["active"]

    def find(self, selector: str) -> tuple[str, dict[str, Any]]:
        registry = self.load()
        if selector in registry["sdks"]:
            return selector, registry["sdks"][selector]
        matches = [
            (name, entry)
            for name, entry in registry["sdks"].items()
            if selector in {entry["sdk_version"], entry["sdk_id"]}
        ]
        if not matches:
            raise SdkRegistrationNotFound(f"SDK is not registered: {selector}")
        if len(matches) > 1:
            names = ", ".join(name for name, _ in matches)
            raise SdkRegistryError(
                f"SDK selector {selector!r} is ambiguous; choose a registration name: {names}"
            )
        return matches[0]

    def use(self, selector: str) -> dict[str, Any]:
        with self._lock():
            registry = self.load()
            if selector in registry["sdks"]:
                name = selector
            else:
                matches = [
                    candidate
                    for candidate, entry in registry["sdks"].items()
                    if selector in {entry["sdk_version"], entry["sdk_id"]}
                ]
                if not matches:
                    raise SdkRegistrationNotFound(f"SDK is not registered: {selector}")
                if len(matches) > 1:
                    raise SdkRegistryError(
                        f"SDK selector {selector!r} is ambiguous: {', '.join(matches)}"
                    )
                name = matches[0]
            changed = registry["active"] != name
            registry["active"] = name
            self._write(registry)
        return {
            "name": name,
            "entry": registry["sdks"][name],
            "active": name,
            "changed": changed,
            "registry": str(self.path),
        }

    def unregister(self, name: str) -> dict[str, Any]:
        with self._lock():
            registry = self.load()
            try:
                entry = registry["sdks"].pop(name)
            except KeyError as exc:
                raise SdkRegistrationNotFound(
                    f"SDK registration does not exist: {name}"
                ) from exc
            if registry["active"] == name:
                registry["active"] = None
            self._write(registry)
        return {
            "name": name,
            "entry": entry,
            "active": registry["active"],
            "changed": True,
            "registry": str(self.path),
        }

    def doctor(self) -> dict[str, Any]:
        registry = self.load()
        entries = []
        for name, entry in registry["sdks"].items():
            state = "valid"
            error = None
            try:
                root, manifest = sdk_manifest.validate_sdk_root(Path(entry["root"]))
                if (
                    manifest["sdk_id"] != entry["sdk_id"]
                    or manifest["sdk_version"] != entry["sdk_version"]
                ):
                    raise SdkRegistryError("registered identity no longer matches manifest")
            except (sdk_manifest.SdkManifestError, SdkRegistryError) as exc:
                state = "invalid"
                error = str(exc)
                root = Path(entry["root"])
            entries.append(
                {
                    "name": name,
                    "state": state,
                    "root": str(root),
                    "sdk_id": entry["sdk_id"],
                    "sdk_version": entry["sdk_version"],
                    "active": registry["active"] == name,
                    "error": error,
                }
            )
        return {
            "registry": str(self.path),
            "active": registry["active"],
            "entries": entries,
            "valid": all(entry["state"] == "valid" for entry in entries),
        }
