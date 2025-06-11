#!/bin/sh

clang++ -std=c++2b -Wall -Wextra -o hexedit ncurses.cpp -lncurses
clang++ -std=c++2b -Wall -Wextra -o tests main.cpp
