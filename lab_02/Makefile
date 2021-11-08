CC=gcc
FLAGS=-Wall -Werror -O2 -Iinc
CFILE_C=src/client.c
CFILE_S=src/server.c

all: server client

server: $(addprefix out/,$(addsuffix .o,$(notdir $(basename $(CFILE_S)))))
	$(CC) $(FLAGS) $^ -o server

client: $(addprefix out/,$(addsuffix .o,$(notdir $(basename $(CFILE_C)))))
	$(CC) $(FLAGS) $^ -o client

run_s: server
	./server

run_c: client
	./client

out/%.o: */%.c
	$(CC) $(FLAGS) -c -MD $< -o out/$(addsuffix .o,$(basename $(notdir $<)))

clean:
	rm *.out *.exe out/*.* out/. server client


include $(wildcard out/*.d)
