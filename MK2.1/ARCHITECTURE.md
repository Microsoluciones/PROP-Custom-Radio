## Architecture Components
# PROP Board Ultra - Generic Escape Room Template Architecture

## Overview
This is a generic template for escape room projects using ESP32 with a clean task-based architecture and smart message routing.

### 1. Main Task (Coordinator)
- **Role**: Central coordinator that manages the game flow
- **Function**: `mainTask()`
- **Responsibility**: 
  - Manages state transitions between different game phases
  - Creates and destroys phase-specific tasks as needed
  - Monitors task completion and coordinates the overall game flow
  - Handles system initialization and reset

### 2. Game States
The game operates in four distinct states:
- `STATE_IDLE`: Waiting for initial RF1 short press to start
- `STATE_PREPARATION`: Setup and configuration phase
- `STATE_QUEST`: Active game/puzzle phase (can end in win or lose)
- `STATE_CONSEQUENCE`: Post-game results and cleanup (handles win/lose)

### 3. Emergency Restart System
- **RF4 (Red Button)**: Emergency kill switch
- **Function**: Long press on RF4 at any time during operation
- **Complete System Reset**:
  1. Kills ALL active tasks (main, preparation, quest, consequence)
  2. Resets all hardware to safe state
  3. Resets all global variables to default values
  4. Clears message queue of any pending commands
  5. Automatically restarts main task in STATE_IDLE
- **Purpose**: Complete system recovery if any task becomes unresponsive or system enters invalid state

### 4. Phase-Specific Tasks

#### Preparation Task
- **Function**: `preparationTask()`
- **Purpose**: Handles system setup and configuration
- **Lifecycle**: Created when RF1 short press is detected, deleted when preparation completes

#### Quest Task
- **Function**: `questTask()`
- **Purpose**: Main game/puzzle logic
- **Lifecycle**: Created after preparation, deleted when game ends
- **Ending**: Can end in two ways:
  - By user action (RF3 long press): lose scenario (`questSuccess = false`)
  - By hardware/game logic: win scenario (`questSuccess = true`)

#### Consequence Task
- **Function**: `consequenceTask()`
- **Purpose**: End game processing, results, cleanup
- **Lifecycle**: Created after quest ends, deleted after restart decision
- **Win/Lose Handling**: Checks `questSuccess` to run win or lose scenario logic (lights, audio, etc)

### 5. Message Routing System
- **rfControllerTask**: Smart message router that filters RF button presses based on current game state
- **Individual Task Queues**: Each task has its own message queue to prevent race conditions
- **State-Aware Filtering**: Only valid button combinations for current state are forwarded

## How to Use for New Projects

### Quick Start
1. Clone this template
2. Customize the TODO sections in each task function
3. Define your project-specific message filtering in `rfControllerTask`
4. Implement your hardware control functions
5. Configure shift register outputs for your specific hardware
6. Set up audio tracks and PCF8574 I/O as needed

### Key Customization Points
- **Message Filtering**: Define which RF buttons are valid in each state
- **Hardware Control**: Implement project-specific output functions
- **Game Logic**: Fill in preparation, quest, and consequence phase logic
- **Audio/Visual**: Configure audio tracks and visual feedback
- **I/O Expansion**: Use PCF8574 for additional sensors/actuators

## Global Variables

- `currentGameState`: Current phase of the game (IDLE, PREPARATION, QUEST, CONSEQUENCE)
- `systemReady`: Flag indicating system readiness
- `emergencyRestart`: Flag for emergency restart detection
- `questSuccess`: True if quest completed successfully (win), false if failed (lose)
- Task handles for dynamic task management

This template provides a clean, safe, and efficient structure for escape room projects with proper emergency handling and race condition prevention.

## RF Message Handling and Filtering

### Unified Queue and Robust Fetching

All tasks now use a single queue for RF messages. To fetch only valid and fresh messages, use:

```cpp
bool getRfValidMessage(MainTaskMsg* outMsg, bool (*isValid)(const MainTaskMsg&));
```

- Skips old messages (older than 2 seconds)
- Skips unwanted messages (not valid for the current phase)
- Returns true if a valid message is found, false if timeout

This approach prevents the queue from being blocked and ensures only relevant messages are processed by each phase.

## Win/Lose Scenario Handling

The template now supports two ways to end the quest phase and handles win/lose logic in the consequence phase using the `questSuccess` variable.