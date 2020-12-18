# shell-core (sc)

An extendible Linux / Unix shell with pipe and redirection support.

This program implements the **core** functionalities of a shell: \
read, parse, fork, setup redirects/pipes, fork, exec, wait (if not ran in background), and repeat.

## Features

#### Detects super cow powers:
```
[64bit-awesome@arch-linux ~] $ sudo sc
[sudo] password for 64bit-awesome:
[shell-core ~] # 
```

#### Keeps track of current directory:
```
[64bit-awesome@arch-linux ~] $ sc
[shell-core ~] $ mkdir awesome
[shell-core ~] $ cd awesome
[shell-core awesome] $ 
```

#### Redirection of STDIN and/or STDOUT
```
[shell-core ~] $ cat < input.txt > input.txt.copy
```

#### STDOUT redirection append mode:
```
[shell-core ~] $ cat < more-input.txt >> input.txt.copy
```

#### Pipelining: 
```
[shell-core ~] $ ls -la | grep Makefile
```
```
[shell-core ~] $ ls -la | grep c | ... | sort
```

#### Run commands in the background (&):
```
[shell-core ~] $ ls -ls &
```

#### Multiple commands in one line (&&):
```
[shell-core ~] $ ls && echo password123 | grep 123
```

#### Run command in background AND pipe result (second process must wait for input of other):
```
[shell-core ~] $ ls -la | grep Makefile
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
- Maximum number of tokens (as defined by **MAX_TOKENS**) is currently 25.
- When processing multiple commands in one line: \
    - the shell will always wait for the first process to finish before spawning the next; \
    - unless the statement is ran in the background with the & symbol.
