# b4m36esw-cpp-libmicrohttpd-server

Write an HTTP 1.1 protocol based server application, which will count unique words in data sent by clients. The server must meet the following requirements:

Clients send data using POST method with path /esw/myserver/data. Data are in plain text format encoded in UTF-8 and compressed by gzip method. The server counts unique words. On startup, the counter equals zero. The server keeps records of words sent by clients. The unique word counter is increased by one for each new word, which is not in records yet. If a GET request with path /esw/myserver/count arrives, the server will answer actual value of unique count and reset it. The value is transferred as a decadic number in UTF-8 encoding. The server must be able to handle a large number of simultaneously connected clients (approximately 100).
