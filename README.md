# Socket Programming with C

This is a sequential client-server connection using sockets with c.

The clients send the server requests for the creation, destruction,
modification or reception of queues with data stored in them. All the queues
are stored in a hash table or dicctionary which is managed exclusively in the
server side.


Notas: se podria crear en la clase libzerocopyMQ una funcion que haga un setup
de los elementos opcion, cola size y cola name. Similar a create_destroy pero
que sirva para todos. Cuidar el handling del pointer a la struct iovec.
