# simple-chess-engine
## simple and modifiable chess engine and gui for game analysis 

CURRENTLY THE ENGINE IS NOT READY FOR USE.

This engine is meant to be a simple and comprehensible chess engine. The idea is that by reading the source code of the engine you can understand how moves of the evaluated game or position get their values.

PyInstaller was used in the project, but only for generating the executable gui file from gui.py. For that reason, the license of PyInstaller (GPLv2 with some exeptions) doesn't restrict the licence of this project (which uses MIT licence).

## building
Clone the repository:
git clone https://github.com/Puupalikka/simple-chess-engine.git
cd simple-chess-engine

Install dependencies and build executables:
make clean all install

Run the engine:
make run

Run the fen-pgn-parser:
make run-parser

Here the parser will ask the name of the file where fen is located. Files are named so that the first game is named fen1.txt, second fen2.txt and so on. Give here the game you want to be parsed to pgn.
