export const CLI_MAJOR = 3;
export const SCHEMA_VERSION = "1.0";

export interface CliDiagnostic {
  code: string;
  severity: "error" | "warning" | "info";
  message: string;
  resource?: string;
  field?: string;
  actual?: unknown;
  expected?: unknown;
  suggestion?: string;
}

export interface CliEnvelope<T = unknown> {
  cli_version: string;
  schema_version: string;
  command: string;
  status: "ok" | "error";
  data: T | null;
  diagnostics: CliDiagnostic[];
}

export class CliContractError extends Error {}

function isRecord(value: unknown): value is Record<string, unknown> {
  return typeof value === "object" && value !== null && !Array.isArray(value);
}

export function parseEnvelope<T>(text: string, command: string): CliEnvelope<T> {
  let value: unknown;
  try {
    value = JSON.parse(text);
  } catch {
    throw new CliContractError(
      "ECOS CLI did not return JSON. Install or upgrade the SDK 3.0 host CLI."
    );
  }
  if (!isRecord(value) || typeof value.cli_version !== "string" ||
      typeof value.schema_version !== "string") {
    throw new CliContractError(
      "ECOS CLI returned an unknown response. Install or upgrade the SDK 3.0 host CLI."
    );
  }
  if (Number(value.cli_version.split(".")[0]) !== CLI_MAJOR ||
      value.schema_version !== SCHEMA_VERSION) {
    throw new CliContractError(
      `ECOS CLI ${value.cli_version} schema ${value.schema_version} is incompatible; ` +
      `install CLI major ${CLI_MAJOR} with schema ${SCHEMA_VERSION}.`
    );
  }
  if (value.command !== command || !["ok", "error"].includes(String(value.status)) ||
      !Array.isArray(value.diagnostics) ||
      !value.diagnostics.every((item: unknown) =>
        isRecord(item) && typeof item.code === "string" &&
        typeof item.message === "string" &&
        ["error", "warning", "info"].includes(String(item.severity)))) {
    throw new CliContractError(`ECOS CLI returned an invalid ${command} response.`);
  }
  return value as unknown as CliEnvelope<T>;
}

export function isUntrustedQuery(args: readonly string[]): boolean {
  if (args[0] === "schema") {
    return args[1] === "list" || args[1] === "describe";
  }
  if (args[0] === "project") {
    return args[1] === "info" || args[1] === "list";
  }
  return ["board", "target", "component", "driver", "device"].includes(args[0] || "") &&
    ["list", "describe"].includes(args[1] || "");
}

export function cliArguments(
  command: string, project: string, extras: readonly string[] = []
): string[] {
  const parts = command.split(".");
  const args = parts.length === 2 ? parts : [command];
  if (["project.info", "project.set-board", "project.set-profile", "validate",
      "configure", "build", "clean", "flash", "monitor", "doctor"].includes(command)) {
    args.push(...extras, "--project", project);
  } else {
    args.push(...extras);
  }
  args.push("--format", "json");
  return args;
}
