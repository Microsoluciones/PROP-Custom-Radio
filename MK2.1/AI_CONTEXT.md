# AI Assistant Context - PROP Board Ultra Template

## Project Vision & Purpose

This is a **generic escape room board template** designed to be reused across multiple escape room projects. The template provides a robust foundation with hardware abstraction and task-based architecture that eliminates common issues like race conditions and unwanted button press handling.

## Core Problem It Solves

**Original Issue**: In escape room systems, button presses can interfere across different game phases. For example, if the system expects "RF1 then RF4" and a user presses RF4 first, that RF4 press shouldn't be processed later when RF1 is finally pressed.

**Solution**: Smart message routing with state-aware filtering. The `rfControllerTask` only forwards button messages that are valid for the current game state, automatically discarding invalid combinations.

## Template Architecture

### Hardware Stack (Consistent Across All Projects)
- **ESP32**: Main microcontroller
- **RF Remote**: 4-button controller (RF1-RF4)
- **DFPlayer Mini**: Audio module with microSD
- **74HC595**: Shift register for digital outputs
- **PCF8574**: I2C expander for additional I/O (sensors, inputs)
- **Fixed Pin Assignments**: Same across all boards for consistency

### Software Framework
- **FreeRTOS Tasks**: Clean separation of concerns
- **State Machine**: IDLE → PREPARATION → QUEST → CONSEQUENCE
- **Individual Task Queues**: Eliminates race conditions
- **Emergency Restart**: RF4 long press for complete system recovery

## How to Use This Template for New Projects

### For AI Assistants Working on New Projects:

1. **This is a TEMPLATE** - Don't remove the framework, only customize the TODO sections
2. **Keep the task structure** - mainTask, preparationTask, questTask, consequenceTask
3. **Customize the TODO sections** in each task for project-specific logic
4. **Define message filtering** in rfControllerTask for your project's button behavior
5. **Implement hardware functions** for your specific escape room puzzle
6. **Configure PCF8574** usage for your sensors/actuators
7. **Set up audio tracks** and shift register outputs for your hardware
8. **dont use goto statments**

### Key Files to Customize:
- **`tasks.cpp`**: Main logic in TODO sections of each task
- **`globals.h`**: Project-specific variables and structures
- **`functions.cpp`**: Hardware control functions
- **`rfControllerTask`**: Message filtering rules

### What NOT to Change:
- Task creation/destruction logic in mainTask
- Emergency restart system (RF4 long press)
- Queue management and ISR handling
- Basic hardware initialization
- Pin assignments (consistent across all projects)

## Common Escape Room Patterns

### Preparation Phase Usually Handles:
- Time/difficulty selection
- Player count configuration
- System calibration
- Audio/visual setup confirmation

### Quest Phase Usually Handles:
### Quest Phase Usually Handles:
- Main puzzle logic
- Timer management
- Win/lose conditions (set questSuccess = true for win, false for lose)
- Multi-stage puzzles
- Sensor monitoring

#### Ending the Quest Phase
- The quest phase can end in two ways:
	- By user action (RF3 long press): sets questSuccess = false (lose scenario)
	- By hardware/game logic: set questSuccess = true and end the phase (win scenario)

### Consequence Phase Usually Handles:
- Results display
- Score calculation
- Photo/video capture
- System cleanup
- Restart decisions
- Handles win/lose scenarios based on questSuccess:
	- If questSuccess is true: run WIN logic (lights, audio, etc)
	- If questSuccess is false: run LOSE logic (lights, audio, etc)
- Restart decisions
### Key Files to Customize:
- **`tasks.cpp`**: Main logic in TODO sections of each task. See questTask for win/lose logic and consequenceTask for scenario handling.
- **`globals.h`**: Project-specific variables and structures. questSuccess is a global variable for win/lose state.
- **`functions.cpp`**: Hardware control functions
- **`rfControllerTask`**: Message filtering rules
- **Eliminates Race Conditions**: No shared queue conflicts
### What NOT to Change:
- Task creation/destruction logic in mainTask
- Emergency restart system (RF4 long press)
- Queue management and ISR handling
- Basic hardware initialization
- Pin assignments (consistent across all projects)
- questSuccess variable definition and its use in quest/consequence phases
- **Robust Operation**: System doesn't break from unexpected input
## RF Message Handling and Filtering & Win/Lose Logic
This ensures robust, error-proof queue management and is the recommended way to handle RF messages in all new projects.

## Win/Lose Scenario Example

In `questTask`:
```cpp
if (puzzleSolved()) {
		questSuccess = true;
		questComplete = true;
}
```
In `consequenceTask`:
```cpp
if (questSuccess) {
		// WIN logic
} else {
		// LOSE logic
}
```

### Why Emergency Restart?
- **Recovery Mechanism**: Complete system reset if anything goes wrong
- **Development Safety**: Easy to recover during testing
- **Fail-Safe Operation**: Ensures system never gets stuck

## Template Usage Philosophy

This template is designed for **rapid escape room development** where:
1. Hardware is consistent across projects
2. Basic task flow is similar (prep → game → results)
3. Message handling needs to be robust and error-free
4. Development time should focus on puzzle logic, not infrastructure

When working with this template, focus your effort on the creative puzzle mechanics within the established framework rather than rebuilding the foundational systems.

## RF Message Handling and Filtering

### How to Use `getRfValidMessage`

To robustly fetch only valid and fresh RF messages in any task, use:

```cpp
bool getRfValidMessage(MainTaskMsg* outMsg, bool (*isValid)(const MainTaskMsg&));
```

- Skips old messages (older than 2 seconds)
- Skips unwanted messages (not valid for the current phase)
- Returns true if a valid message is found, false if timeout

**Example:**

```cpp
auto isValid = [](const MainTaskMsg& m) { return (m.channel == 0 && m.type == LONG_PRESS); };
if (getRfValidMessage(&msg, isValid)) {
	// Process valid message
}
```

This ensures robust, error-proof queue management and is the recommended way to handle RF messages in all new projects.