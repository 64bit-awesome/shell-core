# linux-shell-core (lsc)

An extendible Linux shell with pipe and redirection support.

This program implements the **core** functionalities of a shell: \
read, parse, fork, setup redirects/pipes, fork, exec, wait (if not ran in background), and repeat.

It should also work on Unix.


## Features

#### Detects super cow powers:
```
[64bit-awesome@arch-linux ~] $ sudo lsc
[sudo] password for 64bit-awesome:
[linux-shell-core ~] #
```

#### Keeps track of current directory:
```
[64bit-awesome@arch-linux ~] $ usc
[linux-shell-core ~] $ mkdir awesome
[linux-shell-core ~] $ cd awesome
[linux-shell-core awesome] $
```

#### Redirection of STDIN and/or STDOUT
```
[linux-shell-core ~] $ cat < input.txt > input.txt.copy
```

#### STDOUT redirection append mode:
```
[linux-shell-core ~] $ cat < more-input.txt >> input.txt.copy
```

#### Pipelining: 
```
[linux-shell-core ~] $ ls -la | grep usc.c
```

## Compiling

### Build
```
[64bit-awesome@arch-linux ~] $ make
```

### Cleanup
```
[64bit-awesome@arch-linux ~] $ make clean
```

## Limitations
- Arguments, redirect symbols, and pipes must be seperated by whitespace.
- Only supports background commands (**&**) when not piping the output.
- Maximum number of arguments (as defined by **MAX_ARGS**) is currently 45.
- When processing a pipe: the shell will always wait for the first process to finish before spawning the next.
