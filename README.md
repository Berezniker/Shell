# Shell 

**Backus–Naur form (BNF)**

> <команда_shell> ::= <список_команд>  
> <список_команд> ::= <конвейер> { \[один из &, &&, ||, ;\] <конвейер> } \[&, ;\]  
> <конвейер> ::= <команда> { | <команда> }  
> <команда> ::= <простая_команда> | (<список_команд>)\[<имя_файла>\] \[ \[один из >, >>\] <имя_файла>\]  
> <простая_команда> ::= <имя_файла> { <аргумент> } \[ < <имя_файла>\] \[ \[один из >, >>\] <имя_файла>\]  

<img src="https://github.com/Berezniker/Shell/blob/master/console.png">

### Load:
```sh
$ git clone https://github.com/Berezniker/CLI_Shell.git
$ cd CLI_Shell
```

### Compile:
```sh
$ make shell
```

### Run:
```sh
$ make run
```
