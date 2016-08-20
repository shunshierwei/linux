BIN=http
SRC=http.c
OBJ=$(SRC:.c=.o)
CC=gcc
LDFLAGS=-lpthread
$(BIN):$(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)
%.o:%.c
	$(CC) -c $<
.PHONY:clean
clean:
	rm -f $(OBJ) $(BIN)
.PHONY:debug
debug:
	@echo $(SRC)
	@echo $(OBJ)
