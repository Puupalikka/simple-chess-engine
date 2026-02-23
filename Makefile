CC = gcc
CFLAGS = -Wall -fPIC
LDFLAGS = -shared
TARGET = engine.so

.PHONY: all run install clean

all: $(TARGET)

$(TARGET): engine.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(TARGET) engine.c

run: $(TARGET)
	python3 ui.py

install:
	pip install -r requirements.txt

clean:
	rm -f $(TARGET)
	find . -type d -name "__pycache__" -exec rm -r {} +
