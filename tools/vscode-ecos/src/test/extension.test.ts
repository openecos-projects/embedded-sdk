import assert from "node:assert/strict";
import Module from "node:module";
import {test} from "node:test";
import * as path from "node:path";
import {CliClient} from "../client";

test("extension enforces trust and binds tasks and Board changes to each project", async () => {
  const roots = [
    path.join(process.cwd(), "Workspace One", "工程"),
    path.join(process.cwd(), "Workspace Two"),
  ];
  const commands = new Map<string, () => Promise<void>>();
  const errors: string[] = [];
  const calls: {command: string; args: readonly string[]; cwd: string; trusted: boolean}[] = [];
  let provider: {provideTasks(): Promise<unknown[]>} | undefined;

  class EventEmitter {
    event = () => ({dispose() {}});
    fire(): void {}
  }
  class TreeItem {
    id?: string;
    description?: string;
    tooltip?: string;
    iconPath?: unknown;
    constructor(public readonly label: string, public readonly collapsibleState?: number) {}
  }
  class ProcessExecution {
    constructor(public readonly process: string, public readonly args: string[],
      public readonly options: {cwd: string}) {}
  }
  class Task {
    group?: unknown;
    constructor(public readonly definition: {project: string; operation: string},
      public readonly scope: unknown, public readonly name: string,
      public readonly source: string, public readonly execution: ProcessExecution) {}
  }
  const fake = {
    workspace: {
      isTrusted: false,
      findFiles: async () => roots.map((root) => ({fsPath: path.join(root, ".ecos", "project.yml")})),
      getConfiguration: () => ({get: (_name: string, fallback: string) => fallback}),
      getWorkspaceFolder: (uri: {fsPath: string}) => ({uri, name: path.basename(uri.fsPath), index: 0}),
      onDidChangeWorkspaceFolders: () => ({dispose() {}}),
      onDidGrantWorkspaceTrust: () => ({dispose() {}}),
      onDidChangeConfiguration: () => ({dispose() {}}),
    },
    window: {
      activeTextEditor: undefined as {document: {uri: {fsPath: string}}} | undefined,
      registerTreeDataProvider: () => ({dispose() {}}),
      createStatusBarItem: () => ({show() {}, hide() {}, dispose() {}}),
      createOutputChannel: () => ({append() {}, appendLine() {}, show() {}, dispose() {}}),
      showQuickPick: async (items: unknown[]) => items[0],
      showInformationMessage: async () => "Switch Board",
      showErrorMessage: (message: string) => { errors.push(message); },
      withProgress: async (_options: unknown, action: (progress: unknown, token: unknown) => Promise<unknown>) =>
        action({}, {onCancellationRequested: () => ({dispose() {}})}),
    },
    commands: {
      registerCommand: (name: string, command: () => Promise<void>) => {
        commands.set(name, command);
        return {dispose() {}};
      },
    },
    tasks: {
      registerTaskProvider: (_type: string, value: typeof provider) => {
        provider = value;
        return {dispose() {}};
      },
      executeTask: async () => {},
    },
    languages: {
      createDiagnosticCollection: () => ({clear() {}, set() {}, dispose() {}}),
    },
    Uri: {
      file: (filename: string) => ({fsPath: filename, toString: () => `file://${filename}`}),
      parse: (value: string) => ({fsPath: value.slice(7), toString: () => value}),
    },
    EventEmitter,
    TreeItem,
    ThemeIcon: class { constructor(public readonly id: string) {} },
    ProcessExecution,
    Task,
    TaskScope: {Workspace: 1},
    TaskGroup: {Build: 1},
    TreeItemCollapsibleState: {Expanded: 2},
    StatusBarAlignment: {Left: 1},
    ProgressLocation: {Notification: 1},
    Diagnostic: class { code?: string; source?: string;
      constructor(public readonly range: unknown, public readonly message: string,
        public readonly severity: number) {} },
    DiagnosticSeverity: {Error: 0, Warning: 1, Information: 2},
    Range: class { constructor(..._args: number[]) {} },
  };

  const moduleWithLoad = Module as unknown as {
    _load(request: string, parent: unknown, isMain: boolean): unknown;
  };
  const originalLoad = moduleWithLoad._load;
  moduleWithLoad._load = function(request, parent, isMain) {
    return request === "vscode" ? fake : originalLoad.call(this, request, parent, isMain);
  };
  let activate: (context: any) => Promise<void>;
  try {
    ({activate} = require("../extension") as typeof import("../extension"));
  } finally {
    moduleWithLoad._load = originalLoad;
  }

  const originalRun = CliClient.prototype.run;
  CliClient.prototype.run = (async function(command, args, cwd, trusted) {
    calls.push({command, args, cwd, trusted});
    const data: Record<string, unknown> = {
      "schema.list": {schemas: []},
      "project.info": {
        path: cwd, name: path.basename(cwd), example: "hello",
        sdk: {id: "ecos-embedded-sdk", version: "3.0.0", kind: "checkout"},
        board: "old", target: "soc", profile: "default", configured: true,
        built: false, outputs: {},
      },
      "toolchain.status": {selection: {id: "xpack", release: "15.2.0-1"},
        installation: {state: "installed"}},
      "board.list": {boards: [{id: "new", name: "New Board", target: "soc", profiles: []}]},
      "project.list": {examples: [{name: "hello", supported_boards: ["new"]}]},
      "board.describe": {id: "new", target: "soc", profiles: [],
        default_profile: null, capabilities: []},
      "project.set-board": {board: "new", target: "soc", changed: true},
    };
    return {exitCode: 0, envelope: {
      cli_version: "3.0.0.dev0", schema_version: "1.0", command,
      status: "ok", data: data[command] || {}, diagnostics: [],
    }};
  }) as typeof originalRun;
  try {
    await activate({subscriptions: []});
    assert.deepEqual(calls.map((call) => call.command), [
      "schema.list", "project.info", "schema.list", "project.info",
    ]);
    assert.equal((await provider?.provideTasks())?.length, 0);
    await commands.get("ecos.selectBoard")?.();
    assert.match(errors[0], /Trust this workspace/);
    assert.equal(calls.some((call) => call.command === "project.set-board"), false);

    fake.workspace.isTrusted = true;
    await commands.get("ecos.refresh")?.();
    const tasks = await provider?.provideTasks() as Task[];
    assert.equal(tasks.length, 10);
    for (const task of tasks) {
      assert.equal(task.execution.options.cwd, task.definition.project);
      assert.deepEqual(task.execution.args.slice(-2), ["--project", task.definition.project]);
    }

    fake.window.activeTextEditor = {document: {uri: {fsPath: path.join(roots[0], "main.c")}}};
    await commands.get("ecos.selectBoard")?.();
    const selections = calls.filter((call) => call.command === "project.set-board");
    assert.equal(selections.length, 2);
    assert.equal(selections[0].cwd, roots[0]);
    assert.equal(selections[0].args.includes("--dry-run"), true);
    assert.equal(selections[1].args.includes("--dry-run"), false);
  } finally {
    CliClient.prototype.run = originalRun;
  }
});
