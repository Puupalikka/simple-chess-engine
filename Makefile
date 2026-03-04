CC = gcc
CFLAGS = -Wall -fPIC
LDFLAGS = -shared
TARGET = engine.so

VENV_DIR = venv
PYTHON = $(VENV_DIR)/bin/python
PIP = $(VENV_DIR)/bin/pip

.PHONY: all run install clean

all: $(TARGET)

$(VENV_DIR)/bin/activate:
	python3 -m venv $(VENV_DIR)
	$(PIP) install --upgrade pip

$(TARGET): engine.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(TARGET) engine.c

install: $(VENV_DIR)/bin/activate
	$(PIP) install -r requirements.txt

run: $(TARGET) install
	$(PYTHON) gui.py

run-parser: $(TARGET) install
	$(PYTHON) fen-pgn-parser.py

clean:
	rm -f $(TARGET)
	rm -rf $(VENV_DIR)
