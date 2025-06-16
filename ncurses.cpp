#include "MMapFile.h"
#include "HexEditorOverlay.h"

#include <ncurses.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <format>
#include <ranges>
#include <algorithm>

constexpr int MAX_LINE_LENGTH = 50;  // Longueur maximale des lignes

struct Windows {
    WINDOW *offsets_win;
    WINDOW *hex_win;
    WINDOW *ascii_win;
};

void print_in_window(WINDOW* win, int y, std::string const& line) {
    mvwprintw(win, y, 0, "%-*s", MAX_LINE_LENGTH, line.c_str());
}

constexpr int bytes_per_line {16};

void format_and_print_lines(Windows &windows, const size_t offset, HexEditorOverlay& editor, const size_t current_win_line) {
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

    // Compléter les espaces si la ligne n'est pas complète
    while (hex_line.size() < bytes_per_line * 3) {
        hex_line += "   ";
    }

    std::string const offset_line { std::format("{:08X}:", offset) };

    print_in_window(windows.offsets_win, current_win_line, offset_line);
    print_in_window(windows.hex_win, current_win_line, hex_line);
    print_in_window(windows.ascii_win, current_win_line, ascii_line);
}

// Fonction pour afficher un fichier en hexadécimal
void hex_dump(HexEditorOverlay& editor, Windows &windows, size_t const max_lines, int start_line) {
    size_t offset = start_line * bytes_per_line;
    size_t current_line = start_line;

    // Lecture et affichage du fichier
    while (true) {
        const size_t current_win_line {current_line - start_line};

        format_and_print_lines(windows, offset, editor, current_win_line);

        offset += bytes_per_line;
        ++current_line;
        // Si on atteint la fin de l'écran, on attend une entrée
        if (current_win_line >= max_lines) {
            break;
        }
    }
}

// Fonction pour gérer la boucle événementielle et l'affichage dynamique
void event_loop(HexEditorOverlay& editor, Windows &windows, size_t const max_printable_lines_per_win) {
    size_t start_line = 0;

    hex_dump(editor, windows, max_printable_lines_per_win, start_line);
    wrefresh(windows.offsets_win);
    wrefresh(windows.hex_win);
    wrefresh(windows.ascii_win);

    while (true) {
        wmove(windows.ascii_win, 0, 0);
        int ch = wgetch(windows.ascii_win);

        switch (ch) {
            case KEY_UP:  // Déplacer vers le haut
                if (start_line > 0) {
                    start_line--;  // Remonter dans le fichier
                    if (wscrl(windows.ascii_win, -1) == ERR) {
                        endwin();
                        std::cerr << "wscrl returned an error" << std::endl;
                        break;
                    }
                    size_t const first_printed_line { start_line };
                    size_t const offset { first_printed_line * bytes_per_line };
                    size_t const first_win_line { 0 };
                    format_and_print_lines(windows, offset, editor, first_win_line);
                    wrefresh(windows.ascii_win);
                }
                break;

            case KEY_DOWN:  // Déplacer vers le bas
            {
                start_line++;  // Descendre dans le fichier
                if (wscrl(windows.ascii_win, 1) == ERR) {
                    endwin();
                    std::cerr << "wscrl returned an error" << std::endl;
                    break;
                }
                size_t const last_printed_line { start_line + max_printable_lines_per_win };
                size_t const offset { last_printed_line * bytes_per_line };
                size_t const last_win_line { max_printable_lines_per_win - 1 };
                format_and_print_lines(windows, offset, editor, last_win_line);
                wrefresh(windows.ascii_win);
                break;
            }

            case KEY_PPAGE:
            {
                if (start_line <= max_printable_lines_per_win) {
                    start_line = 0;
                } else {
                    start_line -= max_printable_lines_per_win;
                }
                hex_dump(editor, windows, max_printable_lines_per_win, start_line);
                wrefresh(windows.ascii_win);
                break;
            }

            case KEY_NPAGE:
            {
                size_t const total_lines_in_the_file {(editor.virtual_size() + bytes_per_line - 1) / bytes_per_line};
                if (total_lines_in_the_file < max_printable_lines_per_win) {
                    break;
                }
                size_t const last_startable_line {total_lines_in_the_file - max_printable_lines_per_win};
                start_line = std::min(start_line + max_printable_lines_per_win, last_startable_line);
                hex_dump(editor, windows, max_printable_lines_per_win, start_line);
                wrefresh(windows.ascii_win);
                break;
            }

            case KEY_HOME:
            {
                start_line = 0;
                hex_dump(editor, windows, max_printable_lines_per_win, start_line);
                wrefresh(windows.ascii_win);
                break;
            }

            case KEY_END:
            {
                size_t const total_lines_in_the_file {(editor.virtual_size() + bytes_per_line - 1) / bytes_per_line};
                if (total_lines_in_the_file > max_printable_lines_per_win) {
                    start_line = total_lines_in_the_file - max_printable_lines_per_win;
                }
                hex_dump(editor, windows, max_printable_lines_per_win, start_line);
                wrefresh(windows.ascii_win);
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
    start_color();
    use_default_colors();
    init_pair(1, -1, -1);
    raw();
    keypad(stdscr, TRUE);
    noecho();
    curs_set(0);
    cbreak();

    // Taille du terminal
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);

    int constexpr max_x_offsets_win { 11 };
    int const max_x_hex_win { 3 * bytes_per_line };
    int const max_x_ascii_win { bytes_per_line };

    if (max_x < max_x_offsets_win * 2) {
        endwin();
        std::cerr << "Minimum size required not met." << std::endl;
        return EXIT_FAILURE;
    }

    Windows windows;

    windows.offsets_win = newwin(max_y, max_x_offsets_win, 0, 0);
    if (scrollok(windows.offsets_win, TRUE) == ERR) {
        endwin();
        std::cerr << "Scrolling cannot be enabled." << std::endl;
        return EXIT_FAILURE;
    }
    keypad(windows.offsets_win, TRUE);

    windows.hex_win = newwin(max_y, max_x_hex_win, 0, max_x_offsets_win);
    if (scrollok(windows.hex_win, TRUE) == ERR) {
        endwin();
        std::cerr << "Scrolling cannot be enabled." << std::endl;
        return EXIT_FAILURE;
    }
    keypad(windows.hex_win, TRUE);

    windows.ascii_win = newwin(max_y, max_x_ascii_win, 0, max_x_offsets_win + max_x_hex_win);
    if (scrollok(windows.ascii_win, TRUE) == ERR) {
        endwin();
        std::cerr << "Scrolling cannot be enabled." << std::endl;
        return EXIT_FAILURE;
    }
    keypad(windows.ascii_win, TRUE);

    // Ouvrir le fichier
    MMapFile file(argv[1]);
    if (!file.is_open()) {
        endwin();
        std::cerr << "Erreur d'ouverture du fichier." << std::endl;
        return EXIT_FAILURE;
    }

    HexEditorOverlay editor(static_cast<uint8_t*>(file.data()), file.size());

    // Boucle d'événements
    event_loop(editor, windows, max_y);

    // Nettoyage
    endwin();
    return 0;
}

