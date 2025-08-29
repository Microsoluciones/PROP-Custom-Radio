# PROP Board Ultra - Generic Escape Room Template

A robust, reusable template for ESP32-based escape room projects with smart message routing and task-based architecture.

## Features

- **State-aware message filtering** - Prevents unwanted button press issues
- **Individual task queues** - Eliminates race conditions
- **Emergency restart system** - RF4 long press for complete recovery
- **Clean task architecture** - Preparation → Quest → Consequence phases
- **Hardware abstraction** - Ready for multiple project types

## Hardware Components

- ESP32 microcontroller
- RF remote control (4 buttons)
- DFPlayer Mini audio module
- 74HC595 shift register for outputs
- PCF8574 I2C expander for additional I/O
- Consistent pin assignments across projects

## Quick Start

1. Clone this template
2. Open in PlatformIO
3. Customize the TODO sections in `src/tasks.cpp`
4. Define your message filtering logic in `rfControllerTask`
5. Implement your hardware control functions
6. Upload and test

## Documentation

- **[ARCHITECTURE.md](ARCHITECTURE.md)** - System architecture and usage guide
- **[AI_CONTEXT.md](AI_CONTEXT.md)** - Complete guide for AI assistants
- **[src/instructions.txt](src/instructions.txt)** - Detailed technical documentation

## Project Structure

```
├── src/
│   ├── main.cpp          # System initialization
│   ├── tasks.cpp         # Main task logic (customize TODO sections)
│   ├── globals.h         # Global variables and structures
│   ├── functions.cpp     # Hardware control functions
│   ├── isr.cpp          # Interrupt service routines
│   └── instructions.txt  # Technical documentation
├── ARCHITECTURE.md       # System architecture guide
├── AI_CONTEXT.md        # AI assistant context
└── platformio.ini       # Build configuration
```

## License

This template is designed for escape room projects. Customize as needed for your specific use case.