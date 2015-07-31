# shell

In this problem a command line interpreter or shell has been implemented. The shell
takes in a command from user at its prompt and executes it and then prompts for more
input from user.
-shell executes a command using fork()and execve() calls. It does not use
temporary files, popen(), or system() calls, sh or bash shells to
execute a command. Once the command is completed it prints its pid and
status.
- shell supports <, >, and >> redirection operators.
- shell supports pipelining any number of commands. E.g.: ls|wc|wc, cat
x| grep pat| uniq| sort
- shell supports two new pipeline operators "||" and "|||". E.g.: ls -l ||
grep ^-, grep ^d. It means that output of ls -l command is passed as input to two other commands. Similarly "|||" means, output of one command is passed as input to three other commands separated by ",".
