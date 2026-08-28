"""Deterministic SDK selection and project pin handling."""

from __future__ import annotations

import json
import os
import stat
import tempfile
from pathlib import Path
from typing import Any, Mapping, Optional

from . import sdk_manifest
from .sdk_context import SdkContext
from .sdk_registry import SdkRegistrationNotFound, SdkRegistry, SdkRegistryError


PROJECT_PIN_RELATIVE_PATH = Path(".ecos") / "sdk.json"
PROJECT_PIN_SCHEMA_VERSION = 1


class SdkResolutionError(RuntimeError):
    """No valid SDK can be selected from the configured sources."""


def _read_project_pin(path: Path) -> dict[str, Any]:
    try:
        payload = json.loads(path.read_text(encoding="utf-8"))
    except OSError as exc:
        raise SdkResolutionError(f"cannot read project SDK pin {path}: {exc}") from exc
    except json.JSONDecodeError as exc:
        raise SdkResolutionError(f"cannot parse project SDK pin {path}: {exc}") from exc
    if not isinstance(payload, dict) or payload.get("schema_version") != 1:
        raise SdkResolutionError(f"unsupported project SDK pin schema: {path}")
    required = {"registration", "sdk_id", "sdk_version"}
    missing = sorted(required - payload.keys())
    if missing:
        raise SdkResolutionError(
            f"project SDK pin {path} is missing: {', '.join(missing)}"
        )
    invalid = [
        field
        for field in sorted(required)
        if not isinstance(payload[field], str) or not payload[field]
    ]
    if invalid:
        raise SdkResolutionError(
            f"project SDK pin {path} fields must be non-empty strings: "
            f"{', '.join(invalid)}"
        )
    return payload


def find_project_pin(start: Path) -> Optional[tuple[Path, dict[str, Any]]]:
    current = start.expanduser().resolve()
    if current.is_file():
        current = current.parent
    for directory in (current, *current.parents):
        candidate = directory / PROJECT_PIN_RELATIVE_PATH
        if candidate.is_file():
            return candidate, _read_project_pin(candidate)
    return None


def find_sdk_checkout(start: Path) -> Optional[Path]:
    current = start.expanduser().resolve()
    if current.is_file():
        current = current.parent
    for directory in (current, *current.parents):
        if (directory / sdk_manifest.MANIFEST_RELATIVE_PATH).is_file():
            return directory
    return None


def write_project_pin(project: Path, context: SdkContext) -> dict[str, Any]:
    project_root = project.expanduser().resolve()
    if not project_root.is_dir():
        raise SdkResolutionError(f"project directory does not exist: {project_root}")
    if not context.registration_name:
        raise SdkResolutionError("project pin requires a registered SDK")
    path = project_root / PROJECT_PIN_RELATIVE_PATH
    payload = {
        "schema_version": PROJECT_PIN_SCHEMA_VERSION,
        "registration": context.registration_name,
        "sdk_id": context.sdk_id,
        "sdk_version": context.version,
    }
    content = json.dumps(payload, indent=2, ensure_ascii=False, sort_keys=True) + "\n"
    existing = path.read_text(encoding="utf-8") if path.is_file() else None
    if existing == content:
        changed = False
    else:
        path.parent.mkdir(parents=True, exist_ok=True)
        previous_mode = stat.S_IMODE(path.stat().st_mode) if path.exists() else 0o644
        with tempfile.NamedTemporaryFile(
            prefix=f".{path.name}.", dir=path.parent, delete=False
        ) as temporary:
            temporary_path = Path(temporary.name)
            temporary.write(content.encode("utf-8"))
        try:
            temporary_path.chmod(previous_mode)
            temporary_path.replace(path)
        finally:
            temporary_path.unlink(missing_ok=True)
        changed = True
    return {
        "path": str(path),
        "changed": changed,
        "pin": payload,
    }


