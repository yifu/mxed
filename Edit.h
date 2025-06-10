#pragma once

#include <vector>

enum class EditType {
    INSERT,
    DELETE,
    REPLACE
};

struct Edit {
    size_t offset;
    EditType type;
    std::vector<uint8_t> data; // For INSERT and REPLACE
    size_t length;             // For DELETE and REPLACE
};

