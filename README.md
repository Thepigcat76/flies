# Flies

A simple cli file explorer for linux

## How to build

To build flies simply run the build.sh bash script.
Alternatively you can also manually compile and run the build.c file.

For example:
```bash
gcc build.c -o flies-build && ./flies-build
```

## How to use

To use flies simply run the built file

If you built the project using the script:
```bash
./flies
```

If you built the project manually
```bash
./build/flies
```

You can also open flies in a specific directory by passing it as an argument:

```bash
./flies /home/thepigcat/Downloads
```

### Keybinds

**Up** and **Down** Arrow Keys for moving the cursor up and down in the current directory.

**Left** Arrow Key to go back one directory (`cd ..`)

**Right** Arrow Key to undo going back one directory

**Enter** Key to open the hovered directory or file or to execute a command

### Commands

Commands are an essential feature of Flies that allow you to perform various tasks on files.

- `:rn <new-name>` Renames the hovered file or directory to `<new-name>`

- `:nf <name>` Creates a new file `<name>`

- `:nd <name>` Created a new directory `<name>`

- `:c` Copies the currently hovered file/directory 

- `:p` Pastes the current clipboard contents if they are a file/content

- `:d` Deletes the current file/directory

- `:r` Reloads the config

- `:q` Exits the app

### Command Keybinds

Some commands also have keybinds that can be used instead

- `Ctrl + C` = `:c`

- `Ctrl + V` = `:p`

- `Ctrl + D` = `:d`

- `Ctrl + Q` = `:q`

- `Ctrl + Z` Undo the last action

- `Ctrl + X` Cuts the current directory/file

### Configuration

Some of the features in Flies can also be configured.
The config file is located at `/home/<user>/.flies/flies.cfg`.
It gets generated when the program is first ran.

- `text-editor` The default text editor used to open a file (default: `nano`)

- `max-rows` The maximum amount of rows displayed at a time (default: `9`)

- `show-hidden-dirs` Whether to show hidden directories (default: `true`)
