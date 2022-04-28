# Chat Application
A server for chat application based on the client-server model.

Supported:
```bash
(1) Bootstrapping: The server should take a command line argument for the port on which to listen.
(2) Concurrency: The server is able to handle multiple clients at the same time.
(3) The server maintains a record of all the Clients' IDs currently participating. It is able to handle four types of messages from the clients:
    * HELLO message (Clients tell the server that they are here. The server will register the client by keeping track of its ClientID and send back a HELLO_ACK message.
                     Additionally it will also send a CLIENT_LIST message which contains the information about existing clients. This list is in the exact order in which clients arrived at the server and also contains the current clientID at the end.
                     If a ClientID already exists, the server responds with an ERROR(CLIENT_ALREADY_PRESENT) message and closes the TCP connection). 
    * LIST_REQUEST message (A client can also explicitly request the list of connected clients by sending a LIST_REQUEST message.
                            The list is in the exact order in which clients arrived at the server).
    * CHAT message (The server will forward this message to the intended recipient client in another CHAT message.
                    If the recipient client doesn’t exist, the server discards the message and sends an ERROR(CANNOT_DELIVER) message, which has the Message-ID appropriately set, back to the sender).
    * EXIT message (Signals that the client is leaving. In this case the server removes the client from its record).
(4) The server can keep partial messages for solving possible delay and later process them when the rest of the messages arrives from client. 
    For partial messages, if there is no follow-up message for 1 minute, the server will discard that partial message, and the connection with that particular client will be closed.
(5) Error handling (e.g. Client arbitrarily closing the socket. A new client sends a CHAT message instead of HELLO. A CHAT message without a destination or client trying to chat with herself.)
```
## Usage
```bash
make
./a.out [port]
```
