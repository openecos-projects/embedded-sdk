# FCC on StarrySky C2

This development example mounts an existing FatFs volume and starts a
Letter-Shell prompt. It is available on `starrysky-c2` with either the
`flash-xip` or `psram-data` profile. The Flash volume must already contain
the source file; the example does not format an empty volume.

At the prompt, use `ls`, `cd` and `fcc <source.c>` to compile a file on drive
`0:`. Relative paths follow the current Shell directory. Run the most recently
compiled program with `fcc-run`. Any failed compilation disables `fcc-run`
until another source compiles successfully.

For a first hardware check, put this file on the volume as `answer.c`:

```c
int counter=1;
int main(){
    counter=counter+1;
    if(counter==2){return twice(21);}
    return 0;
}
int twice(int x){return x*2;}
```

`fcc answer.c` should succeed and `fcc-run` should report return value 42.
For a loop check, compile this as `loop.c`; `fcc-run` should report 3:

```c
int main(){
    int x;
    x=0;
    while(x<3){x=x+1;}
    return x;
}
```

The compiler supports a small integer-only C subset: global integer constants,
functions with up to eight integer arguments, local `int` declarations at the
start of a function, assignment, `if`/`else`, `while` and `return`. It does not
implement standard C headers, pointers, strings or `for`. Source and generated
code are each limited to 32 KiB; the global variable area is 1 KiB.

Generated instructions run from a linker-reserved executable SRAM section.
Compilation and both profile layouts have been checked by host and cross-build
tests. The example ships with SDK 3.0; actual execution, Flash I/O and stack
headroom still require C2 hardware validation.
