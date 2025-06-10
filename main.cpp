#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include <vector>

#include "HexEditorOverlay.h"

int main() {
    int fd = open("fichier.bin", O_RDONLY);
    size_t fileSize = lseek(fd, 0, SEEK_END);
    uint8_t* ptr = (uint8_t*) mmap(NULL, fileSize, PROT_READ, MAP_PRIVATE, fd, 0);

    HexEditorOverlay editor(ptr, fileSize);

    // Insertion de deux octets à l’offset 10
    editor.addInsert(10, {0xAA, 0xBB});

    // Remplacement de 3 octets à l’offset 20
    editor.addReplace(20, {0x01, 0x02, 0x03});

    // Delete
    editor.addDelete(25, 3);

    // Remove added bytes at the beginning
    editor.addDelete(10, 2);

    // Lecture virtuelle avec patchs appliqués
    for (size_t i = 0; i < 30; ++i) {
        printf("%02zu ", i);
    }
    printf("\n");

    for (size_t i = 0; i < 30; ++i) {
        printf("%02X ", editor.readByte(i));
    }
    printf("\n");

    munmap(ptr, fileSize);
    close(fd);
    return 0;
}

