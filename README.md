# hex-editor
View and edit binary files via a TUI

# Current Supported Commands
## Navigation Mode
| Command | Description |
| ----------- | ----------- |
| j | move cursor down (ie. jump ahead 16 bytes) |
| k | move cursor up (ie. jump back 16 byte) |
| l | move cursor right (ie. jump ahead 1 byte) |
| h | move cursor left (ie. jump back 1 byte) |
| v | toggle selection mode |
| r | edit byte(s) in selection |
| TBD | yank selection |
| TBD | yank selection to clipboard |
| TBD | paste |
| ESC | exit mode |

## Command Mode
To enter the command prompt type `:`
| Command | Description |
| ----------- | ----------- |
| :w | write file |
| :q | quit hex editor |
