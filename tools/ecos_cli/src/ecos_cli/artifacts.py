"""Firmware artifact inspection, validation, and manifest handling."""

from __future__ import annotations

import hashlib
import json
import re
import struct
import tempfile
from pathlib import Path
from typing import Any, Optional


ARTIFACT_SCHEMA_VERSION = 1
ARTIFACT_MANIFEST_NAME = "artifacts.json"
EM_RISCV = 243
EF_RISCV_RVE = 0x8
PT_LOAD = 1
SHF_ALLOC = 0x2
SHT_SYMTAB = 2
SHT_NOBITS = 8
SHT_DYNSYM = 11
DIGEST_PATTERN = re.compile(r"^[0-9a-f]{64}$")


class ArtifactError(RuntimeError):
    """Base error for firmware artifact handling."""


class ArtifactValidationError(ArtifactError):
    """A produced artifact violates the Target build contract."""


class ArtifactManifestError(ArtifactError):
    """The artifact manifest is missing, stale, or malformed."""


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    try:
        with path.open("rb") as stream:
            for chunk in iter(lambda: stream.read(1024 * 1024), b""):
                digest.update(chunk)
    except OSError as exc:
        raise ArtifactValidationError(f"cannot hash artifact {path}: {exc}") from exc
    return digest.hexdigest()


def _slice(data: bytes, offset: int, size: int, description: str) -> bytes:
    if offset < 0 or size < 0 or offset + size > len(data):
        raise ArtifactValidationError(
            f"ELF {description} extends outside the file (offset={offset}, size={size})"
        )
    return data[offset : offset + size]


def _string(table: bytes, offset: int) -> str:
    if offset < 0 or offset >= len(table):
        return ""
    end = table.find(b"\0", offset)
    if end < 0:
        end = len(table)
    return table[offset:end].decode("utf-8", errors="replace")


def inspect_elf(path: Path) -> dict[str, Any]:
    try:
        data = path.read_bytes()
    except OSError as exc:
        raise ArtifactValidationError(f"cannot read ELF artifact {path}: {exc}") from exc
    if len(data) < 52 or data[:4] != b"\x7fELF":
        raise ArtifactValidationError(f"firmware is not an ELF file: {path}")
    elf_class = data[4]
    encoding = data[5]
    if elf_class != 1:
        raise ArtifactValidationError(
            f"firmware must be ELF32, got ELF class {elf_class}: {path}"
        )
    if encoding not in {1, 2}:
        raise ArtifactValidationError(f"firmware ELF has invalid byte order: {path}")
    endian = "<" if encoding == 1 else ">"
    header = struct.unpack(endian + "HHIIIIIHHHHHH", _slice(data, 16, 36, "header"))
    (
        elf_type,
        machine,
        version,
        entry,
        program_offset,
        section_offset,
        flags,
        header_size,
        program_entry_size,
        program_count,
        section_entry_size,
        section_count,
        section_names_index,
    ) = header
    if header_size < 52 or version != 1:
        raise ArtifactValidationError(f"firmware ELF header is invalid: {path}")
    if machine != EM_RISCV:
        raise ArtifactValidationError(
            f"firmware ELF machine must be RISC-V ({EM_RISCV}), got {machine}: {path}"
        )

    section_headers: list[tuple[int, ...]] = []
    if section_count:
        if section_entry_size < 40:
            raise ArtifactValidationError(f"firmware ELF section headers are invalid: {path}")
        for index in range(section_count):
            raw = _slice(
                data,
                section_offset + index * section_entry_size,
                40,
                f"section header {index}",
            )
            section_headers.append(struct.unpack(endian + "IIIIIIIIII", raw))
    section_names = b""
    if section_headers:
        if section_names_index >= len(section_headers):
            raise ArtifactValidationError(
                f"firmware ELF section name table is invalid: {path}"
            )
        names_header = section_headers[section_names_index]
        section_names = _slice(
            data, names_header[4], names_header[5], "section name table"
        )

    sections: list[dict[str, Any]] = []
    for section in section_headers:
        if section[1] != SHT_NOBITS and section[5]:
            _slice(data, section[4], section[5], "section data")
        sections.append(
            {
                "name": _string(section_names, section[0]),
                "type": section[1],
                "flags": section[2],
                "address": section[3],
                "offset": section[4],
                "size": section[5],
                "link": section[6],
                "entry_size": section[9],
            }
        )

    symbols: dict[str, int] = {}
    for section in sections:
        if section["type"] not in {SHT_SYMTAB, SHT_DYNSYM} or not section["entry_size"]:
            continue
        link = section["link"]
        if link >= len(sections):
            raise ArtifactValidationError(f"firmware ELF symbol table is invalid: {path}")
        string_section = sections[link]
        strings = _slice(
            data,
            string_section["offset"],
            string_section["size"],
            "symbol string table",
        )
        count = section["size"] // section["entry_size"]
        for index in range(count):
            raw = _slice(
                data,
                section["offset"] + index * section["entry_size"],
                16,
                f"symbol {index}",
            )
            name_offset, value, _size, _info, _other, _section = struct.unpack(
                endian + "IIIBBH", raw
            )
            name = _string(strings, name_offset)
            if name:
                symbols.setdefault(name, value)

    segments: list[dict[str, Any]] = []
    if program_count and program_entry_size < 32:
        raise ArtifactValidationError(f"firmware ELF program headers are invalid: {path}")
    for index in range(program_count):
        raw = _slice(
            data,
            program_offset + index * program_entry_size,
            32,
            f"program header {index}",
        )
        (
            segment_type,
            offset,
            virtual_address,
            physical_address,
            file_size,
            memory_size,
            segment_flags,
            alignment,
        ) = struct.unpack(endian + "IIIIIIII", raw)
        if segment_type == PT_LOAD:
            if memory_size < file_size:
                raise ArtifactValidationError(
                    f"firmware ELF load segment {index} has file size larger than memory size"
                )
            if file_size:
                _slice(data, offset, file_size, f"load segment {index}")
            segments.append(
                {
                    "type": segment_type,
                    "offset": offset,
                    "virtual_address": virtual_address,
                    "physical_address": physical_address,
                    "file_size": file_size,
                    "memory_size": memory_size,
                    "flags": segment_flags,
                    "alignment": alignment,
                }
            )

    return {
        "class": "ELF32",
        "endianness": "little" if encoding == 1 else "big",
        "type": elf_type,
        "machine": "RISC-V",
        "machine_id": machine,
        "entry": entry,
        "flags": flags,
        "rve": bool(flags & EF_RISCV_RVE),
        "sections": sections,
        "segments": segments,
        "symbols": symbols,
    }


