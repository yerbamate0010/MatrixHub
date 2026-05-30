#ifndef MACRO_DEFINITIONS_H
#define MACRO_DEFINITIONS_H

#include <Arduino.h>
#include <new>
#include <vector>
#include <string>
#include <memory>

#include "../../system/memory/PsramAllocator.h"

namespace MACROS {
using SYSTEM::PsramAllocator;
using SYSTEM::PsramString;
using SYSTEM::PsramVector;

/**
 * @brief Custom Allocator to force container storage into ESP32-S3 PSRAM.
 * Prevents heap fragmentation in internal SRAM.
 */

// Enum definitions
enum class MacroStatus {
    IDLE,
    RUNNING,
    PAUSED, 
    ERROR,
    COMPLETED
};

// DuckyScript Command Types
enum class CommandType {
    UNKNOWN,
    REM,
    DELAY,
    DEFAULT_DELAY,
    STRING,
    STRINGLN,
    KEY,      
    COMBO,    
    REPEAT,
    // Extensions
    MOUSE_MOVE,
    MOUSE_CLICK,
    MATRIX_PRINT,
    SYSTEM_CONTROL,  // Power Off, Sleep, Wake
    CONSUMER,        // Media keys (Volume, Play, etc.) - uses 16-bit usage code
    
    // Gaming / Advanced
    PRESS_KEY,      // Press and hold
    RELEASE_KEY,    // Release specific key
    RELEASE_ALL     // Release all keys
};

// Modifier bitmask flags for multi-key combos (e.g., CTRL ALT DELETE)
constexpr uint8_t MOD_CTRL  = 0x01;
constexpr uint8_t MOD_SHIFT = 0x02;
constexpr uint8_t MOD_ALT   = 0x04;
constexpr uint8_t MOD_GUI   = 0x08;

// Internal Command Structure (Parsed)
struct MacroCommand {
    CommandType type = CommandType::UNKNOWN;
    PsramString textData;     // Stores STRING argument or generic text
    uint32_t numericData = 0; // REPEAT count or DELAY ms
    uint8_t modifiers = 0;    
    uint16_t key = 0;

    MacroCommand() : textData(PsramAllocator<char>()) {}
};

// Service State
struct MacroState {
    PsramString currentScript;
    MacroStatus status = MacroStatus::IDLE;
    uint32_t currentLine = 0;
    unsigned long startTime = 0;
    PsramString lastError;

    MacroState() 
        : currentScript(PsramAllocator<char>()), 
          lastError(PsramAllocator<char>()) {}
};



} // namespace MACROS

#endif // MACRO_DEFINITIONS_H
