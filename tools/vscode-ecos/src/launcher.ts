import * as fs from "node:fs";
import * as path from "node:path";

export interface LaunchSpec {
  executable: string;
  prefixArgs: string[];
}

function windowsCandidate(cliPath: string, environment: NodeJS.ProcessEnv): string | undefined {
  const pathVariable = environment.Path || environment.PATH || "";
  const explicit = path.isAbsolute(cliPath) || /[\\/]/.test(cliPath);
  const directories = explicit ? [""] : pathVariable.split(path.delimiter);
  const names = path.extname(cliPath) ? [cliPath] : [`${cliPath}.exe`, `${cliPath}.cmd`];
  for (const directory of directories) {
    for (const name of names) {
      const candidate = directory ? path.join(directory, name) : name;
      if (fs.existsSync(candidate)) {
        return path.resolve(candidate);
      }
    }
  }
  return undefined;
}

export function resolveLauncher(
  cliPath: string, platform: NodeJS.Platform = process.platform,
  environment: NodeJS.ProcessEnv = process.env
): LaunchSpec {
  if (platform !== "win32") {
    return {executable: cliPath, prefixArgs: []};
  }
  const candidate = windowsCandidate(cliPath, environment);
  if (!candidate) {
    return {executable: cliPath, prefixArgs: []};
  }
  if (path.extname(candidate).toLowerCase() !== ".cmd") {
    return {executable: candidate, prefixArgs: []};
  }
  const descriptor = path.resolve(path.dirname(candidate), "..", "share", "ecos", "cli-launch.json");
  let value: unknown;
  try {
    value = JSON.parse(fs.readFileSync(descriptor, "utf8"));
  } catch {
    throw new Error(`ECOS Windows launcher metadata is missing: ${descriptor}. Reinstall the host CLI.`);
  }
  if (typeof value !== "object" || value === null ||
      !('schema_version' in value) || value.schema_version !== 1 ||
      !('python_executable' in value) || typeof value.python_executable !== "string" ||
      !('script' in value) || typeof value.script !== "string" ||
      !path.isAbsolute(value.python_executable) ||
      path.resolve(value.script) !== path.resolve(path.dirname(candidate), "ecos")) {
    throw new Error(`Invalid ECOS Windows launcher metadata: ${descriptor}`);
  }
  return {executable: value.python_executable, prefixArgs: [value.script]};
}
