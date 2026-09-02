"""Shell completion generators for the ECOS command-line interface."""

from __future__ import annotations

from typing import Callable


SUPPORTED_SHELLS = ("bash", "zsh", "fish", "powershell")
COMPLETION_FILENAMES = {
    "bash": "ecos.bash",
    "zsh": "_ecos",
    "fish": "ecos.fish",
    "powershell": "ecos.ps1",
}

ROOT_COMMANDS = (
    "sdk",
    "toolchain",
    "project",
    "validate",
    "configure",
    "menuconfig",
    "build",
    "flash",
    "monitor",
    "completion",
    "help",
)
TOOLCHAIN_COMMANDS = ("detect", "status", "install")
SDK_COMMANDS = ("register", "list", "current", "use", "pin", "unregister", "doctor")
PROJECT_COMMANDS = ("create", "set-board", "set-target")


def _bash() -> str:
    roots = " ".join(ROOT_COMMANDS)
    toolchains = " ".join(TOOLCHAIN_COMMANDS)
    sdks = " ".join(SDK_COMMANDS)
    projects = " ".join(PROJECT_COMMANDS)
    return f'''# ECOS completion for Bash
_ecos_complete() {{
    local cur cmd sub candidates
    cur="${{COMP_WORDS[COMP_CWORD]}}"
    cmd="${{COMP_WORDS[1]-}}"
    sub="${{COMP_WORDS[2]-}}"

    if (( COMP_CWORD == 1 )); then
        candidates="{roots}"
    elif [[ "$cmd" == "toolchain" && $COMP_CWORD -eq 2 ]]; then
        candidates="{toolchains}"
    elif [[ "$cmd" == "sdk" && $COMP_CWORD -eq 2 ]]; then
        candidates="{sdks}"
    elif [[ "$cmd" == "project" && $COMP_CWORD -eq 2 ]]; then
        candidates="{projects}"
    elif [[ "$cmd" == "completion" && $COMP_CWORD -eq 2 ]]; then
        candidates="bash zsh fish powershell"
    elif [[ "$cmd" == "build" && "$cur" == -* ]]; then
        candidates="--project --clean --format --help"
    elif [[ "$cmd" == "validate" && "$cur" == -* ]]; then
        candidates="--project --format --help"
    elif [[ "$cmd" == "configure" && "$cur" == -* ]]; then
        candidates="--project --dry-run --format --help"
    elif [[ "$cmd" == "menuconfig" && "$cur" == -* ]]; then
        candidates="--project --help"
    elif [[ "$cmd" == "flash" && "$cur" == -* ]]; then
        candidates="--project --device --format --help"
    elif [[ "$cmd" == "monitor" && "$cur" == -* ]]; then
        candidates="--project --port --baudrate --timeout --expect --format --help"
    elif [[ "$cmd" == "toolchain" && "$cur" == -* ]]; then
        case "$sub" in
            detect) candidates="--format --help" ;;
            status) candidates="--format --prefix --custom --help" ;;
            install) candidates="--format --prefix --cache-dir --archive --force --dry-run --help" ;;
        esac
    elif [[ "$cmd" == "sdk" && "$cur" == -* ]]; then
        case "$sub" in
            register) candidates="--name --kind --activate --replace --format --help" ;;
            current) candidates="--project --sdk --format --help" ;;
            pin) candidates="--project --format --help" ;;
            *) candidates="--format --help" ;;
        esac
    elif [[ "$cmd" == "project" && "$cur" == -* ]]; then
        case "$sub" in
            create) candidates="--name --path --board --target --profile --dry-run --force --format --help" ;;
            set-board|set-target) candidates="--project --format --help" ;;
        esac
    else
        return 0
    fi
    COMPREPLY=( $(compgen -W "$candidates" -- "$cur") )
}}
complete -F _ecos_complete ecos
'''


