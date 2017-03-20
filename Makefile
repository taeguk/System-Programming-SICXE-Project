SOURCES=command.c history.c list.c main.c memory.c opcode.c
TARGET=20141500.out

all: $(TARGET)

$(TARGET): $(SOURCES)
	gcc -std=gnu99 $(SOURCES) -o $(TARGET)

clean:
	rm -f $(TARGET)
