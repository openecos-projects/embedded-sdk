import assert from "node:assert/strict";
import {test} from "node:test";
import {CliClient} from "../client";
import {cliArguments, isUntrustedQuery, parseEnvelope} from "../contract";

const envelope = {
  cli_version: "3.0.0.dev0",
  schema_version: "1.0",
  command: "project.info",
  status: "ok",
  data: {board: "demo"},
  diagnostics: [],
};

test("CLI JSON fixture is accepted only for the matching command and versions", () => {
  assert.equal(parseEnvelope<{board: string}>(JSON.stringify(envelope), "project.info").data?.board, "demo");
  assert.throws(() => parseEnvelope(JSON.stringify({...envelope, cli_version: "2.9.0"}), "project.info"),
    /incompatible/);
  assert.throws(() => parseEnvelope(JSON.stringify({...envelope, schema_version: "2.0"}), "project.info"),
    /incompatible/);
  assert.throws(() => parseEnvelope(JSON.stringify(envelope), "board.list"), /invalid/);
  assert.throws(() => parseEnvelope("build log\n" + JSON.stringify(envelope), "project.info"),
    /did not return JSON/);
});

test("Restricted Mode permits only documented read-only queries", () => {
  assert.equal(isUntrustedQuery(["schema", "list", "--format", "json"]), true);
  assert.equal(isUntrustedQuery(["schema", "describe", "board"]), true);
  assert.equal(isUntrustedQuery(["project", "info", "--project", "/tmp/demo"]), true);
  assert.equal(isUntrustedQuery(["board", "describe", "demo"]), true);
  for (const args of [
    ["schema", "validate", "board"], ["project", "set-board", "demo"],
    ["toolchain", "status"], ["toolchain", "install"], ["build"], ["flash"], ["monitor"],
  ]) {
    assert.equal(isUntrustedQuery(args), false, args.join(" "));
  }
});

test("project paths remain individual process arguments", () => {
  assert.deepEqual(cliArguments("project.set-board", "C:\\Work Space\\工程", ["demo"]), [
    "project", "set-board", "demo", "--project", "C:\\Work Space\\工程", "--format", "json",
  ]);
});

test("client rejects a mutation before starting a process in Restricted Mode", async () => {
  const client = new CliClient("missing-ecos-executable");
  await assert.rejects(client.run("build", ["build", "--format", "json"], process.cwd(), false),
    /Trust this workspace/);
});

test("client uses argument arrays and the selected working directory", async () => {
  const command = "schema.list";
  const script = [
    "import json,os,sys;",
    "print(json.dumps({'cli_version':'3.0.0.dev0','schema_version':'1.0',",
    "'command':'schema.list','status':'ok','data':{'cwd':os.getcwd(),",
    "'args':sys.argv[1:]},'diagnostics':[]}))",
  ].join("");
  const python = process.platform === "win32" ? "python" : "python3";
  const result = await new CliClient(python).run<{cwd: string; args: string[]}>(
    command, ["-c", script, "path with spaces", "$(literal)"], process.cwd(), true
  );
  assert.equal(result.exitCode, 0);
  assert.equal(result.envelope.data?.cwd, process.cwd());
  assert.deepEqual(result.envelope.data?.args, ["path with spaces", "$(literal)"]);
});
