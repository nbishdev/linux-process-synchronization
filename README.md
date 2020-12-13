# Linux Process Synchronization and Communication

## Description
We use the [Client-Server Model](https://en.wikipedia.org/wiki/Client%E2%80%93server_model) to test process synchronization and communication in a [POSIX](https://en.wikipedia.org/wiki/POSIX) Operating System.

We create a Server process (S) which has access to some directory with files and a Client process (C) which has access to some log file.

After some time interval, C clones itself, creating a subprocess C' which requests the contents of a file from S. This time interval follows an exponential distribution with some pre-defined constant value for lambda.

After S receives a request, S clones itself, creating a subprocess S' which processes the request, opens the requested file, reads its content and sends this content as a reply to the specific C' which requested the specific file.

After S' replies to C', it terminates.

When C' receives the reply, it updates the log file with the details of the exchange and terminates.

After a pre-defined number of transactions, both C and S terminate.

The messages are exchanged using a Shared Memory Segment and the process synchronization is achieved using Semaphores.

## Execution
In order to execute the software, use a Terminal Emulator and navigate to the src directory.

Then, to compile and build the software type the following command:
```bash
make
```

After you have successfully built the software, execute it using the following command:
```bash
./cs [directory] [number of files] [number of transactions] [lambda]
```

For example, to execute the software using the directory with the dummy sample text files provided in this repository with 10 text files, 10 transactions between the client and the server and with 0.1 as the value of lambda, use the following command:
```bash
./cs ../files/ 10 10 0.1
```