def _zsh() -> str:
    roots = " ".join(ROOT_COMMANDS)
    toolchains = " ".join(TOOLCHAIN_COMMANDS)
    sdks = " ".join(SDK_COMMANDS)
    projects = " ".join(PROJECT_COMMANDS)
    return f'''#compdef ecos
# ECOS completion for Zsh
_ecos() {{
    local -a candidates
    if (( CURRENT == 2 )); then
        candidates=({roots})
    elif [[ "$words[2]" == "toolchain" && $CURRENT -eq 3 ]]; then
        candidates=({toolchains})
    elif [[ "$words[2]" == "sdk" && $CURRENT -eq 3 ]]; then
        candidates=({sdks})
    elif [[ "$words[2]" == "project" && $CURRENT -eq 3 ]]; then
        candidates=({projects})
    elif [[ "$words[2]" == "completion" && $CURRENT -eq 3 ]]; then
        candidates=(bash zsh fish powershell)
    elif [[ "$words[2]" == "build" && "$PREFIX" == -* ]]; then
        candidates=(--project --clean --format --help)
    elif [[ "$words[2]" == "validate" && "$PREFIX" == -* ]]; then
        candidates=(--project --format --help)
    elif [[ "$words[2]" == "configure" && "$PREFIX" == -* ]]; then
        candidates=(--project --dry-run --format --help)
    elif [[ "$words[2]" == "menuconfig" && "$PREFIX" == -* ]]; then
        candidates=(--project --help)
    elif [[ "$words[2]" == "flash" && "$PREFIX" == -* ]]; then
        candidates=(--project --device --format --help)
    elif [[ "$words[2]" == "monitor" && "$PREFIX" == -* ]]; then
        candidates=(--project --port --baudrate --timeout --expect --format --help)
    elif [[ "$words[2]" == "toolchain" && "$PREFIX" == -* ]]; then
        case "$words[3]" in
            detect) candidates=(--format --help) ;;
            status) candidates=(--format --prefix --custom --help) ;;
            install) candidates=(--format --prefix --cache-dir --archive --force --dry-run --help) ;;
        esac
    elif [[ "$words[2]" == "sdk" && "$PREFIX" == -* ]]; then
        case "$words[3]" in
            register) candidates=(--name --kind --activate --replace --format --help) ;;
            current) candidates=(--project --sdk --format --help) ;;
            pin) candidates=(--project --format --help) ;;
            *) candidates=(--format --help) ;;
        esac
    elif [[ "$words[2]" == "project" && "$PREFIX" == -* ]]; then
        case "$words[3]" in
            create) candidates=(--name --path --board --target --profile --dry-run --force --format --help) ;;
            set-board|set-target) candidates=(--project --format --help) ;;
        esac
    else
        return 0
    fi
    compadd -- $candidates
}}
compdef _ecos ecos
'''


