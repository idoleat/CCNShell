# CCNShell
A shell from CCNS to experiment weird ideas like hell.

## Usage
After cloning and changing directory into the repository,
```
$ make
$ ./ccnshell
```

## Notes
Line editing and history is provided by [linenoise](https://github.com/antirez/linenoise). The project license follows it as well.  

CCNShell currently can:
* Execute binaries from `/usr/bin/`
* Take arguments
* Execute shell script by entring `../../home/USER_NAME/path/to/your/script`
  * Script should have `#!` annotating which shell binary to use at the first line

CCNShell currently can not:
* Pass any environment variable
* Trim trailing spaces
  
Need futher code refactoring to separate shell logic from linenoise.

## Roadmap
* Add very-easy-to-use shell script
* Infrastructures to test concurrent programs or IPC
* Any coool stuff!
* RIIR. Rewrite at the first day? Why not? It's cool.
