import * as path from "node:path";
import * as vscode from "vscode";
import {CliClient} from "./client";
import {cliArguments, CliDiagnostic, CliEnvelope} from "./contract";
import {resolveLauncher} from "./launcher";

interface ProjectInfo {
  path: string;
  name: string;
  example: string;
  sdk: {id: string; version: string; kind: string};
  board: string | null;
  target: string;
  profile: string | null;
  configured: boolean;
  built: boolean;
  outputs: {elf?: string};
}

interface BoardCatalog {
  boards: {id: string; name: string; target: string; profiles: string[]}[];
}

interface ExampleCatalog {
  examples: {name: string; supported_boards: string[]}[];
}

interface BoardDescription {
  id: string;
  target: string;
  profiles: string[];
  default_profile: string | null;
  capabilities: string[];
  debug?: {adapter: "cppdbg"; server_address: string; stop_at_entry: boolean} | null;
}

interface ToolchainStatus {
  installation: {state: string; debugger?: string};
  selection: {release: string; id: string};
}

interface ProjectState {
  root: string;
  info?: ProjectInfo;
  toolchain?: ToolchainStatus;
  error?: string;
}

type Operation = "configure" | "build" | "clean" | "flash" | "monitor";
const OPERATIONS: readonly Operation[] = ["configure", "build", "clean", "flash", "monitor"];

class ProjectTree implements vscode.TreeDataProvider<vscode.TreeItem> {
  private readonly change = new vscode.EventEmitter<vscode.TreeItem | undefined>();
  readonly onDidChangeTreeData = this.change.event;

  constructor(private readonly states: Map<string, ProjectState>) {}

  refresh(): void { this.change.fire(undefined); }
  getTreeItem(item: vscode.TreeItem): vscode.TreeItem { return item; }

  getChildren(item?: vscode.TreeItem): vscode.TreeItem[] {
    if (!item) {
      return [...this.states.values()].map((state) => {
        const node = new vscode.TreeItem(
          state.info?.name || path.basename(state.root),
          vscode.TreeItemCollapsibleState.Expanded
        );
        node.id = state.root;
        node.description = state.info?.board || (state.error ? "Unavailable" : "No Board");
        node.tooltip = state.root;
        node.iconPath = new vscode.ThemeIcon("circuit-board");
        return node;
      });
    }
    const state = this.states.get(item.id || "");
    if (!state) { return []; }
    if (!state.info) { return [this.detail("Status", state.error || "Unknown")]; }
    const info = state.info;
    return [
      this.detail("SDK", `${info.sdk.version} (${info.sdk.kind})`),
      this.detail("Board", info.board || "None"),
      this.detail("Target", info.target),
      this.detail("Profile", info.profile || "Default"),
      this.detail("Toolchain", state.toolchain?.installation.state || "Not checked"),
      this.detail("Build", info.built ? "Current" : info.configured ? "Needs build" : "Needs configure"),
    ];
  }

  private detail(label: string, value: string): vscode.TreeItem {
    const item = new vscode.TreeItem(label);
    item.description = value;
    return item;
  }
}

class EcosWorkspace implements vscode.TaskProvider {
  private readonly states = new Map<string, ProjectState>();
  private readonly tree = new ProjectTree(this.states);
  private readonly diagnostics = vscode.languages.createDiagnosticCollection("ecos");
  private readonly issues = new Map<string, Map<string, CliDiagnostic[]>>();
  private readonly channels = new Map<string, vscode.OutputChannel>();
  private readonly status = vscode.window.createStatusBarItem(vscode.StatusBarAlignment.Left, 90);

  constructor(private readonly context: vscode.ExtensionContext) {
    context.subscriptions.push(this.diagnostics, this.status);
    context.subscriptions.push(vscode.window.registerTreeDataProvider("ecos.projects", this.tree));
    context.subscriptions.push(vscode.tasks.registerTaskProvider("ecos", this));
    context.subscriptions.push(vscode.workspace.onDidChangeWorkspaceFolders(() => void this.refresh()));
    context.subscriptions.push(vscode.workspace.onDidGrantWorkspaceTrust(() => void this.refresh()));
    context.subscriptions.push(vscode.workspace.onDidChangeConfiguration((event) => {
      if (event.affectsConfiguration("ecos.cliPath")) { void this.refresh(); }
    }));
    this.status.command = "ecos.refresh";
    this.status.tooltip = "Refresh ECOS projects";
  }

  private channel(root: string): vscode.OutputChannel {
    let channel = this.channels.get(root);
    if (!channel) {
      channel = vscode.window.createOutputChannel(`ECOS: ${path.basename(root)}`);
      this.channels.set(root, channel);
      this.context.subscriptions.push(channel);
    }
    return channel;
  }

