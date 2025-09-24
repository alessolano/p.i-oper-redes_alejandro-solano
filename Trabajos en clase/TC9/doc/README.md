# How to run with the makefile

```
make
```

Then run the server:

```
make run-server
```

Then run the client:

.cpp:

```
make run-client-cpp
```

.c:
```
make run-client-c
```

In case you don´t have the certificate for SSL, run the command:

```
openssl req -x509 -newkey rsa:2048 -nodes \
  -keyout key0123.pem -out ci0123.pem \
  -days 365 -subj "/CN=localhost"

```