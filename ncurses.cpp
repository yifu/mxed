#include <ncurses.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <format>
#include <ranges>

constexpr int MAX_LINE_LENGTH = 50;  // Longueur maximale des lignes

// Fonction pour afficher une ligne de texte
void print_line(int y, const std::string& line) {
    mvprintw(y, 0, "%-*s", MAX_LINE_LENGTH, line.c_str());
}

// Fonction pour afficher un fichier en hexadécimal
void hex_dump(std::ifstream& file, WINDOW* win, size_t const max_lines, int start_line) {
    constexpr int bytes_per_line = 16;
    std::vector<unsigned char> buffer(bytes_per_line);
    unsigned long offset = start_line * bytes_per_line;
    size_t current_line = start_line;

    // Déplacer le fichier au début de la section à afficher
    file.clear();
    file.seekg(offset);

    // Lecture et affichage du fichier
    while (true) {
        file.read(reinterpret_cast<char*>(buffer.data()), bytes_per_line);
        size_t bytes_read = file.gcount();

        if (bytes_read == 0) break;  // Fin du fichier

        // Construction de la ligne en hexadécimal et en ASCII
        std::string hex_line;
        std::string ascii_line;

        for (size_t i = 0; i < bytes_read; ++i) {
            hex_line += std::format("{:02X} ", buffer[i]);
            ascii_line += (buffer[i] >= 32 && buffer[i] <= 126) ? static_cast<char>(buffer[i]) : '.';
        }

        // Compléter les espaces si la ligne n'est pas complète (16 octets)
        while (hex_line.size() < bytes_per_line * 3) {
            hex_line += "   ";
        }

        std::string const formatted_line = std::format("{:08X}: {}| {}", offset, hex_line, ascii_line);
        size_t current_win_line {current_line - start_line};
        print_line(current_win_line, formatted_line);
        ++current_line;
        offset += bytes_read;

        // Rafraîchir l'écran
        wrefresh(win);

        // Si on atteint la fin de l'écran, on attend une entrée
        if (current_win_line >= max_lines) {
            break;
        }
    }
}

// Fonction pour gérer la boucle événementielle et l'affichage dynamique
void event_loop(std::ifstream& file, WINDOW* win, size_t const max_lines) {
    int start_line = 0;    // La ligne à partir de laquelle on commence à afficher

    while (true) {
        clear();  // Effacer l'écran à chaque itération
        hex_dump(file, win, max_lines, start_line);

        int ch = getch();  // Attente de l'entrée de l'utilisateur

        switch (ch) {
            case KEY_UP:  // Déplacer vers le haut
                if (start_line > 0) {
                    start_line--;  // Remonter dans le fichier
                }
                break;

            case KEY_DOWN:  // Déplacer vers le bas
                start_line++;  // Descendre dans le fichier
                break;

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

    // Taille du terminal
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);

    // Créer une fenêtre pour afficher le texte
    WINDOW* win = newwin(max_y, max_x, 0, 0);  // Utilisation de max_y ici
    scrollok(win, TRUE);  // Activer le défilement

    // Ouvrir le fichier
    std::ifstream file(argv[1], std::ios::binary);
    if (!file) {
        std::cerr << "Erreur d'ouverture du fichier." << std::endl;
        endwin();
        return 1;
    }

    // Boucle d'événements
    event_loop(file, win, max_y - 1);

    // Nettoyage
    endwin();
    return 0;
}

