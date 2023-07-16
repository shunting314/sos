# Plan
Write a shell in the host os first and then try to migrate it to sos.

# Features
- IO Redirection
- pipe
- builtin commands (cd, exit, etc.)

## optional features
- background execution
- quoted string (with spaces)
- command history and editing
- wildcard characters
- environment variables
- control flow

# Reference
- [a easy to follow blog about creating a shell in C](https://brennan.io/2015/01/16/write-a-shell-in-c/)
  - this is too simply that it does not cover io redirection or pipe.
- [man page of pipe](https://man7.org/linux/man-pages/man2/pipe.2.html)
- [man page of dup](https://man7.org/linux/man-pages/man2/dup.2.html)
- [a geeksforgeeks article](https://www.geeksforgeeks.org/making-linux-shell-c/)
  - covers pipe but does not cover IO redirection. I think IO redirection can be handled by using dup2 syscall (which is used for supporting pipe as well)
  - I hate the auto-play video ads on geeksforgeeks..
- [GRR's Online Book](https://www.cs.purdue.edu/homes/grr/SystemsProgrammingBook/Book/Chapter5-WritingYourOwnShell.pdf): The best I've read so far.
  - explain impleneting a shell in a very principled way.
  - support both pipe and io-rediction
  - support chaining any number of commands with pipe
  - QUOTE: "No shell is complete without wildcards". lol