class SdkResolver:
    def __init__(
        self,
        registry: Optional[SdkRegistry] = None,
        *,
        environ: Optional[Mapping[str, str]] = None,
        checkout_hint: Optional[Path] = None,
    ) -> None:
        self.registry = registry or SdkRegistry()
        self.environ = os.environ if environ is None else environ
        self.checkout_hint = checkout_hint

    def _registered_context(
        self, name: str, entry: dict[str, Any], source: str
    ) -> SdkContext:
        try:
            context = sdk_manifest.context_from_root(
                Path(entry["root"]),
                kind=entry["kind"],
                source=source,
                registration_name=name,
            )
        except sdk_manifest.SdkManifestError as exc:
            raise SdkResolutionError(
                f"SDK selected by {source} is invalid ({name}): {exc}"
            ) from exc
        if context.sdk_id != entry["sdk_id"] or context.version != entry["sdk_version"]:
            raise SdkResolutionError(
                f"SDK selected by {source} no longer matches registration {name}"
            )
        return context

    def _selector_context(self, selector: str) -> SdkContext:
        try:
            name, entry = self.registry.find(selector)
        except SdkRegistrationNotFound:
            candidate = Path(selector).expanduser()
            if candidate.exists():
                candidate = candidate.resolve()
                try:
                    registrations = self.registry.registrations()
                except SdkRegistryError as exc:
                    raise SdkResolutionError(
                        f"cannot inspect SDK registrations for {candidate}: {exc}"
                    ) from exc
                for name, entry in registrations.items():
                    if Path(entry["root"]).expanduser().resolve() == candidate:
                        return self._registered_context(name, entry, "explicit-path")
                try:
                    return sdk_manifest.context_from_root(
                        candidate,
                        kind="checkout",
                        source="explicit-path",
                    )
                except sdk_manifest.SdkManifestError as exc:
                    raise SdkResolutionError(
                        f"SDK selected by --sdk path is invalid: {exc}"
                    ) from exc
            raise SdkResolutionError(
                f"SDK selected by --sdk is not registered and is not a path: {selector}"
            )
        except SdkRegistryError as exc:
            raise SdkResolutionError(str(exc)) from exc
        return self._registered_context(name, entry, "explicit")

    def resolve(
        self,
        *,
        explicit: Optional[str] = None,
        project: Optional[Path] = None,
    ) -> SdkContext:
        if explicit:
            return self._selector_context(explicit)

        project_pin = find_project_pin(project or Path.cwd())
        if project_pin:
            path, pin = project_pin
            try:
                name, entry = self.registry.find(pin["registration"])
            except SdkRegistryError as exc:
                raise SdkResolutionError(
                    f"SDK selected by project pin {path} is unavailable: {exc}"
                ) from exc
            context = self._registered_context(name, entry, f"project:{path}")
            if context.sdk_id != pin["sdk_id"] or context.version != pin["sdk_version"]:
                raise SdkResolutionError(
                    f"SDK selected by project pin {path} does not match its recorded identity"
                )
            return context

        workspace_checkout = find_sdk_checkout(project or Path.cwd())
        if workspace_checkout:
            try:
                return sdk_manifest.context_from_root(
                    workspace_checkout,
                    kind="checkout",
                    source="workspace-checkout",
                )
            except sdk_manifest.SdkManifestError as exc:
                raise SdkResolutionError(
                    f"SDK checkout containing the current path is invalid: {exc}"
                ) from exc

        configured = self.environ.get("ECOS_SDK_HOME")
        if configured:
            try:
                return sdk_manifest.context_from_root(
                    Path(configured),
                    kind="checkout",
                    source="environment:ECOS_SDK_HOME",
                )
            except sdk_manifest.SdkManifestError as exc:
                raise SdkResolutionError(
                    f"SDK selected by ECOS_SDK_HOME is invalid: {exc}"
                ) from exc

        try:
            registry = self.registry.load()
        except SdkRegistryError as exc:
            raise SdkResolutionError(f"cannot use SDK registry: {exc}") from exc
        active = registry["active"]
        if active:
            return self._registered_context(
                active, registry["sdks"][active], "active-registration"
            )

        if self.checkout_hint:
            try:
                return sdk_manifest.context_from_root(
                    self.checkout_hint,
                    kind="checkout",
                    source="checkout-entry",
                )
            except sdk_manifest.SdkManifestError as exc:
                raise SdkResolutionError(
                    f"SDK checkout inferred from CLI entry is invalid: {exc}"
                ) from exc

        raise SdkResolutionError(
            "no SDK is selected; register one with 'ecos sdk register PATH --activate'"
        )
