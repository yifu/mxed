#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LINE_WIDTH 16

void hex_dump(FILE *file) {
    unsigned char buffer[16];
    size_t bytes_read;
    unsigned long offset = 0;

    // Effacer l'écran avant de commencer à afficher
    clear();

    scrollok(stdscr, TRUE);

    while ((bytes_read = fread(buffer, 1, LINE_WIDTH, file)) > 0) {
        // Affichage des données hexadécimales
        printw("%08lx: ", offset);
        for (size_t i = 0; i < bytes_read; i++) {
            printw("%02x ", buffer[i]);
        }
        for (size_t i = bytes_read; i < LINE_WIDTH; i++) {
            printw("   ");
        }

        // Affichage des caractères ASCII correspondants
        printw("| ");
        for (size_t i = 0; i < bytes_read; i++) {
            printw("%c", (buffer[i] >= 32 && buffer[i] <= 126) ? buffer[i] : '.');
        }
        printw("\n");

        offset += bytes_read;

        refresh();
    }

    // Rafraîchir l'écran pour appliquer l'affichage
    refresh();
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <file>\n", argv[0]);
        return 1;
    }

    // Initialisation de ncurses
    initscr();               // Initialisation de ncurses
    raw();                   // Désactive le buffering
    cbreak();                // Lecture caractère par caractère
    noecho();                // Ne pas afficher les caractères tapés
    nonl();                  // Désactive le retour à la ligne automatique
    keypad(stdscr, TRUE);    // Active la gestion des touches spéciales (flèches, etc.)
    intrflush(stdscr, FALSE); // Désactive le nettoyage de l'écran sur interruption
    curs_set(0);             // Masque le curseur pour une meilleure apparence

    // Ouverture du fichier
    FILE *file = fopen(argv[1], "rb");
    if (!file) {
        perror("fopen");
        endwin();
        return 1;
    }

    // Affichage du fichier en hexadécimal
    hex_dump(file);

    // Attendre une touche pour quitter
    getch();

    // Nettoyage
    fclose(file);
    endwin();
    return 0;
}

