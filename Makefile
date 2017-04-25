SOURCES=$(shell find . -type f -iname '*.c')
TARGET=20141500.out

all: $(TARGET)

$(TARGET): $(SOURCES)
	gcc -std=gnu99 $(SOURCES) -o $(TARGET) -W -Wall -Wno-unused-parameter

clean:
	rm -f $(TARGET)
