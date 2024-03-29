C_FILES = $(wildcard **/*.c)
OBJECTS = $(patsubst %.c, %.o, $(C_FILES))

CC = gcc
CFLAGS = -Wall -pedantic
LDFLAGS =
LDLIBS = -lm

client1: .depend client1.o $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) client1.o -o $@ $(LDFLAGS) $(LDLIBS)

depend: .depend

.depend: cmd = gcc -MM -MF depend $(var); cat depend >> .depend;
.depend:
	@echo "Generating dependencies..."
	@$(foreach var, $(C_FILES), $(cmd))
	@rm -f depend

-include .depend

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%: %.o
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(OBJECTS) .depend client1.o

PHONY: clean depend