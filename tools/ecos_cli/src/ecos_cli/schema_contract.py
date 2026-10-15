"""Packaged, offline JSON Schema contracts for SDK resource manifests."""

from __future__ import annotations

import json
from functools import lru_cache
from importlib import resources
from pathlib import Path
from typing import Any

import yaml
from jsonschema import Draft202012Validator

from .resources.schemas import SCHEMA_KINDS

SCHEMA_FILES = {kind: f"{kind}.schema.json" for kind in SCHEMA_KINDS}
MANIFEST_FILES = {
    "board": "ecos-board.yml",
    "target": "ecos-soc.yml",
    "component": "ecos-component.yml",
    "example": "ecos-example.yml",
    "sdk": "tools/sdk-manifest.json",
}


class SchemaValidationError(ValueError):
    def __init__(
        self, kind: str, source: Path, field: str, message: str,
        *, actual: Any = None, expected: Any = None,
    ) -> None:
        super().__init__(f"{kind} manifest {source} at {field}: {message}")
        self.kind = kind
        self.source = source
        self.field = field
        self.actual = actual
        self.expected = expected


@lru_cache(maxsize=len(SCHEMA_FILES))
def load_schema(kind: str) -> dict[str, Any]:
    if kind not in SCHEMA_FILES:
        raise ValueError(f"unknown manifest schema: {kind}")
    path = resources.files("ecos_cli.resources.schemas").joinpath(SCHEMA_FILES[kind])
    schema = json.loads(path.read_text(encoding="utf-8"))
    Draft202012Validator.check_schema(schema)
    return schema


@lru_cache(maxsize=len(SCHEMA_FILES))
def _validator(kind: str) -> Draft202012Validator:
    return Draft202012Validator(load_schema(kind))


def list_schemas() -> dict[str, Any]:
    return {
        "schemas": [
            {
                "kind": kind,
                "manifest": MANIFEST_FILES[kind],
                "version": _schema_version(kind),
            }
            for kind in SCHEMA_FILES
        ]
    }


def describe_schema(kind: str) -> dict[str, Any]:
    schema = load_schema(kind)
    return {
        "kind": kind,
        "manifest": MANIFEST_FILES[kind],
        "version": _schema_version(kind),
        "schema": schema,
    }


def validate_manifest(kind: str, value: Any, source: Path) -> None:
    errors = sorted(
        _validator(kind).iter_errors(value),
        key=lambda error: (
            tuple(str(part) for part in error.absolute_path), error.message
        ),
    )
    if not errors:
        return
    error = errors[0]
    pointer = "/" + "/".join(
        str(part).replace("~", "~0").replace("/", "~1")
        for part in error.absolute_path
    )
    try:
        json.dumps(error.instance)
        actual = error.instance
    except (TypeError, ValueError):
        actual = repr(error.instance)
    raise SchemaValidationError(
        kind, source, pointer or "/", error.message,
        actual=actual, expected=error.validator_value,
    )


def validate_file(kind: str, path: Path) -> dict[str, Any]:
    try:
        content = path.read_text(encoding="utf-8")
        value = json.loads(content) if kind == "sdk" else yaml.safe_load(content)
    except (OSError, yaml.YAMLError, json.JSONDecodeError) as exc:
        raise SchemaValidationError(kind, path, "/", str(exc)) from exc
    validate_manifest(kind, value, path)
    return {
        "kind": kind,
        "path": str(path.resolve()),
        "version": _schema_version(kind),
        "valid": True,
    }


def _schema_version(kind: str) -> int:
    field = "schema_version" if kind == "sdk" else "schema"
    return load_schema(kind)["properties"][field]["const"]
