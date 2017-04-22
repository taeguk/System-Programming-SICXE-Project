SOURCES=$(shell find . -type f -iname '*.c')
TARGET=20141500.out

all: $(TARGET)

$(TARGET): $(SOURCES)
	gcc -std=gnu99 -W -Wall $(SOURCES) -o $(TARGET)

clean:
	rm -f $(TARGET)
