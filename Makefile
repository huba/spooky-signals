C_FILES = $(wildcard *.c)
OBJECTS = $(patsubst %.c, %.o, $(C_FILES))

CC = gcc
CFLAGS = -Wall -pedantic
LDFLAGS =
LDLIBS = -lm

client1: .depend $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ client1.o $(LDFLAGS) $(LDLIBS)

depend: .depend

.depend: cmd = gcc -MM -MF depend $(var); cat depend >> .depend;
.depend:
	@echo "Generating dependencies..."
	@$(foreach var, $(C_FILES), $(cmd))
	@rm -f depend

%.o: %.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f $(OBJECTS) .depend

PHONY: clean depend