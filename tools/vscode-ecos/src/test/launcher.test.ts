import assert from "node:assert/strict";
import * as fs from "node:fs";
import * as os from "node:os";
import * as path from "node:path";
import {test} from "node:test";
import {resolveLauncher} from "../launcher";

test("POSIX launcher is executed directly", () => {
  assert.deepEqual(resolveLauncher("/opt/ECOS SDK/bin/ecos", "linux"), {
    executable: "/opt/ECOS SDK/bin/ecos", prefixArgs: [],
  });
});

test("Windows CMD launcher resolves the published Python argument vector", () => {
  const root = fs.mkdtempSync(path.join(os.tmpdir(), "ECOS path with spaces "));
  try {
    const bin = path.join(root, "bin");
    const share = path.join(root, "share", "ecos");
    fs.mkdirSync(bin);
    fs.mkdirSync(share, {recursive: true});
    const cmd = path.join(bin, "ecos.cmd");
    const script = path.join(bin, "ecos");
    fs.writeFileSync(cmd, "@echo off\n");
    fs.writeFileSync(script, "");
    fs.writeFileSync(path.join(share, "cli-launch.json"), JSON.stringify({
      schema_version: 1,
      python_executable: path.join(root, "Python With Spaces", "python.exe"),
      script,
    }));
    assert.deepEqual(resolveLauncher(cmd, "win32"), {
      executable: path.join(root, "Python With Spaces", "python.exe"),
      prefixArgs: [script],
    });
  } finally {
    fs.rmSync(root, {recursive: true, force: true});
  }
});