  private client(root: string): CliClient {
    const configured = vscode.workspace.getConfiguration("ecos", vscode.Uri.file(root))
      .get<string>("cliPath", "ecos");
    return new CliClient(configured);
  }

  private publish(root: string, command: string, items: readonly CliDiagnostic[]): void {
    const commands = this.issues.get(root) || new Map<string, CliDiagnostic[]>();
    commands.set(command, [...items]);
    this.issues.set(root, commands);
    this.renderDiagnostics();
  }

  private renderDiagnostics(): void {
    const byUri = new Map<string, vscode.Diagnostic[]>();
    for (const [root, commands] of this.issues) {
      for (const items of commands.values()) {
        for (const item of items) {
          const filename = item.resource && path.isAbsolute(item.resource)
            ? item.resource : path.join(root, ".ecos", "project.yml");
          const uri = vscode.Uri.file(filename);
          const detail = [item.field, item.message, item.suggestion].filter(Boolean).join("\n");
          const diagnostic = new vscode.Diagnostic(
            new vscode.Range(0, 0, 0, 1), detail,
            item.severity === "error" ? vscode.DiagnosticSeverity.Error :
              item.severity === "warning" ? vscode.DiagnosticSeverity.Warning :
                vscode.DiagnosticSeverity.Information
          );
          diagnostic.code = item.code;
          diagnostic.source = "ECOS";
          const key = uri.toString();
          byUri.set(key, [...(byUri.get(key) || []), diagnostic]);
        }
      }
    }
    this.diagnostics.clear();
    for (const [key, values] of byUri) {
      this.diagnostics.set(vscode.Uri.parse(key), values);
    }
  }

  private async invoke<T>(
    root: string, command: string, extras: readonly string[] = [],
    signal?: AbortSignal, showOutput = false
  ): Promise<CliEnvelope<T>> {
    const args = cliArguments(command, root, extras);
    const channel = this.channel(root);
    if (showOutput) { channel.show(true); }
    const result = await this.client(root).run<T>(
      command, args, root, vscode.workspace.isTrusted, signal,
      (message) => channel.append(message)
    );
    if (result.envelope.diagnostics.length ||
        ["project.info", "toolchain.status", "validate", "configure", "build", "clean",
          "flash", "project.set-board", "project.set-profile", "toolchain.install"].includes(command)) {
      this.publish(root, command, result.envelope.diagnostics);
    }
    if (result.exitCode !== 0 || result.envelope.status !== "ok") {
      const message = result.envelope.diagnostics.map((item) => item.message).join("\n") ||
        `ECOS ${command} failed with exit code ${result.exitCode}.`;
      throw new Error(message);
    }
    return result.envelope;
  }

  private async compatible(root: string): Promise<void> {
    await this.invoke(root, "schema.list");
  }

  async refresh(): Promise<void> {
    const manifests = await vscode.workspace.findFiles("**/.ecos/project.yml", "**/{build,node_modules}/**");
    const roots = [...new Set(manifests.map((uri) => path.dirname(path.dirname(uri.fsPath))))].sort();
    this.states.clear();
    for (const root of this.issues.keys()) {
      if (!roots.includes(root)) { this.issues.delete(root); }
    }
    this.renderDiagnostics();
    for (const root of roots) {
      const state: ProjectState = {root};
      this.states.set(root, state);
      try {
        await this.compatible(root);
        const result = await this.invoke<ProjectInfo>(root, "project.info");
        state.info = result.data || undefined;
        if (vscode.workspace.isTrusted) {
          const toolchain = await this.invoke<ToolchainStatus>(root, "toolchain.status");
          state.toolchain = toolchain.data || undefined;
        }
      } catch (error) {
        state.error = String(error);
      }
    }
    this.tree.refresh();
    const active = this.activeProject();
    if (active?.info) {
      this.status.text = `$(circuit-board) ECOS ${active.info.board || active.info.target}`;
      this.status.show();
    } else {
      this.status.hide();
    }
  }

  private activeProject(): ProjectState | undefined {
    const activePath = vscode.window.activeTextEditor?.document.uri.fsPath;
    if (activePath) {
      const match = [...this.states.values()].sort((a, b) => b.root.length - a.root.length).find((state) =>
        activePath === state.root || activePath.startsWith(state.root + path.sep));
      if (match) { return match; }
    }
    return this.states.size === 1 ? [...this.states.values()][0] : undefined;
  }

  private async chooseProject(): Promise<ProjectState | undefined> {
    const active = this.activeProject();
    if (active) { return active; }
    const choices = [...this.states.values()].map((state) => ({
      label: state.info?.name || path.basename(state.root),
      description: state.root,
      state,
    }));
    const selected = await vscode.window.showQuickPick(choices, {placeHolder: "ECOS project"});
    return selected?.state;
  }

