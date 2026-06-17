# MemoBot – The Teachable C Chatbot

MemoBot is a small ELIZA-inspired chatbot written in C. It starts with built-in conversation rules, asks the user to teach it when it does not know how to respond, and saves learned replies so future sessions can reuse them.

## Features

- Rule-based keyword and pattern matching inspired by classic ELIZA-style chatbots.
- Teachable responses: unknown inputs trigger a teaching prompt.
- Persistent memory through `memory.txt`.
- Conversation history saving through `chatlog.txt`.
- Console colors for user messages, bot replies, and system messages.
- Special commands for help, memory count, saving, resetting learned rules, teaching, and quitting.
- No external libraries required; it uses standard C and optional Windows console APIs.

## Requirements

- A C compiler such as GCC.
- Linux, macOS, or Windows.

## Build and Run

### Linux or macOS

Compile:

```bash
gcc -Wall -Wextra -std=c11 memobot.c -o memobot
```

Run:

```bash
./memobot
```

### Windows with GCC / MinGW

Compile:

```cmd
gcc -Wall -Wextra -std=c11 memobot.c -o memobot.exe
```

Run:

```cmd
memobot.exe
```

## How to Use

When MemoBot starts, press Enter after the welcome screen, then type messages at the `You:` prompt.

Example conversation:

```text
You: I feel sad today
Bot: Why do you feel sad today?

You: I love pizza
[MemoBot] I don't know how to reply to that.
[MemoBot] What should I say when someone says something like that?

You: Pizza sounds delicious! What is your favourite topping?
Bot: Thank you! I've learned a new reply.

You: I love pizza so much
Bot: Pizza sounds delicious! What is your favourite topping?
```

## Commands

| Command | Description |
| --- | --- |
| `#help` | Show the command list. |
| `#memory` | Show how many rules MemoBot currently knows. |
| `#save` | Save the current conversation history to `chatlog.txt`. |
| `#reset` | Forget all learned rules while keeping built-in rules. |
| `#teach` | Teach a reply for the previous user message. |
| `#quit` | Exit MemoBot. |
| `#bye` | Exit MemoBot. |

Commands are case-insensitive and may include extra leading or trailing spaces, for example ` #MeMoRy `.

## Persistent Files

- `memory.txt` stores learned rules in this format:

```text
keyword|pattern|response1|response2|...
```

- `chatlog.txt` is created when you run `#save` and contains the conversation history.

## Project Structure

```text
.
├── README.md      # Project documentation and run instructions
├── memobot.c      # Main C source file
├── memory.txt     # Learned chatbot rules loaded at startup
└── memobot        # Optional compiled binary, if present
```

## Notes for Demonstration

A good live demo is to teach MemoBot a topic it does not know yet, then immediately ask about the same topic again. This shows learning, dynamic memory, file I/O, and rule matching in a simple way that is easy to explain.
