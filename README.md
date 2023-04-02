This project has can be built using run.sh.
It depends on X11 and pango
pango can be downloaded using apt install libpango-1.0-dev

compile_flags.txt has a newline-seperated list of arguments that are passed
to clang during compilation and has been generated manually by running pkg-config.
compile_flags.txt is used by clangd (the language server for this project) to
provide intellisense to your editor of choice.
As you can see in run.sh, the output of pkg-config is being used
to pass the needed include paths and linker paths to clang.

The assets folder contains images, fonts, and other blobs that are used by the
program.