def _fish() -> str:
    root_lines = "\n".join(
        f"complete -c ecos -f -n '__fish_use_subcommand' -a {command}"
        for command in ROOT_COMMANDS
    )
    toolchain_lines = "\n".join(
        f"complete -c ecos -f -n '__fish_seen_subcommand_from toolchain' -a {command}"
        for command in TOOLCHAIN_COMMANDS
    )
    sdk_lines = "\n".join(
        f"complete -c ecos -f -n '__fish_seen_subcommand_from sdk' -a {command}"
        for command in SDK_COMMANDS
    )
    project_lines = "\n".join(
        f"complete -c ecos -f -n '__fish_seen_subcommand_from project' -a {command}"
        for command in PROJECT_COMMANDS
    )
    completion_lines = "\n".join(
        f"complete -c ecos -f -n '__fish_seen_subcommand_from completion' -a {shell}"
        for shell in SUPPORTED_SHELLS
    )
    return f'''# ECOS completion for Fish
{root_lines}
{toolchain_lines}
{sdk_lines}
{project_lines}
{completion_lines}
complete -c ecos -f -n '__fish_seen_subcommand_from sdk; and __fish_seen_subcommand_from register list current use pin unregister doctor' -l format -a 'text json'
complete -c ecos -f -n '__fish_seen_subcommand_from register' -l name -r
complete -c ecos -f -n '__fish_seen_subcommand_from register' -l kind -a 'checkout release'
complete -c ecos -f -n '__fish_seen_subcommand_from register' -l activate
complete -c ecos -f -n '__fish_seen_subcommand_from register' -l replace
complete -c ecos -f -n '__fish_seen_subcommand_from current pin' -l project -r
complete -c ecos -f -n '__fish_seen_subcommand_from current' -l sdk -r
complete -c ecos -f -n '__fish_seen_subcommand_from create' -l name -r
complete -c ecos -f -n '__fish_seen_subcommand_from create' -l path -r
complete -c ecos -f -n '__fish_seen_subcommand_from create' -l board -r
complete -c ecos -f -n '__fish_seen_subcommand_from create' -l target -r
complete -c ecos -f -n '__fish_seen_subcommand_from create' -l profile -r
complete -c ecos -f -n '__fish_seen_subcommand_from create' -l dry-run
complete -c ecos -f -n '__fish_seen_subcommand_from create' -l force
complete -c ecos -f -n '__fish_seen_subcommand_from create' -l format -a 'text json'
complete -c ecos -f -n '__fish_seen_subcommand_from set-board set-target' -l project -r
complete -c ecos -f -n '__fish_seen_subcommand_from set-board set-target' -l format -a 'text json'
complete -c ecos -f -n '__fish_seen_subcommand_from build' -l project -r
complete -c ecos -f -n '__fish_seen_subcommand_from build' -l clean
complete -c ecos -f -n '__fish_seen_subcommand_from build' -l format -a 'text json'
complete -c ecos -f -n '__fish_seen_subcommand_from validate configure flash monitor' -l project -r
complete -c ecos -f -n '__fish_seen_subcommand_from validate configure flash monitor' -l format -a 'text json'
complete -c ecos -f -n '__fish_seen_subcommand_from menuconfig' -l project -r
complete -c ecos -f -n '__fish_seen_subcommand_from configure' -l dry-run
complete -c ecos -f -n '__fish_seen_subcommand_from flash' -l device -r
complete -c ecos -f -n '__fish_seen_subcommand_from monitor' -l port -r
complete -c ecos -f -n '__fish_seen_subcommand_from monitor' -l baudrate -r
complete -c ecos -f -n '__fish_seen_subcommand_from monitor' -l timeout -r
complete -c ecos -f -n '__fish_seen_subcommand_from monitor' -l expect -r
complete -c ecos -f -n '__fish_seen_subcommand_from toolchain; and __fish_seen_subcommand_from detect status install' -l format -a 'text json'
complete -c ecos -f -n '__fish_seen_subcommand_from status install' -l prefix -r
complete -c ecos -f -n '__fish_seen_subcommand_from status' -l custom -r
complete -c ecos -f -n '__fish_seen_subcommand_from install' -l cache-dir -r
complete -c ecos -f -n '__fish_seen_subcommand_from install' -l archive -r
complete -c ecos -f -n '__fish_seen_subcommand_from install' -l force
complete -c ecos -f -n '__fish_seen_subcommand_from install' -l dry-run
'''