def _integer(value: Any, field: str) -> int:
    if isinstance(value, bool):
        raise ArtifactValidationError(f"{field} must be an integer")
    if isinstance(value, int):
        return value
    if isinstance(value, str):
        text = value.strip().upper()
        multiplier = 1
        if text.endswith("K"):
            multiplier, text = 1024, text[:-1]
        elif text.endswith("M"):
            multiplier, text = 1024 * 1024, text[:-1]
        try:
            return int(text, 0) * multiplier
        except ValueError as exc:
            raise ArtifactValidationError(f"{field} is not an integer: {value!r}") from exc
    raise ArtifactValidationError(f"{field} must be an integer")


def _memory_regions(resolved: dict[str, Any]) -> list[dict[str, Any]]:
    regions: list[dict[str, Any]] = []
    for name, value in resolved["target"].get("memory", {}).items():
        origin = _integer(value.get("origin"), f"Target.memory.{name}.origin")
        size = _integer(value.get("size"), f"Target.memory.{name}.size")
        if size <= 0:
            raise ArtifactValidationError(f"Target.memory.{name}.size must be positive")
        regions.append({"name": name, "origin": origin, "size": size, "end": origin + size})
    return regions


def _contains(regions: list[dict[str, Any]], address: int, size: int = 1) -> bool:
    end = address + max(size, 1)
    return any(region["origin"] <= address and end <= region["end"] for region in regions)


def _validate_elf(elf: dict[str, Any], resolved: dict[str, Any]) -> dict[str, Any]:
    cpu = resolved["target"]["cpu"]
    march = cpu["march"]
    if march.startswith("rv32") and elf["class"] != "ELF32":
        raise ArtifactValidationError(f"Target {march} requires ELF32 firmware")
    if march.startswith("rv32e") and not elf["rve"]:
        raise ArtifactValidationError(
            "Target rv32e requires the ELF RVE architecture flag"
        )
    start = elf["symbols"].get("_start")
    if start is None:
        raise ArtifactValidationError("firmware ELF does not define _start")
    if elf["entry"] != start:
        raise ArtifactValidationError(
            f"firmware entry 0x{elf['entry']:x} does not match _start 0x{start:x}"
        )
    if "main" not in elf["symbols"]:
        raise ArtifactValidationError("firmware ELF does not define main")
    regions = _memory_regions(resolved)
    if regions and not _contains(regions, elf["entry"]):
        raise ArtifactValidationError(
            f"firmware entry 0x{elf['entry']:x} is outside declared Target memory"
        )
    for section in elf["sections"]:
        if (
            regions
            and section["size"]
            and section["flags"] & SHF_ALLOC
            and not _contains(regions, section["address"], section["size"])
        ):
            raise ArtifactValidationError(
                f"allocatable ELF section {section['name']!r} at "
                f"0x{section['address']:x} is outside declared Target memory"
            )
    return {
        **elf,
        "expected": {"march": march, "abi": cpu["abi"], "entry_symbol": "_start"},
        "memory_regions": regions,
    }


