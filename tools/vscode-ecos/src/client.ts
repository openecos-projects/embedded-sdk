import {spawn} from "node:child_process";
import {CliEnvelope, isUntrustedQuery, parseEnvelope} from "./contract";
import {resolveLauncher} from "./launcher";

export interface CliRun<T = unknown> {
  envelope: CliEnvelope<T>;
  exitCode: number;
}

export class CliClient {
  constructor(private readonly executable: string) {}

  async run<T>(
    command: string, args: readonly string[], cwd: string,
    trusted: boolean, signal?: AbortSignal,
    onLog?: (text: string) => void
  ): Promise<CliRun<T>> {
    if (!trusted && !isUntrustedQuery(args)) {
      throw new Error("Trust this workspace before running ECOS commands that may change files or access devices.");
    }
    const launch = resolveLauncher(this.executable);
    const allArgs = [...launch.prefixArgs, ...args];
    return new Promise((resolve, reject) => {
      const child = spawn(launch.executable, allArgs, {
        cwd, shell: false, windowsHide: true, signal,
      });
      let stdout = "";
      let stderr = "";
      child.stdout.setEncoding("utf8");
      child.stderr.setEncoding("utf8");
      child.stdout.on("data", (chunk: string) => {
        stdout += chunk;
        if (stdout.length > 8_000_000) {
          child.kill();
          reject(new Error("ECOS CLI JSON response exceeded 8 MB."));
        }
      });
      child.stderr.on("data", (chunk: string) => {
        stderr = (stderr + chunk).slice(-65_536);
        onLog?.(chunk);
      });
      child.on("error", reject);
      child.on("close", (code) => {
        try {
          const envelope = parseEnvelope<T>(stdout, command);
          resolve({envelope, exitCode: code ?? 1});
        } catch (error) {
          const output = stdout.trim().slice(0, 300);
          reject(new Error(
            `${String(error)} (exit ${code ?? "unknown"})${stderr ? `\n${stderr}` : ""}` +
            `${output ? `\nCLI stdout: ${output}` : ""}`
          ));
        }
      });
    });
  }
}
