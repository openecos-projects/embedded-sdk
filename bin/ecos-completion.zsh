#!/usr/bin/env zsh

# ECOS command completion script for Zsh and Bash

_ecos_completions() {
    local cur prev cmds boards templates

    # Cross-compatibility for zsh and bash
    if [[ -n "${ZSH_VERSION-}" ]]; then
        # Zsh handling
        cur="${words[CURRENT]}"
        prev="${words[CURRENT-1]}"
        # Ensure words array starts at 1
        setopt localoptions ksharrays
        local cmd_pos=2
    else
        # Bash handling
        cur="${COMP_WORDS[COMP_CWORD]}"
        prev="${COMP_WORDS[COMP_CWORD-1]}"
        local cmd_pos=1
    fi

    # Available commands and options
    cmds="init_project set_board flash monitor help"
    boards="c1 c2 l3"
    templates="asm_hello asm_gpio gpio hello i2c_scan i2c_sgp30 minesweeper pwm spi_st7735 spi_st7735_clock spi_st7735_donut spi_st7789 list"

    # Complete main commands
    if [[ ( -n "${ZSH_VERSION-}" && ${CURRENT} -eq 2 ) || ( -n "${BASH_VERSION-}" && ${COMP_CWORD} -eq 1 ) ]]; then
        if [[ -n "${ZSH_VERSION-}" ]]; then
            # compadd without splitting to avoid issues
            compadd -a "${(s: :)cmds}"
        else
            COMPREPLY=( $(compgen -W "${cmds}" -- "${cur}") )
        fi
        return 0
    fi

    # Subcommand completion
    local subcmd=""
    if [[ -n "${ZSH_VERSION-}" ]]; then
        subcmd="${words[2]}"
    else
        subcmd="${COMP_WORDS[1]}"
    fi

    case "${subcmd}" in
        set_board)
            if [[ ( -n "${ZSH_VERSION-}" && ${CURRENT} -eq 3 ) || ( -n "${BASH_VERSION-}" && ${COMP_CWORD} -eq 2 ) ]]; then
                if [[ -n "${ZSH_VERSION-}" ]]; then
                    compadd -a "${(s: :)boards}"
                else
                    COMPREPLY=( $(compgen -W "${boards}" -- "${cur}") )
                fi
            fi
            return 0
            ;;
        init_project)
            if [[ ( -n "${ZSH_VERSION-}" && ${CURRENT} -eq 3 ) || ( -n "${BASH_VERSION-}" && ${COMP_CWORD} -eq 2 ) ]]; then
                if [[ -n "${ZSH_VERSION-}" ]]; then
                    compadd -a "${(s: :)templates}"
                else
                    COMPREPLY=( $(compgen -W "${templates}" -- "${cur}") )
                fi
            fi
            return 0
            ;;
        monitor)
            if [[ ( -n "${ZSH_VERSION-}" && ${CURRENT} -eq 3 ) || ( -n "${BASH_VERSION-}" && ${COMP_CWORD} -eq 2 ) ]]; then
                if [[ -n "${ZSH_VERSION-}" ]]; then
                    _files -g "/dev/tty*"
                else
                    compopt -o default
                    COMPREPLY=( $(compgen -G "/dev/tty*" -- "${cur}") )
                fi
            fi
            return 0
            ;;
        *)
            ;;
    esac
}

# Register the completion function
if [[ -n "${ZSH_VERSION-}" ]]; then
    compdef _ecos_completions ecos
elif [[ -n "${BASH_VERSION-}" ]]; then
    complete -F _ecos_completions ecos
fi
