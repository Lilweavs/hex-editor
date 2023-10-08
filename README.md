# hex-editor
View and edit binary files via a TUI. Built with C++ and [FTXUI](https://github.com/ArthurSonzogni/FTXUI).

![hex_editor_main_view](https://github.com/Lilweavs/hex-editor/assets/35853304/8c8eac67-8155-40fc-bd40-8311c3507898)

# Current Supported Commands
## Keybindings
List of currently supported keybindings:
  -  `j` move cursor down (ie. jump ahead 16 bytes)
  -  `k` move cursor up (ie. jump back 16 byte)
  -  `l` move cursor right (ie. jump ahead 1 byte)
  -  `h` move cursor left (ie. jump back 1 byte)
  -  `v` toggle selection mode
  -  `r` edit byte(s) in selection
  -  `y` **WIP**
  -  `Y` yank selection to system clipboard (Windows only as of now)
  -  `p` **WIP**
  -  `P` **WIP**
  -  `ESC` exit mode
  -  `f` open pattern view
  -  `n` create new pattern (in pattern view) 
  -  `N` delete last pattern (in pattern view)
  -  `:` open command prompt
  -  `:w` write file
  -  `:q` quit hex editor
    
## Give it a shot
`git clone https://github.com/Lilweavs/hex-editor.git && cd ./hex-editor && cmake . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build ./build`
