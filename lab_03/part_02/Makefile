all: client server

client: src/client.cxx
	g++ -pthread src/client.cxx -o client

server: server.o thread_pool.o
	g++ -pthread -Iinc -o server out/server.o out/thread_pool.o

server.o: src/server.cxx inc/thread_pool.hpp
	g++ -c src/server.cxx -Iinc -o out/server.o

thread_pool.o: src/thread_pool.cxx inc/thread_pool.hpp
	g++ -c src/thread_pool.cxx -Iinc -o out/thread_pool.o

clean:
	rm -rf out/ && rm *.exe *.out server client
