#include "MMapFile.h"
#include "HexEditorOverlay.h"

#include <ncurses.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <format>
#include <ranges>

constexpr int MAX_LINE_LENGTH = 50;  // Longueur maximale des lignes

// Fonction pour afficher une ligne de texte
void print_line(WINDOW* win, int y, const std::string& line) {
    mvwprintw(win, y, 0, "%-*s", MAX_LINE_LENGTH, line.c_str());
}

constexpr size_t bytes_per_line {16};

void format_and_print_line(WINDOW* win, const size_t offset, HexEditorOverlay& editor, const size_t current_win_line) {
    std::vector<unsigned char> buffer;
    for (size_t i = 0; i < bytes_per_line; ++i) {
        size_t const editor_offset {offset + i};
        if (editor_offset < editor.virtual_size()) {
            buffer.emplace_back(editor.readByte(editor_offset));
        }
    }

    if (buffer.size() == 0) {
        return;
    }

    // Construction de la ligne en hexadécimal et en ASCII
    std::string hex_line;
    std::string ascii_line;

    for (size_t i = 0; i < buffer.size(); ++i) {
        hex_line += std::format("{:02X} ", buffer[i]);
        ascii_line += (buffer[i] >= 32 && buffer[i] <= 126) ? static_cast<char>(buffer[i]) : '.';
    }

    // Compléter les espaces si la ligne n'est pas complète (16 octets)
    while (hex_line.size() < bytes_per_line * 3) {
        hex_line += "   ";
    }

    std::string const formatted_line = std::format("{:08X}: {}| {}", offset, hex_line, ascii_line);
    print_line(win, current_win_line, formatted_line);

}

// Fonction pour afficher un fichier en hexadécimal
void hex_dump(HexEditorOverlay& editor, WINDOW* win, size_t const max_lines, int start_line) {
    size_t offset = start_line * bytes_per_line;
    size_t current_line = start_line;

    // Lecture et affichage du fichier
    while (true) {
        const size_t current_win_line {current_line - start_line};
        format_and_print_line(win, offset, editor, current_win_line);

        offset += bytes_per_line;
        ++current_line;
        // Si on atteint la fin de l'écran, on attend une entrée
        if (current_win_line >= max_lines) {
            break;
        }
    }
}

// Fonction pour gérer la boucle événementielle et l'affichage dynamique
void event_loop(HexEditorOverlay& editor, WINDOW* win, size_t const max_lines) {
    int start_line = 0;    // La ligne à partir de laquelle on commence à afficher

    hex_dump(editor, win, max_lines, start_line);
    wrefresh(win);

    while (true) {

        int ch = wgetch(win);  // Attente de l'entrée de l'utilisateur

        switch (ch) {
            case KEY_UP:  // Déplacer vers le haut
                if (start_line > 0) {
                    start_line--;  // Remonter dans le fichier
                }
                hex_dump(editor, win, max_lines, start_line);
                wrefresh(win);
                break;

            case KEY_DOWN:  // Déplacer vers le bas
            {
                start_line++;  // Descendre dans le fichier
                if (wscrl(win, 1) == ERR) {
                    endwin();
                    std::cerr << "wscrl returned an error" << std::endl;
                    break;
                }
                size_t const last_line {start_line + max_lines - 1};
                size_t const offset { last_line * bytes_per_line };
                size_t const last_win_line { max_lines - 1 };
                format_and_print_line(win, offset, editor, last_win_line);
                wrefresh(win);

                // hex_dump(editor, win, max_lines, start_line);
                // wrefresh(win);
                break;
            }

            case 'q':  // Quitter avec 'q'
                return;

            default:
                break;
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <file>" << std::endl;
        return 1;
    }

    // Initialisation de ncurses
    initscr();
    raw();
    keypad(stdscr, TRUE);
    noecho();
    curs_set(0);
    cbreak();

    // Taille du terminal
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);

    // Créer une fenêtre pour afficher le texte
    WINDOW* win = newwin(max_y, max_x, 0, 0);  // Utilisation de max_y ici
    if (scrollok(win, TRUE) == ERR) {
        endwin();
        std::cerr << "Scrolling cannot be enabled." << std::endl;
        return EXIT_FAILURE;
    }
    keypad(win, TRUE);

    // Ouvrir le fichier
    MMapFile file(argv[1]);
    if (!file.is_open()) {
        endwin();
        std::cerr << "Erreur d'ouverture du fichier." << std::endl;
        return EXIT_FAILURE;
    }

    HexEditorOverlay editor(static_cast<uint8_t*>(file.data()), file.size());

    // Boucle d'événements
    event_loop(editor, win, max_y - 1);

    // Nettoyage
    endwin();
    return 0;
}

