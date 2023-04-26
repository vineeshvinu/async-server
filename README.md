# RPC-server
A server-side part of a client-server application that will execute commands from
a client. You may use any transport protocol (UDP/TCP) to transfer data but explain your choice.
# Note
*TCP is the transport protocol to transfer data. TCP provides reliable and ordered data transmission, 
ensuring that data is not lost or duplicated during transmission. Additionally, TCP has a built-in flow control 
mechanism that ensures that the server does not become overwhelmed with incoming connections.*

The program should receive commands from a client, execute them and return a result to the
client. A client might send several commands sequentially (after receiving a result to the
previous command).

The solution should serve at least 10 clients at a time and should be based on libevent library.
Incoming command format:
<COMMAND> <ARGUMENT 1> <ARGUMENT 2> … <ARGUMENT N>\n
The command and all arguments are separated by spaces. For simplicity, arguments can’t
contain spaces.

Answer format:
<STATUS> <ANSWER>\n
STATUS - a symbol indicating a result of the command:
“S” - successful
“E” - unsuccessful
A status field and an answer field are separated by a space. The answer can contain any
number of spaces.

supported commands:
Ping command:
ping -> S pong
File content output by name:
cat <FILENAME> -> S <DATA> or E <comment> if there is no such file
The sum of all parameters:
sum 1 2 3 … 4 5 (unlimited number of arguments) -> S <SUM>
  
Run:
```bash
gcc rpcserver.c -o server -levent
./server
```
  
Execute command:
```bash
telnet localhost 8080
```