def _powershell() -> str:
    roots = ", ".join(f"'{value}'" for value in ROOT_COMMANDS)
    toolchains = ", ".join(f"'{value}'" for value in TOOLCHAIN_COMMANDS)
    sdks = ", ".join(f"'{value}'" for value in SDK_COMMANDS)
    projects = ", ".join(f"'{value}'" for value in PROJECT_COMMANDS)
    return f'''# ECOS completion for PowerShell
Register-ArgumentCompleter -Native -CommandName ecos -ScriptBlock {{
    param($wordToComplete, $commandAst, $cursorPosition)

    $words = @($commandAst.CommandElements | ForEach-Object {{ $_.Extent.Text }})
    $command = if ($words.Count -gt 1) {{ $words[1] }} else {{ '' }}
    $subcommand = if ($words.Count -gt 2) {{ $words[2] }} else {{ '' }}
    $candidates = @()

    if ($words.Count -le 2) {{
        $candidates = @({roots})
    }} elseif ($command -eq 'toolchain' -and $words.Count -le 3) {{
        $candidates = @({toolchains})
    }} elseif ($command -eq 'sdk' -and $words.Count -le 3) {{
        $candidates = @({sdks})
    }} elseif ($command -eq 'project' -and $words.Count -le 3) {{
        $candidates = @({projects})
    }} elseif ($command -eq 'completion' -and $words.Count -le 3) {{
        $candidates = @('bash', 'zsh', 'fish', 'powershell')
    }} elseif ($command -eq 'build' -and $wordToComplete.StartsWith('-')) {{
        $candidates = @('--project', '--clean', '--format', '--help')
    }} elseif ($command -eq 'validate' -and $wordToComplete.StartsWith('-')) {{
        $candidates = @('--project', '--format', '--help')
    }} elseif ($command -eq 'configure' -and $wordToComplete.StartsWith('-')) {{
        $candidates = @('--project', '--dry-run', '--format', '--help')
    }} elseif ($command -eq 'menuconfig' -and $wordToComplete.StartsWith('-')) {{
        $candidates = @('--project', '--help')
    }} elseif ($command -eq 'flash' -and $wordToComplete.StartsWith('-')) {{
        $candidates = @('--project', '--device', '--format', '--help')
    }} elseif ($command -eq 'monitor' -and $wordToComplete.StartsWith('-')) {{
        $candidates = @('--project', '--port', '--baudrate', '--timeout', '--expect', '--format', '--help')
    }} elseif ($command -eq 'toolchain' -and $wordToComplete.StartsWith('-')) {{
        $candidates = switch ($subcommand) {{
            'detect' {{ '--format', '--help' }}
            'status' {{ '--format', '--prefix', '--custom', '--help' }}
            'install' {{ '--format', '--prefix', '--cache-dir', '--archive', '--force', '--dry-run', '--help' }}
        }}
    }} elseif ($command -eq 'sdk' -and $wordToComplete.StartsWith('-')) {{
        $candidates = switch ($subcommand) {{
            'register' {{ '--name', '--kind', '--activate', '--replace', '--format', '--help' }}
            'current' {{ '--project', '--sdk', '--format', '--help' }}
            'pin' {{ '--project', '--format', '--help' }}
            default {{ '--format', '--help' }}
        }}
    }} elseif ($command -eq 'project' -and $wordToComplete.StartsWith('-')) {{
        $candidates = switch ($subcommand) {{
            'create' {{ '--name', '--path', '--board', '--target', '--profile', '--dry-run', '--force', '--format', '--help' }}
            'set-board' {{ '--project', '--format', '--help' }}
            'set-target' {{ '--project', '--format', '--help' }}
        }}
    }}

    $candidates |
        Where-Object {{ $_ -like "$wordToComplete*" }} |
        ForEach-Object {{
            [System.Management.Automation.CompletionResult]::new($_, $_, 'ParameterValue', $_)
        }}
}}
'''


GENERATORS: dict[str, Callable[[], str]] = {
    "bash": _bash,
    "zsh": _zsh,
    "fish": _fish,
    "powershell": _powershell,
}


def generate(shell: str) -> str:
    try:
        content = GENERATORS[shell]()
    except KeyError as exc:
        supported = ", ".join(SUPPORTED_SHELLS)
        raise ValueError(f"unsupported completion shell {shell}; choose: {supported}") from exc
    return content.rstrip() + "\n"
