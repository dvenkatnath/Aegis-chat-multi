# CyberKnot Project

## Overview
The CyberKnot project is a collection of cryptographic tools designed to provide secure communication channels. It includes various modules such as AES, Base64, HMAC, and more.

## Files
- **aes.c**: Implementation of AES encryption.
- **Base64.c**: Base64 encoding/decoding functions.
- **EncryptionToolsApi.c**: API for encryption tools.
- **Hmac.c**: HMAC (Hash-based Message Authentication Code) implementation.
- **HighLevED.c**: High-level encryption and decryption utilities.
- **Global2.c**: Global variables and constants.
- **SHA29system
- **Functions3.c**: Additional utility functions.
- **Pstr.c**: String manipulation utilities.
- **UberHighLevED.c**: Advanced encryption and decryption utilities.
- **ProcMSG.c**: Message processing utilities.
- **API.c**: API for various cryptographic operations.
- **Main.Kobi.Workflow2.c**: Test program to verify library functionality

## Test Program - Main.Kobi.Workflow2.c

This file serves as the comprehensive test suite for validating CyberKnot's core functionalities.

### Key Testing Areas
| Test Type | Description |
|-----------|-------------|
| **Encryption/Decryption** | Validates PUEncrypt and PUDecrypt round-trip consistency |
| **Message Handling** | Tests MessageHandler for proper packet processing |
| **ACK/NAK Mechanisms** | Verifies communication acknowledgment protocols |
| **Session Setup** | Ensures secure communications channel establishment |

### Key Testing Patterns
```c
// Encryption verification pattern
for (int k = 0; k < TEST_ITERATIONS; k++) {
    encrypted = PUEncrypt(plaintext, plaintext_len);
    decrypted = PUDecrypt(encrypted);
    
    assert(memcmp(plaintext, decrypted, plaintext_len) == 0);
}

// Message handler simulation
binary_data test_message = create_test_message();
processed_data = MessageHandler(test_message);
assert(processed_data.status == SUCCESS);
```

## Build Instructions

Here are the available build configurations:

### Linux Static Library (`Makefile-A`)
```sh
make -f src/Makefile-A
```
- Compiler: `gcc`
- Output: `libCyberKnot.a`
- Flags: Debug mode (-g, -O0), PIC support
- Use case: Statically linkable library for Linux applications

### Linux Shared Library (`Makefile-SO`)
```sh
make -f src/Makefile-SO
```
- Compiler: `gcc`
- Output: `libCyberKnot.so`
- Flags: Optimization (-O2), PIC support with debugging
- Use case: Dynamic linking for Linux services

### Windows Cross-Compilation (`Makefile-A-Win`)
```sh
make -f src/Makefile-A-Win
```
- Compiler: `x86_64-w64-mingw32-gcc`
- Output: `libCyberKnot.lib` (Windows .lib format)
- Flags: Windows-specific (_WIN32 macro)
- Use case: Cross-compiling for Windows applications

Using VS Code tasks defined in `src/.vscode/tasks.json`, you can execute these build commands by clicking the corresponding buttons in the Tasks panel. This allows for a streamlined development workflow.

## Usage

To use the CyberKnot library in your own projects, include the necessary header files from the `include/` directory and link against `libCyberKnot.a`.

Example with specific configurations:
```sh
# For Linux static linking
g++ -o myapp_linux_static myapp.cpp -L./src -lCyberKnot -I./src/include --static

# For Linux dynamic linking
g++ -o myapp_linux_shared myapp.cpp -L./src -l:CyberKnot.so.1 -I./src/include

# For Windows (on Linux via MinGW)
x86_64-w64-mingw32-g++ -o myapp_windows myapp.cpp -L./src -lCyberKnot -I./src/include


### Usage Overview

#### Library Integration
```c
// In your project's source file
#include "api.h"
#include "common.h"

int main
