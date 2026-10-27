#!/usr/bin/env python3
"""Opt-in, offline installation of the bundled ECOS Agent Skill.

No Agent settings, SDK registry, tools or hardware are changed. An existing skill
must be managed and unmodified; only --update may replace it with new content.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import shutil
import stat
import sys
import tempfile
from pathlib import Path
from typing import Any, Optional

SKILL_NAME = "ecos-sdk"
SKILL_RELATIVE = Path(".agents/skills") / SKILL_NAME
MARKER = ".ecos-skill-install.json"


class SkillInstallError(RuntimeError):
    pass


def _linked(path: Path) -> bool:
    try:
        info = path.lstat()
    except FileNotFoundError:
        return False
    # Windows junctions are reparse points, including on Python 3.9 where
    # Path.is_junction() is unavailable. Refuse all reparse-point destinations.
    return stat.S_ISLNK(info.st_mode) or bool(
        getattr(info, "st_file_attributes", 0) & getattr(stat, "FILE_ATTRIBUTE_REPARSE_POINT", 0)
    )


def _no_links(path: Path) -> None:
    for part in (path, *path.parents):
        if _linked(part):
            raise SkillInstallError(f"refusing a symlink/junction installation path: {part}")


def _inventory(root: Path, *, source: bool = False) -> dict[str, Any]:
    if _linked(root) or not root.is_dir():
        raise SkillInstallError(f"skill must be a regular directory: {root}")
    files: dict[str, str] = {}
    directories: list[str] = []
    def visit(directory: Path) -> None:
        for path in sorted(directory.iterdir()):
            relative = path.relative_to(root)
            # Never traverse ignored bytecode or a reparse-point directory.
            if source and (path.name == "__pycache__" or path.suffix in {".pyc", ".pyo"}):
                continue
            if _linked(path):
                raise SkillInstallError(f"skill contains a symlink/junction: {path}")
            if relative.as_posix() == MARKER:
                continue
            if path.is_dir():
                directories.append(relative.as_posix())
                visit(path)
            elif path.is_file():
                files[relative.as_posix()] = hashlib.sha256(path.read_bytes()).hexdigest()
            else:
                raise SkillInstallError(f"skill contains a non-regular file: {path}")

    visit(root)
    return {"files": files, "directories": sorted(directories)}


def _source(sdk_root: Path) -> tuple[Path, dict[str, Any]]:
    try:
        manifest = json.loads((sdk_root / "tools/sdk-manifest.json").read_text(encoding="utf-8"))
    except (OSError, ValueError) as exc:
        raise SkillInstallError(f"cannot read SDK manifest at {sdk_root}: {exc}") from exc
    if not isinstance(manifest, dict):
        raise SkillInstallError("SDK manifest must be an object")
    version = manifest.get("sdk_version")
    if (
        manifest.get("schema_version") != 1
        or manifest.get("sdk_id") != "ecos-embedded-sdk"
        or not isinstance(version, str) or version.split(".", 1)[0] != "3"
    ):
        raise SkillInstallError("this installer requires ecos-embedded-sdk major 3 / manifest schema 1")
    source = sdk_root / SKILL_RELATIVE
    _no_links(source)
    if not (source / "SKILL.md").is_file():
        raise SkillInstallError(f"bundled SKILL.md is missing: {source}")
    inventory = _inventory(source, source=True)
    marker = {
        "schema_version": 1, "skill": SKILL_NAME,
        "sdk_id": manifest["sdk_id"], "sdk_version": version,
        **inventory,
    }
    return source, marker


def _installed(destination: Path) -> Optional[dict[str, Any]]:
    if not destination.exists():
        return None
    if not destination.is_dir():
        raise SkillInstallError(f"skill destination is not a directory: {destination}")
    marker_path = destination / MARKER
    if _linked(marker_path):
        raise SkillInstallError(f"installation marker is a symlink/junction: {marker_path}")
    try:
        marker = json.loads(marker_path.read_text(encoding="utf-8"))
    except (OSError, ValueError) as exc:
        raise SkillInstallError(f"unmanaged skill exists; refusing to overwrite: {destination}") from exc
    if (
        not isinstance(marker, dict) or marker.get("schema_version") != 1
        or marker.get("skill") != SKILL_NAME or marker.get("sdk_id") != "ecos-embedded-sdk"
    ):
        raise SkillInstallError(f"invalid skill installation marker: {marker_path}")
    current = _inventory(destination)
    if any(marker.get(key) != current[key] for key in ("files", "directories")):
        raise SkillInstallError(f"installed skill was modified; preserve it before updating: {destination}")
    return marker


def install_skill(
    sdk_root: Path, skills_dir: Path, *, update: bool = False, dry_run: bool = False,
) -> dict[str, Any]:
    sdk_root = sdk_root.expanduser().resolve()
    # abspath is intentional: resolve() would hide symlinked destination parents.
    skills_dir = Path(os.path.abspath(skills_dir.expanduser()))
    destination = skills_dir / SKILL_NAME
    _no_links(destination)
    source, wanted = _source(sdk_root)
    if source == destination or source in destination.parents or destination in source.parents:
        raise SkillInstallError(f"source and destination must not overlap: {destination}")
    installed = _installed(destination)
    same_payload = installed is not None and all(
        installed.get(key) == wanted[key] for key in ("files", "directories")
    )
    action = "reused" if same_payload else ("update" if installed else "install")
    if action == "update" and not update:
        raise SkillInstallError("a different managed skill exists; use --update after reviewing the new skill")
    plan = {
        "skill": SKILL_NAME, "sdk_id": wanted["sdk_id"], "sdk_version": wanted["sdk_version"],
        "source": str(source), "destination": str(destination), "action": action,
        "file_count": len(wanted["files"]), "dry_run": dry_run,
        "changed": not dry_run and action != "reused", "would_change": action != "reused",
    }
    if dry_run or same_payload:
        return plan
    skills_dir.mkdir(parents=True, exist_ok=True)
    work = Path(tempfile.mkdtemp(prefix=".ecos-sdk-install-", dir=skills_dir))
    staged = work / "staged"
    backup = work / "previous"
    preserve_backup = False
    try:
        staged.mkdir()
        for relative in wanted["directories"]:
            (staged / relative).mkdir(parents=True, exist_ok=True)
        for relative in wanted["files"]:
            target = staged / relative
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(source / relative, target)
        if _inventory(staged) != {key: wanted[key] for key in ("files", "directories")}:
            raise SkillInstallError("skill source changed while copying; retry after stopping concurrent edits")
        (staged / MARKER).write_text(json.dumps(wanted, indent=2, sort_keys=True) + "\n", encoding="utf-8")
        # Recheck just before replacement; never adopt a path that appeared/changed.
        _no_links(destination)
        if _installed(destination) != installed:
            raise SkillInstallError("skill destination changed during installation")
        if installed is not None:
            destination.replace(backup)
        try:
            staged.replace(destination)
        except OSError:
            if backup.exists():
                try:
                    backup.replace(destination)
                except OSError as exc:
                    preserve_backup = True
                    raise SkillInstallError(f"cannot restore previous skill; recover it from {backup}: {exc}") from exc
            raise
    finally:
        if not preserve_backup:
            shutil.rmtree(work)
    return plan


def main(argv: Optional[list[str]] = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sdk-root", type=Path, default=Path(__file__).resolve().parent.parent)
    scope = parser.add_mutually_exclusive_group(required=True)
    scope.add_argument("--user", action="store_true", help="install to ~/.agents/skills/ecos-sdk")
    scope.add_argument("--project", type=Path, help="install to PROJECT/.agents/skills/ecos-sdk")
    scope.add_argument("--skills-dir", type=Path, help="custom skills parent directory (ecos-sdk is appended)")
    parser.add_argument("--dry-run", action="store_true", help="preview without creating files/directories")
    parser.add_argument("--update", action="store_true", help="replace only an unmodified managed installation")
    parser.add_argument("--format", choices=("text", "json"), default="text")
    args = parser.parse_args(argv)
    try:
        if args.project is not None:
            project = Path(os.path.abspath(args.project.expanduser()))
            _no_links(project)
            if not project.is_dir():
                raise SkillInstallError(f"project directory does not exist: {project}")
            skills_dir = project / ".agents/skills"
        else:
            skills_dir = Path.home() / ".agents/skills" if args.user else args.skills_dir
        data = install_skill(args.sdk_root, skills_dir, update=args.update, dry_run=args.dry_run)
    except (SkillInstallError, OSError, ValueError) as exc:
        if args.format == "json":
            print(json.dumps({"schema_version": 1, "command": "skill.install", "status": "error", "data": None,
                              "diagnostics": [str(exc)]}, ensure_ascii=True))
        else:
            print(f"Skill installation failed: {exc}", file=sys.stderr)
        return 3
    if args.format == "json":
        print(json.dumps({"schema_version": 1, "command": "skill.install", "status": "ok", "data": data,
                          "diagnostics": []}, indent=2, ensure_ascii=True))
    else:
        print(f"Skill {data['action']}{' (dry run)' if data['dry_run'] else ''}: {data['destination']}")
        print("Agent settings and SDK selection were not changed. Reload/restart your Agent to discover the skill.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