  private requireTrusted(): void {
    if (!vscode.workspace.isTrusted) {
      throw new Error("Trust this workspace to change an ECOS project or run tools.");
    }
  }

  private async withProgress<T>(
    root: string, command: string, extras: readonly string[] = []
  ): Promise<CliEnvelope<T>> {
    this.requireTrusted();
    await this.compatible(root);
    const controller = new AbortController();
    return vscode.window.withProgress(
      {location: vscode.ProgressLocation.Notification, title: `ECOS ${command}`, cancellable: true},
      async (_progress, token) => {
        const subscription = token.onCancellationRequested(() => controller.abort());
        try {
          const result = await this.invoke<T>(root, command, extras, controller.signal, true);
          if (command === "project.set-board" || command === "project.set-profile") {
            this.issues.delete(root);
            this.renderDiagnostics();
          }
          return result;
        } finally {
          subscription.dispose();
          await this.refresh();
        }
      }
    );
  }

  async selectBoard(): Promise<void> {
    this.requireTrusted();
    const state = await this.chooseProject();
    if (!state?.info) { return; }
    const [catalog, examples] = await Promise.all([
      this.invoke<BoardCatalog>(state.root, "board.list"),
      this.invoke<ExampleCatalog>(state.root, "project.list"),
    ]);
    const example = examples.data?.examples.find((item) => item.name === state.info?.example);
    const supported = new Set(example?.supported_boards || []);
    const choices = (catalog.data?.boards || []).filter((board) => supported.has(board.id)).map((board) => ({
      label: board.name,
      description: board.id,
      detail: `Compatible with ${state.info?.example}; Target ${board.target}`,
      id: board.id,
    }));
    const selected = await vscode.window.showQuickPick(choices, {placeHolder: "Compatible Boards"});
    if (!selected || selected.id === state.info.board) { return; }
    const board = await this.invoke<BoardDescription>(state.root, "board.describe", [selected.id]);
    await this.invoke(state.root, "project.set-board", [selected.id, "--dry-run"]);
    const confirmed = await vscode.window.showInformationMessage(
      `${selected.id} is compatible with ${state.info.example}. Target: ${board.data?.target}. Switch Board?`,
      {modal: true}, "Switch Board"
    );
    if (confirmed === "Switch Board") {
      await this.withProgress(state.root, "project.set-board", [selected.id]);
    }
  }

  async selectProfile(): Promise<void> {
    this.requireTrusted();
    const state = await this.chooseProject();
    if (!state?.info?.board) { return; }
    const board = await this.invoke<BoardDescription>(state.root, "board.describe", [state.info.board]);
    const selected = await vscode.window.showQuickPick(board.data?.profiles || [], {
      placeHolder: "Board profile",
    });
    if (selected && selected !== state.info.profile) {
      await this.invoke(state.root, "project.set-profile", [selected, "--dry-run"]);
      await this.withProgress(state.root, "project.set-profile", [selected]);
    }
  }

  async runOperation(operation: Exclude<Operation, "monitor">): Promise<void> {
    const state = await this.chooseProject();
    if (state?.info) { await this.withProgress(state.root, operation); }
  }

  async monitor(): Promise<void> {
    this.requireTrusted();
    const state = await this.chooseProject();
    if (!state?.info) { return; }
    await this.compatible(state.root);
    await vscode.tasks.executeTask(this.makeTask("monitor", state.root));
  }

  async toolchainStatus(): Promise<void> {
    this.requireTrusted();
    const state = await this.chooseProject();
    if (!state) { return; }
    const response = await this.invoke<ToolchainStatus>(state.root, "toolchain.status");
    const data = response.data;
    if (data) {
      vscode.window.showInformationMessage(
        `${data.selection.id} ${data.selection.release}: ${data.installation.state}`
      );
    }
  }

  async toolchainInstall(): Promise<void> {
    this.requireTrusted();
    const state = await this.chooseProject();
    if (!state) { return; }
    const response = await this.invoke<ToolchainStatus>(state.root, "toolchain.status");
    const data = response.data;
    if (!data || data.installation.state === "installed") { return; }
    const choice = await vscode.window.showInformationMessage(
      `Install ${data.selection.id} ${data.selection.release}?`,
      {modal: true}, "Install Toolchain"
    );
    if (choice === "Install Toolchain") {
      await this.withProgress(state.root, "toolchain.install");
    }
  }

  async showSchema(): Promise<void> {
    const selected = await vscode.window.showQuickPick(
      ["board", "target", "component", "example"], {placeHolder: "Manifest schema"}
    );
    if (!selected) { return; }
    const root = (await this.chooseProject())?.root || vscode.workspace.workspaceFolders?.[0]?.uri.fsPath;
    if (!root) { return; }
    const response = await this.invoke(root, "schema.describe", [selected]);
    const channel = this.channel(root);
    channel.appendLine(JSON.stringify(response.data, null, 2));
    channel.show(true);
  }

