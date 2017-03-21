SOURCES=command.c history.c list.c 20141500.c memory.c opcode.c
TARGET=20141500.out

all: $(TARGET)

$(TARGET): $(SOURCES)
	gcc -std=gnu99 -W -Wall $(SOURCES) -o $(TARGET)

clean:
	rm -f $(TARGET)
