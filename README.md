# Online Integer Calculator (UDP)
The app has two executables: a client and a server.

The server executes the math operations while the client interacts with the user.

The server has one parameter: the port number, if it is not given the server assume the default port number. <br>
Given a wrong port number to the server alerts the user and close the application with an error code. <br>
`Server.exe 55000` (assign 55000 as the port number for the server)

The client has two parameters: the address and the port number of the server. <br>
If the parameters are missing the client uses the default parameters. <br>
If the address is not a valid address the client uses the default ip address, if the port number is not a valid port number the client uses the default port number. <br>
`Client.exe 127.0.0.1:55000` (connect with the server on the 55000 port at 127.0.0.1) or <br>
`Client.exe localhost:55000` (connect with the server on the 55000 port at localhost) or <br>
`Client.exe srv.di.uniba.it:55000` (connect with the server on the port 55000 at srv.di.uniba.it)


The calculator accepts only integer number and return a result (or an error), <br> the command for the operations is: `[operation char] [integer] [integer]`. <br>
The possible operation are: ‘+’ (addition), ‘–‘ (subtraction), ‘x’ (multiplication) and ‘/’ (division). <br>
eg. `+ 1 1` = `1 + 1`.

If the client cannot send/receive from the server for 3 or more times the client shutdown the connection.