  async debug(): Promise<void> {
    this.requireTrusted();
    const state = await this.chooseProject();
    if (!state?.info) { return; }
    await this.compatible(state.root);
    if (!state.info.built || !state.info.outputs.elf) {
      throw new Error("Build the ECOS project before starting a debug session.");
    }
    const resource = state.info.board ? "board.describe" : "target.describe";
    const selected = state.info.board || state.info.target;
    const described = await this.invoke<BoardDescription>(state.root, resource, [selected]);
    const configuration = described.data?.debug;
    if (!configuration) {
      throw new Error(`${selected} does not declare a debug server configuration.`);
    }
    const toolchain = await this.invoke<ToolchainStatus>(state.root, "toolchain.status");
    const debuggerPath = toolchain.data?.installation.debugger;
    if (!debuggerPath) {
      throw new Error("The selected toolchain has no verified GDB executable.");
    }
    if (!vscode.extensions.getExtension("ms-vscode.cpptools")) {
      throw new Error("The C/C++ debug adapter is unavailable; install the Microsoft C/C++ extension.");
    }
    const folder = vscode.workspace.getWorkspaceFolder(vscode.Uri.file(state.root));
    const started = await vscode.debug.startDebugging(folder, {
      name: `ECOS ${selected}`,
      type: configuration.adapter,
      request: "launch",
      program: state.info.outputs.elf,
      cwd: state.root,
      MIMode: "gdb",
      miDebuggerPath: debuggerPath,
      miDebuggerServerAddress: configuration.server_address,
      stopAtEntry: configuration.stop_at_entry,
      launchCompleteCommand: "None",
    });
    if (!started) { throw new Error("VS Code could not start the ECOS debug session."); }
  }

  private makeTask(operation: Operation, root: string): vscode.Task {
    const config = vscode.workspace.getConfiguration("ecos", vscode.Uri.file(root));
    const launcher = resolveLauncher(config.get<string>("cliPath", "ecos"));
    const args = operation === "monitor"
      ? ["monitor", "--project", root]
      : [operation, "--project", root];
    const folder = vscode.workspace.getWorkspaceFolder(vscode.Uri.file(root));
    const task = new vscode.Task(
      {type: "ecos", operation, project: root}, folder || vscode.TaskScope.Workspace,
      `ECOS ${operation}: ${path.basename(root)}`, "ecos",
      new vscode.ProcessExecution(launcher.executable, [...launcher.prefixArgs, ...args], {cwd: root})
    );
    if (operation === "build") { task.group = vscode.TaskGroup.Build; }
    return task;
  }

  async provideTasks(): Promise<vscode.Task[]> {
    if (!vscode.workspace.isTrusted) { return []; }
    const tasks: vscode.Task[] = [];
    for (const state of this.states.values()) {
      if (!state.info) { continue; }
      try {
        await this.compatible(state.root);
        for (const operation of OPERATIONS) {
          tasks.push(this.makeTask(operation, state.root));
        }
      } catch (error) {
        this.channel(state.root).appendLine(String(error));
      }
    }
    return tasks;
  }

  async resolveTask(task: vscode.Task): Promise<vscode.Task | undefined> {
    if (!vscode.workspace.isTrusted) { return undefined; }
    const {operation, project} = task.definition;
    if (typeof project !== "string" || !this.states.get(project)?.info ||
        !OPERATIONS.includes(operation as Operation)) {
      return undefined;
    }
    await this.compatible(project);
    return this.makeTask(operation as Operation, project);
  }
}

export async function activate(context: vscode.ExtensionContext): Promise<void> {
  const workspace = new EcosWorkspace(context);
  const commands: Record<string, () => Promise<void>> = {
    "ecos.refresh": () => workspace.refresh(),
    "ecos.selectBoard": () => workspace.selectBoard(),
    "ecos.selectProfile": () => workspace.selectProfile(),
    "ecos.configure": () => workspace.runOperation("configure"),
    "ecos.build": () => workspace.runOperation("build"),
    "ecos.clean": () => workspace.runOperation("clean"),
    "ecos.flash": () => workspace.runOperation("flash"),
    "ecos.monitor": () => workspace.monitor(),
    "ecos.toolchainStatus": () => workspace.toolchainStatus(),
    "ecos.toolchainInstall": () => workspace.toolchainInstall(),
    "ecos.showSchema": () => workspace.showSchema(),
    "ecos.debug": () => workspace.debug(),
  };
  for (const [name, command] of Object.entries(commands)) {
    context.subscriptions.push(vscode.commands.registerCommand(name, async () => {
      try { await command(); }
      catch (error) { vscode.window.showErrorMessage(String(error)); }
    }));
  }
  await workspace.refresh();
}

export function deactivate(): void {}
