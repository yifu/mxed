#pragma once

#include "Edit.h"

#include <assert.h>

#include <ranges>

class HexEditorOverlay {
public:
    HexEditorOverlay(uint8_t* basePtr, size_t fileSize)
        : base(basePtr), size(fileSize) {}

    void addInsert(size_t offset, const std::vector<uint8_t>& data) {
        edits.push_back({offset, EditType::INSERT, data, 0});
    }

    void addDelete(size_t offset, size_t length) {
        edits.push_back({offset, EditType::DELETE, {}, length});
    }

    void addReplace(size_t offset, const std::vector<uint8_t>& data) {
        edits.push_back({offset, EditType::REPLACE, data, data.size()});
    }

    uint8_t readByte(size_t virtualOffset) {
        for (auto const& edit : edits | std::views::reverse) {
            if(edit.type == EditType::INSERT) {
                if (virtualOffset >= edit.offset + edit.data.size()) {
                    virtualOffset -= edit.data.size();
                } else if (virtualOffset >= edit.offset && virtualOffset < edit.offset + edit.data.size()) {
                    return edit.data[virtualOffset - edit.offset];
                }
            } else if (edit.type == EditType::REPLACE) {
                if (virtualOffset >= edit.offset + edit.data.size()) {
                } else if (virtualOffset >= edit.offset && virtualOffset < edit.offset + edit.data.size()) {
                    return edit.data[virtualOffset - edit.offset];
                }
            } else if (edit.type == EditType::DELETE) {
                if (virtualOffset >= edit.offset) {
                    virtualOffset += edit.length;
                }
            }
        }
        assert(virtualOffset < size);
        return base[virtualOffset];
    }

private:
    uint8_t* base;
    size_t size;
    std::vector<Edit> edits;
};