def _artifact_paths(
    build_dir: Path, firmware_name: str, outputs: list[str]
) -> dict[str, Path]:
    paths: dict[str, Path] = {}
    for output in outputs:
        if output == "compile_commands":
            paths[output] = build_dir / "compile_commands.json"
        else:
            paths[output] = build_dir / f"{firmware_name}.{output}"
    return paths


def create_manifest(
    project_root: Path,
    resolved: dict[str, Any],
    *,
    firmware_name: str = "retrosoc_fw",
) -> dict[str, Any]:
    root = project_root.resolve()
    build_dir = root / "build"
    outputs = resolved["build"]["outputs"]
    required_outputs = {"elf", "bin", "hex", "map", "size", "compile_commands"}
    missing_contract = sorted(required_outputs.difference(outputs))
    if missing_contract:
        raise ArtifactValidationError(
            "resolved Target omits required artifact outputs: "
            + ", ".join(missing_contract)
        )
    paths = _artifact_paths(build_dir, firmware_name, outputs)
    missing = sorted(name for name, path in paths.items() if not path.is_file())
    if missing:
        raise ArtifactValidationError(
            f"Target build did not produce required outputs: {', '.join(missing)}"
        )
    for name, path in paths.items():
        if path.stat().st_size <= 0:
            raise ArtifactValidationError(f"build output is empty: {name} ({path})")
    try:
        compile_commands = json.loads(paths["compile_commands"].read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        raise ArtifactValidationError("compile_commands.json is not valid JSON") from exc
    if not isinstance(compile_commands, list) or not compile_commands:
        raise ArtifactValidationError("compile_commands.json must contain compile commands")

    elf = _validate_elf(inspect_elf(paths["elf"]), resolved)
    files: dict[str, dict[str, Any]] = {}
    for name, path in paths.items():
        files[name] = {
            "path": path.relative_to(root).as_posix(),
            "size": path.stat().st_size,
            "sha256": sha256(path),
        }
    image = files["bin"]
    manifest = {
        "schema_version": ARTIFACT_SCHEMA_VERSION,
        "firmware": {
            "name": firmware_name,
            "architecture": resolved["target"]["arch"],
            "isa": resolved["target"]["cpu"]["march"],
            "abi": resolved["target"]["cpu"]["abi"],
            "entry": elf["entry"],
            "entry_symbol": "_start",
            "image": "bin",
            "size": image["size"],
            "sha256": image["sha256"],
        },
        "project": {
            "name": resolved["project"]["name"],
            "board": resolved["project"]["board"],
            "target": resolved["project"]["target"],
            "profile": resolved["project"]["profile"],
        },
        "sdk": {"id": resolved["sdk"]["id"], "version": resolved["sdk"]["version"]},
        "toolchain": {
            "id": resolved["toolchain"]["id"],
            "release": resolved["toolchain"]["release"],
        },
        "source_fingerprint": resolved["source_fingerprint"],
        "configuration_fingerprint": resolved["configuration"]["fingerprint"],
        "files": files,
        "elf": elf,
    }
    manifest_path = build_dir / ARTIFACT_MANIFEST_NAME
    content = json.dumps(manifest, indent=2, ensure_ascii=False, sort_keys=True) + "\n"
    temporary_path: Optional[Path] = None
    try:
        with tempfile.NamedTemporaryFile(
            "w",
            encoding="utf-8",
            newline="\n",
            dir=build_dir,
            prefix=".artifacts.",
            suffix=".tmp",
            delete=False,
        ) as temporary:
            temporary.write(content)
            temporary_path = Path(temporary.name)
        temporary_path.replace(manifest_path)
    except OSError as exc:
        if temporary_path is not None:
            temporary_path.unlink(missing_ok=True)
        raise ArtifactManifestError(
            f"cannot write artifact manifest {manifest_path}: {exc}"
        ) from exc
    return {**manifest, "manifest": str(manifest_path)}


def _validate_manifest_structure(value: Any, path: Path) -> dict[str, Any]:
    if not isinstance(value, dict) or value.get("schema_version") != ARTIFACT_SCHEMA_VERSION:
        raise ArtifactManifestError(f"artifact manifest schema is invalid: {path}")
    for field in ("source_fingerprint", "configuration_fingerprint"):
        digest = value.get(field)
        if not isinstance(digest, str) or not DIGEST_PATTERN.fullmatch(digest):
            raise ArtifactManifestError(f"artifact manifest {field} is invalid: {path}")
    project = value.get("project")
    if (
        not isinstance(project, dict)
        or not isinstance(project.get("name"), str)
        or not project["name"]
        or not isinstance(project.get("target"), str)
        or not project["target"]
        or not isinstance(project.get("board"), (str, type(None)))
        or not isinstance(project.get("profile"), (str, type(None)))
    ):
        raise ArtifactManifestError(f"artifact manifest project identity is invalid: {path}")
    sdk = value.get("sdk")
    if not isinstance(sdk, dict) or not all(
        isinstance(sdk.get(field), str) and sdk[field]
        for field in ("id", "version")
    ):
        raise ArtifactManifestError(f"artifact manifest SDK identity is invalid: {path}")
    toolchain = value.get("toolchain")
    if not isinstance(toolchain, dict) or not all(
        isinstance(toolchain.get(field), str) and toolchain[field]
        for field in ("id", "release")
    ):
        raise ArtifactManifestError(
            f"artifact manifest toolchain identity is invalid: {path}"
        )
    firmware = value.get("firmware")
    firmware_fields = {
        "name": str,
        "architecture": str,
        "isa": str,
        "abi": str,
        "entry": int,
        "entry_symbol": str,
        "image": str,
        "size": int,
        "sha256": str,
    }
    if not isinstance(firmware, dict) or any(
        not isinstance(firmware.get(field), expected)
        for field, expected in firmware_fields.items()
    ):
        raise ArtifactManifestError(f"artifact manifest firmware summary is invalid: {path}")
    if (
        isinstance(firmware["entry"], bool)
        or isinstance(firmware["size"], bool)
        or firmware["entry"] < 0
        or firmware["size"] <= 0
        or not DIGEST_PATTERN.fullmatch(firmware["sha256"])
    ):
        raise ArtifactManifestError(f"artifact manifest firmware digest is invalid: {path}")
    elf = value.get("elf")
    if (
        not isinstance(elf, dict)
        or not isinstance(elf.get("entry"), int)
        or not isinstance(elf.get("sections"), list)
        or not isinstance(elf.get("segments"), list)
        or not isinstance(elf.get("expected"), dict)
        or firmware["entry"] != elf["entry"]
        or firmware["isa"] != elf["expected"].get("march")
        or firmware["abi"] != elf["expected"].get("abi")
    ):
        raise ArtifactManifestError(f"artifact manifest ELF metadata is invalid: {path}")
    files = value.get("files")
    required_files = {"elf", "bin", "hex", "map", "size", "compile_commands"}
    if not isinstance(files, dict) or required_files.difference(files):
        raise ArtifactManifestError(f"artifact manifest files mapping is incomplete: {path}")
    if firmware["image"] not in files:
        raise ArtifactManifestError(f"artifact manifest firmware image is missing: {path}")
    image = files[firmware["image"]]
    if not isinstance(image, dict) or (
        image.get("size") != firmware["size"]
        or image.get("sha256") != firmware["sha256"]
    ):
        raise ArtifactManifestError(
            f"artifact manifest firmware summary does not match its image: {path}"
        )
    return value


def load_manifest(project_root: Path, *, verify: bool = True) -> dict[str, Any]:
    root = project_root.resolve()
    path = root / "build" / ARTIFACT_MANIFEST_NAME
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
    except OSError as exc:
        raise ArtifactManifestError(
            f"artifact manifest does not exist: {path}; run 'ecos build'"
        ) from exc
    except json.JSONDecodeError as exc:
        raise ArtifactManifestError(f"artifact manifest is invalid JSON: {path}") from exc
    value = _validate_manifest_structure(value, path)
    files = value["files"]
    if verify:
        seen_paths: set[Path] = set()
        for name, item in files.items():
            if (
                not isinstance(name, str)
                or not isinstance(item, dict)
                or not isinstance(item.get("path"), str)
                or not isinstance(item.get("size"), int)
                or isinstance(item.get("size"), bool)
                or item["size"] <= 0
                or not isinstance(item.get("sha256"), str)
                or not DIGEST_PATTERN.fullmatch(item["sha256"])
            ):
                raise ArtifactManifestError(f"artifact {name!r} is invalid in {path}")
            artifact_path = (root / item["path"]).resolve()
            try:
                artifact_path.relative_to((root / "build").resolve())
            except ValueError as exc:
                raise ArtifactManifestError(
                    f"artifact {name!r} resolves outside the build directory: {artifact_path}"
                ) from exc
            if artifact_path in seen_paths:
                raise ArtifactManifestError(
                    f"multiple artifacts resolve to the same file: {artifact_path}"
                )
            seen_paths.add(artifact_path)
            if not artifact_path.is_file():
                raise ArtifactManifestError(f"artifact {name!r} is missing: {artifact_path}")
            if (
                artifact_path.stat().st_size != item.get("size")
                or sha256(artifact_path) != item.get("sha256")
            ):
                raise ArtifactManifestError(
                    f"artifact {name!r} does not match its recorded digest: {artifact_path}"
                )
    return {**value, "manifest": str(path)}
