/*
    MemoBot - The Teachable C Chatbot

    Features:
    - ELIZA-inspired rule-based chatbot
    - Learns new replies from the user
    - Saves learned rules to memory.txt
    - Loads learned rules in future sessions
    - Saves chat history to chatlog.txt
    - Uses dynamic memory, structs, strings, file I/O, and functions

    Compile on Windows:
        gcc memobot.c -o memobot.exe

    Compile on Linux/macOS:
        gcc memobot.c -o memobot

    Run:
        ./memobot
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#endif

#define INITIAL_CAPACITY 20
#define MAX_KEYWORD 30
#define MAX_PATTERN 80
#define MAX_RESPONSE 120
#define MAX_RESPONSES 5
#define MAX_INPUT 256
#define MAX_HISTORY 100

#define MEMORY_FILE "memory.txt"
#define CHATLOG_FILE "chatlog.txt"

typedef struct {
    char keyword[MAX_KEYWORD];
    char pattern[MAX_PATTERN];
    char responses[MAX_RESPONSES][MAX_RESPONSE];
    int respCount;
    int learned;
} Rule;

Rule *ruleBase = NULL;
int ruleCount = 0;
int ruleCapacity = 0;

char history[MAX_HISTORY][MAX_INPUT + MAX_RESPONSE + 20];
int historyCount = 0;
int historyStart = 0;

int teachingMode = 0;
char lastUserInput[MAX_INPUT] = "";

enum Color {
    COLOR_WHITE,
    COLOR_CYAN,
    COLOR_GREEN,
    COLOR_YELLOW,
    COLOR_RED
};

void setColor(enum Color color);
void resetColor(void);
void clearScreen(void);
void pauseEnter(void);

void initializeRuleBase(void);
void addBuiltInRules(void);
void addRule(const char *keyword, const char *pattern, char responses[][MAX_RESPONSE], int respCount, int learned);
void ensureCapacity(void);
void freeRuleBase(void);

void loadMemory(void);
void appendRuleToMemory(Rule *rule);
void rewriteMemoryFile(void);
void resetLearnedRules(void);

void displayWelcome(void);
void displayHelp(void);
void addHistory(const char *speaker, const char *message);
void saveHistory(void);

void trim(char *str);
void toLowerCase(char *str);
void removePunctuation(char *str);
void preprocess(char *str);
void normalizeCommand(char *str);

int isCommand(const char *input);
int handleCommand(const char *input);

Rule *findMatchingRule(const char *input, char *captured);
int matchPattern(const char *input, const char *pattern, char *captured);
void reflectPronouns(const char *input, char *output);
void generateResponse(Rule *rule, const char *captured, char *response);

void learnNewRule(const char *originalInput, const char *responseTemplate);
void extractKeywordAndPattern(const char *input, char *keyword, char *pattern);
int isSignificantWord(const char *word);

void botSay(const char *message);
void userSay(const char *message);
void systemSay(const char *message);

int main(void) {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif
    char input[MAX_INPUT];
    char processed[MAX_INPUT];
    char captured[MAX_INPUT];
    char response[MAX_RESPONSE * 2];

    srand((unsigned int)time(NULL));

    initializeRuleBase();
    addBuiltInRules();
    loadMemory();

    displayWelcome();

    botSay("Hello. I am MemoBot. Tell me what is on your mind.");

    while (1) {
        setColor(COLOR_CYAN);
        printf("\nYou: ");
        resetColor();

        if (fgets(input, sizeof(input), stdin) == NULL) {
            break;
        }

        trim(input);

        if (strlen(input) == 0) {
            continue;
        }

        addHistory("You", input);

        if (isCommand(input)) {
            if (handleCommand(input)) {
                break;
            }
            continue;
        }

        if (teachingMode) {
            learnNewRule(lastUserInput, input);
            teachingMode = 0;
            botSay("Thank you! I've learned a new reply.");
            continue;
        }

        strcpy(processed, input);
        preprocess(processed);

        captured[0] = '\0';

        Rule *matchedRule = findMatchingRule(processed, captured);

        if (matchedRule != NULL) {
            generateResponse(matchedRule, captured, response);
            botSay(response);
        } else {
            strcpy(lastUserInput, input);
            teachingMode = 1;
            systemSay("I don't know how to reply to that.");
            systemSay("What should I say when someone says something like that?");
        }
    }

    botSay("Goodbye. Thank you for teaching me.");
    freeRuleBase();

    return 0;
}

void setColor(enum Color color) {
#ifdef _WIN32
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

    switch (color) {
        case COLOR_CYAN:
            SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
            break;
        case COLOR_GREEN:
            SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
            break;
        case COLOR_YELLOW:
            SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
            break;
        case COLOR_RED:
            SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_INTENSITY);
            break;
        case COLOR_WHITE:
        default:
            SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
            break;
    }
#else
    switch (color) {
        case COLOR_CYAN:
            printf("\033[96m");
            break;
        case COLOR_GREEN:
            printf("\033[92m");
            break;
        case COLOR_YELLOW:
            printf("\033[93m");
            break;
        case COLOR_RED:
            printf("\033[91m");
            break;
        case COLOR_WHITE:
        default:
            printf("\033[97m");
            break;
    }
#endif
}

void resetColor(void) {
#ifdef _WIN32
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
#else
    printf("\033[0m");
#endif
}

void clearScreen(void) {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

void pauseEnter(void) {
    printf("\nPress Enter to start chatting...");
    getchar();
}

void initializeRuleBase(void) {
    ruleCapacity = INITIAL_CAPACITY;
    ruleCount = 0;

    ruleBase = (Rule *)malloc(ruleCapacity * sizeof(Rule));

    if (ruleBase == NULL) {
        printf("Memory allocation failed.\n");
        exit(1);
    }
}

void ensureCapacity(void) {
    if (ruleCount >= ruleCapacity) {
        ruleCapacity *= 2;

        Rule *newBase = (Rule *)realloc(ruleBase, ruleCapacity * sizeof(Rule));

        if (newBase == NULL) {
            printf("Memory reallocation failed.\n");
            freeRuleBase();
            exit(1);
        }

        ruleBase = newBase;
    }
}

void addRule(const char *keyword, const char *pattern, char responses[][MAX_RESPONSE], int respCount, int learned) {
    int i;

    ensureCapacity();

    strncpy(ruleBase[ruleCount].keyword, keyword, MAX_KEYWORD - 1);
    ruleBase[ruleCount].keyword[MAX_KEYWORD - 1] = '\0';

    strncpy(ruleBase[ruleCount].pattern, pattern, MAX_PATTERN - 1);
    ruleBase[ruleCount].pattern[MAX_PATTERN - 1] = '\0';

    if (respCount > MAX_RESPONSES) {
        respCount = MAX_RESPONSES;
    }

    ruleBase[ruleCount].respCount = respCount;
    ruleBase[ruleCount].learned = learned;

    for (i = 0; i < respCount; i++) {
        strncpy(ruleBase[ruleCount].responses[i], responses[i], MAX_RESPONSE - 1);
        ruleBase[ruleCount].responses[i][MAX_RESPONSE - 1] = '\0';
    }

    ruleCount++;
}

void addBuiltInRules(void) {
    char r1[MAX_RESPONSES][MAX_RESPONSE] = {
        "Hello. How are you feeling today?",
        "Hi there. What would you like to talk about?"
    };
    addRule("hello", "hello *", r1, 2, 0);

    char r2[MAX_RESPONSES][MAX_RESPONSE] = {
        "Why do you feel %1?",
        "How long have you felt %1?",
        "What makes you feel %1?"
    };
    addRule("feel", "i feel *", r2, 3, 0);

    char r3[MAX_RESPONSES][MAX_RESPONSE] = {
        "Why are you %1?",
        "Do you enjoy being %1?",
        "What caused you to be %1?"
    };
    addRule("am", "i am *", r3, 3, 0);

    char r4[MAX_RESPONSES][MAX_RESPONSE] = {
        "I am sorry you are feeling sad.",
        "What do you think is making you sad?",
        "Talking about sadness can sometimes help."
    };
    addRule("sad", "* sad *", r4, 3, 0);

    char r5[MAX_RESPONSES][MAX_RESPONSE] = {
        "That sounds difficult. Can you tell me more?",
        "Why do you think this is making you stressed?",
        "Try taking a slow breath. What is causing the stress?"
    };
    addRule("stress", "* stress *", r5, 3, 0);

    char r6[MAX_RESPONSES][MAX_RESPONSE] = {
        "Tell me more about your family.",
        "How does your family make you feel?",
        "Family relationships can be important. Please continue."
    };
    addRule("family", "* family *", r6, 3, 0);

    char r7[MAX_RESPONSES][MAX_RESPONSE] = {
        "Why do you think about your mother?",
        "How is your relationship with your mother?"
    };
    addRule("mother", "* mother *", r7, 2, 0);

    char r8[MAX_RESPONSES][MAX_RESPONSE] = {
        "Why do you think about your father?",
        "How is your relationship with your father?"
    };
    addRule("father", "* father *", r8, 2, 0);

    char r9[MAX_RESPONSES][MAX_RESPONSE] = {
        "What makes you happy?",
        "That is nice to hear. Why are you happy?",
        "Happiness is important. Tell me more."
    };
    addRule("happy", "* happy *", r9, 3, 0);

    char r10[MAX_RESPONSES][MAX_RESPONSE] = {
        "Why do you want %1?",
        "Would getting %1 really help you?",
        "What would happen if you got %1?"
    };
    addRule("want", "i want *", r10, 3, 0);

    char r11[MAX_RESPONSES][MAX_RESPONSE] = {
        "Why do you need %1?",
        "Would it help if you had %1?",
        "What makes %1 necessary?"
    };
    addRule("need", "i need *", r11, 3, 0);

    char r12[MAX_RESPONSES][MAX_RESPONSE] = {
        "Why do you think you cannot %1?",
        "What is stopping you from %1?",
        "Have you tried to %1 before?"
    };
    addRule("can't", "i cant *", r12, 3, 0);

    char r13[MAX_RESPONSES][MAX_RESPONSE] = {
        "Why do you like %1?",
        "What do you enjoy about %1?"
    };
    addRule("like", "i like *", r13, 2, 0);

    char r14[MAX_RESPONSES][MAX_RESPONSE] = {
        "Why do you dislike %1?",
        "What bothers you about %1?"
    };
    addRule("hate", "i hate *", r14, 2, 0);

    char r15[MAX_RESPONSES][MAX_RESPONSE] = {
        "Dreams can be interesting. What do you think it means?",
        "Tell me more about this dream."
    };
    addRule("dream", "* dream *", r15, 2, 0);

    char r16[MAX_RESPONSES][MAX_RESPONSE] = {
        "School can be challenging. What happened?",
        "How do you feel about school?"
    };
    addRule("school", "* school *", r16, 2, 0);

    char r17[MAX_RESPONSES][MAX_RESPONSE] = {
        "Assignments can feel overwhelming. Which part is hardest?",
        "Do you want to break the assignment into smaller tasks?"
    };
    addRule("assignment", "* assignment *", r17, 2, 0);

    char r18[MAX_RESPONSES][MAX_RESPONSE] = {
        "Friendships can be complicated. Tell me more.",
        "How does your friend make you feel?"
    };
    addRule("friend", "* friend *", r18, 2, 0);

    char r19[MAX_RESPONSES][MAX_RESPONSE] = {
        "That sounds interesting. Please continue.",
        "Can you explain that in more detail?"
    };
    addRule("because", "* because *", r19, 2, 0);

    char r20[MAX_RESPONSES][MAX_RESPONSE] = {
        "Goodbye. I enjoyed our conversation.",
        "See you next time."
    };
    addRule("bye", "* bye *", r20, 2, 0);
}

void freeRuleBase(void) {
    free(ruleBase);
    ruleBase = NULL;
    ruleCount = 0;
    ruleCapacity = 0;
}

void loadMemory(void) {
    FILE *file = fopen(MEMORY_FILE, "r");

    if (file == NULL) {
        return;
    }

    char line[600];

    while (fgets(line, sizeof(line), file) != NULL) {
        char *token;
        char keyword[MAX_KEYWORD];
        char pattern[MAX_PATTERN];
        char responses[MAX_RESPONSES][MAX_RESPONSE];
        int respCount = 0;

        trim(line);

        if (strlen(line) == 0) {
            continue;
        }

        token = strtok(line, "|");
        if (token == NULL) continue;
        strncpy(keyword, token, MAX_KEYWORD - 1);
        keyword[MAX_KEYWORD - 1] = '\0';

        token = strtok(NULL, "|");
        if (token == NULL) continue;
        strncpy(pattern, token, MAX_PATTERN - 1);
        pattern[MAX_PATTERN - 1] = '\0';

        while ((token = strtok(NULL, "|")) != NULL && respCount < MAX_RESPONSES) {
            strncpy(responses[respCount], token, MAX_RESPONSE - 1);
            responses[respCount][MAX_RESPONSE - 1] = '\0';
            respCount++;
        }

        if (respCount > 0) {
            addRule(keyword, pattern, responses, respCount, 1);
        }
    }

    fclose(file);
}

void appendRuleToMemory(Rule *rule) {
    FILE *file = fopen(MEMORY_FILE, "a");

    if (file == NULL) {
        systemSay("Warning: Could not save memory to file.");
        return;
    }

    fprintf(file, "%s|%s", rule->keyword, rule->pattern);

    for (int i = 0; i < rule->respCount; i++) {
        fprintf(file, "|%s", rule->responses[i]);
    }

    fprintf(file, "\n");

    fclose(file);
}

void rewriteMemoryFile(void) {
    FILE *file = fopen(MEMORY_FILE, "w");

    if (file == NULL) {
        systemSay("Warning: Could not rewrite memory file.");
        return;
    }

    for (int i = 0; i < ruleCount; i++) {
        if (ruleBase[i].learned) {
            fprintf(file, "%s|%s", ruleBase[i].keyword, ruleBase[i].pattern);

            for (int j = 0; j < ruleBase[i].respCount; j++) {
                fprintf(file, "|%s", ruleBase[i].responses[j]);
            }

            fprintf(file, "\n");
        }
    }

    fclose(file);
}

void resetLearnedRules(void) {
    int writeIndex = 0;

    for (int readIndex = 0; readIndex < ruleCount; readIndex++) {
        if (!ruleBase[readIndex].learned) {
            if (writeIndex != readIndex) {
                ruleBase[writeIndex] = ruleBase[readIndex];
            }
            writeIndex++;
        }
    }

    ruleCount = writeIndex;
    rewriteMemoryFile();

    systemSay("All learned rules have been forgotten. Built-in rules remain.");
}

void displayWelcome(void) {
    clearScreen();

    setColor(COLOR_WHITE);
    printf("============================================================\n");
    printf("                         MemoBot                           \n");
    printf("============================================================\n");
    resetColor();

    setColor(COLOR_GREEN);
    printf("\n");
    printf("  __  __                      ____        _   \n");
    printf(" |  \\/  | ___ _ __ ___   ___ | __ )  ___ | |_ \n");
    printf(" | |\\/| |/ _ \\ '_ ` _ \\ / _ \\|  _ \\ / _ \\| __|\n");
    printf(" | |  | |  __/ | | | | | (_) | |_) | (_) | |_ \n");
    printf(" |_|  |_|\\___|_| |_| |_|\\___/|____/ \\___/ \\__|\n");
    printf("\n");
    resetColor();

    setColor(COLOR_YELLOW);
    printf("A teachable ELIZA-inspired chatbot written in C.\n\n");
    resetColor();

    printf("Commands:\n");
    printf("  #help    - Show commands\n");
    printf("  #memory  - Show how many rules MemoBot knows\n");
    printf("  #save    - Save chat history to chatlog.txt\n");
    printf("  #reset   - Forget all learned rules\n");
    printf("  #teach   - Teach a new reply for your previous message\n");
    printf("  #quit    - Exit MemoBot\n");
    printf("  #bye     - Exit MemoBot\n");

    printf("\nMemory file: %s\n", MEMORY_FILE);
    printf("Chat log:    %s\n", CHATLOG_FILE);

    pauseEnter();
    clearScreen();

    setColor(COLOR_WHITE);
    printf("════════════════════════════ MemoBot ════════════════════════════\n");
    resetColor();
}

void displayHelp(void) {
    systemSay("Available commands:");
    systemSay("#memory  - Show rule count");
    systemSay("#save    - Save conversation history");
    systemSay("#reset   - Forget learned rules");
    systemSay("#teach   - Teach a response for your last message");
    systemSay("#quit    - Exit the program");
    systemSay("#bye     - Exit the program");
}

void addHistory(const char *speaker, const char *message) {
    char entry[MAX_INPUT + MAX_RESPONSE + 20];

    snprintf(entry, sizeof(entry), "%s: %s", speaker, message);

    if (historyCount < MAX_HISTORY) {
        strcpy(history[historyCount], entry);
        historyCount++;
    } else {
        strcpy(history[historyStart], entry);
        historyStart = (historyStart + 1) % MAX_HISTORY;
    }
}

void saveHistory(void) {
    FILE *file = fopen(CHATLOG_FILE, "w");

    if (file == NULL) {
        systemSay("Could not save chat history.");
        return;
    }

    fprintf(file, "MemoBot Conversation Log\n");
    fprintf(file, "========================\n\n");

    if (historyCount < MAX_HISTORY) {
        for (int i = 0; i < historyCount; i++) {
            fprintf(file, "%s\n", history[i]);
        }
    } else {
        for (int i = 0; i < MAX_HISTORY; i++) {
            int index = (historyStart + i) % MAX_HISTORY;
            fprintf(file, "%s\n", history[index]);
        }
    }

    fclose(file);

    systemSay("Conversation saved to chatlog.txt.");
}

void trim(char *str) {
    int start = 0;
    int end = (int)strlen(str) - 1;
    int i;

    while (str[start] && isspace((unsigned char)str[start])) {
        start++;
    }

    while (end >= start && isspace((unsigned char)str[end])) {
        end--;
    }

    if (start > end) {
        str[0] = '\0';
        return;
    }

    for (i = 0; start <= end; start++, i++) {
        str[i] = str[start];
    }

    str[i] = '\0';
}

void toLowerCase(char *str) {
    for (int i = 0; str[i]; i++) {
        str[i] = (char)tolower((unsigned char)str[i]);
    }
}

void removePunctuation(char *str) {
    int writeIndex = 0;

    for (int readIndex = 0; str[readIndex]; readIndex++) {
        char c = str[readIndex];

        if (isalnum((unsigned char)c) || isspace((unsigned char)c) || c == '\'' || c == '*') {
            str[writeIndex++] = c;
        } else {
            str[writeIndex++] = ' ';
        }
    }

    str[writeIndex] = '\0';
}

void preprocess(char *str) {
    trim(str);
    toLowerCase(str);
    removePunctuation(str);
    trim(str);

    /*
        Normalize common contractions after punctuation processing.
        This keeps matching simple.
    */
    char temp[MAX_INPUT] = "";
    char copy[MAX_INPUT];
    char *token;

    strcpy(copy, str);
    token = strtok(copy, " ");

    while (token != NULL) {
        if (strcmp(token, "i'm") == 0) {
            strcat(temp, "i am ");
        } else if (strcmp(token, "cant") == 0 || strcmp(token, "can't") == 0) {
            strcat(temp, "cant ");
        } else if (strcmp(token, "you're") == 0) {
            strcat(temp, "you are ");
        } else {
            strcat(temp, token);
            strcat(temp, " ");
        }

        token = strtok(NULL, " ");
    }

    trim(temp);
    strcpy(str, temp);
}

void normalizeCommand(char *str) {
    trim(str);
    toLowerCase(str);
}

int isCommand(const char *input) {
    return input[0] == '#';
}

int handleCommand(const char *input) {
    char command[MAX_INPUT];

    strcpy(command, input);
    normalizeCommand(command);

    if (strcmp(command, "#quit") == 0 || strcmp(command, "#bye") == 0) {
        return 1;
    }

    if (strcmp(command, "#help") == 0) {
        displayHelp();
    } else if (strcmp(command, "#memory") == 0) {
        char message[100];
        snprintf(message, sizeof(message), "I currently know %d rules.", ruleCount);
        systemSay(message);
    } else if (strcmp(command, "#save") == 0) {
        saveHistory();
    } else if (strcmp(command, "#reset") == 0) {
        resetLearnedRules();
    } else if (strcmp(command, "#teach") == 0) {
        if (strlen(lastUserInput) == 0) {
            systemSay("There is no previous message to teach me about.");
        } else {
            teachingMode = 1;
            systemSay("What should I say when someone says something like your last message?");
        }
    } else {
        systemSay("Unknown command. Type #help to see available commands.");
    }

    return 0;
}

Rule *findMatchingRule(const char *input, char *captured) {
    for (int i = 0; i < ruleCount; i++) {
        if (strstr(input, ruleBase[i].keyword) != NULL) {
            if (matchPattern(input, ruleBase[i].pattern, captured)) {
                return &ruleBase[i];
            }
        }
    }

    /*
        Second pass: if keyword appears but pattern is too specific,
        still allow a simpler keyword match.
    */
    for (int i = 0; i < ruleCount; i++) {
        if (strstr(input, ruleBase[i].keyword) != NULL) {
            strcpy(captured, input);
            return &ruleBase[i];
        }
    }

    return NULL;
}

int matchPattern(const char *input, const char *pattern, char *captured) {
    char patternCopy[MAX_PATTERN];
    char beforeStar[MAX_PATTERN] = "";
    char afterStar[MAX_PATTERN] = "";

    strcpy(patternCopy, pattern);

    char *star = strchr(patternCopy, '*');

    if (star == NULL) {
        if (strcmp(input, patternCopy) == 0) {
            captured[0] = '\0';
            return 1;
        }
        return 0;
    }

    *star = '\0';
    strcpy(beforeStar, patternCopy);
    strcpy(afterStar, star + 1);

    trim(beforeStar);
    trim(afterStar);

    const char *startPosition = input;

    if (strlen(beforeStar) > 0) {
        char *foundBefore = strstr(input, beforeStar);

        if (foundBefore == NULL) {
            return 0;
        }

        startPosition = foundBefore + strlen(beforeStar);
    }

    const char *endPosition = input + strlen(input);

    if (strlen(afterStar) > 0) {
        char *foundAfter = strstr(startPosition, afterStar);

        if (foundAfter == NULL) {
            return 0;
        }

        endPosition = foundAfter;
    }

    int length = (int)(endPosition - startPosition);

    if (length < 0) {
        return 0;
    }

    while (length > 0 && isspace((unsigned char)*startPosition)) {
        startPosition++;
        length--;
    }

    while (length > 0 && isspace((unsigned char)startPosition[length - 1])) {
        length--;
    }

    strncpy(captured, startPosition, length);
    captured[length] = '\0';

    return 1;
}

void reflectPronouns(const char *input, char *output) {
    char copy[MAX_INPUT];
    char *token;

    output[0] = '\0';

    strcpy(copy, input);
    token = strtok(copy, " ");

    while (token != NULL) {
        if (strcmp(token, "i") == 0) {
            strcat(output, "you");
        } else if (strcmp(token, "me") == 0) {
            strcat(output, "you");
        } else if (strcmp(token, "my") == 0) {
            strcat(output, "your");
        } else if (strcmp(token, "mine") == 0) {
            strcat(output, "yours");
        } else if (strcmp(token, "you") == 0) {
            strcat(output, "I");
        } else if (strcmp(token, "your") == 0) {
            strcat(output, "my");
        } else if (strcmp(token, "yours") == 0) {
            strcat(output, "mine");
        } else if (strcmp(token, "am") == 0) {
            strcat(output, "are");
        } else if (strcmp(token, "are") == 0) {
            strcat(output, "am");
        } else {
            strcat(output, token);
        }

        token = strtok(NULL, " ");

        if (token != NULL) {
            strcat(output, " ");
        }
    }
}

void generateResponse(Rule *rule, const char *captured, char *response) {
    int index = rand() % rule->respCount;
    char reflected[MAX_INPUT];
    char *placeholder;

    strcpy(response, rule->responses[index]);

    reflectPronouns(captured, reflected);

    placeholder = strstr(response, "%1");

    if (placeholder != NULL) {
        char finalResponse[MAX_RESPONSE * 2];
        int beforeLength = (int)(placeholder - response);

        strncpy(finalResponse, response, beforeLength);
        finalResponse[beforeLength] = '\0';

        strcat(finalResponse, reflected);
        strcat(finalResponse, placeholder + 2);

        strcpy(response, finalResponse);
    }
}

void learnNewRule(const char *originalInput, const char *responseTemplate) {
    char processed[MAX_INPUT];
    char keyword[MAX_KEYWORD];
    char pattern[MAX_PATTERN];
    char responses[MAX_RESPONSES][MAX_RESPONSE];

    strcpy(processed, originalInput);
    preprocess(processed);

    extractKeywordAndPattern(processed, keyword, pattern);

    strncpy(responses[0], responseTemplate, MAX_RESPONSE - 1);
    responses[0][MAX_RESPONSE - 1] = '\0';

    addRule(keyword, pattern, responses, 1, 1);

    appendRuleToMemory(&ruleBase[ruleCount - 1]);
}

void extractKeywordAndPattern(const char *input, char *keyword, char *pattern) {
    char copy[MAX_INPUT];
    char *token;
    char selected[MAX_KEYWORD] = "";

    strcpy(copy, input);
    token = strtok(copy, " ");

    while (token != NULL) {
        if (isSignificantWord(token)) {
            strncpy(selected, token, MAX_KEYWORD - 1);
            selected[MAX_KEYWORD - 1] = '\0';
        }

        token = strtok(NULL, " ");
    }

    if (strlen(selected) == 0) {
        strcpy(selected, "misc");
    }

    strcpy(keyword, selected);

    /*
        Create a simple learned pattern.
        If the original sentence contains "i am", use "i am *".
        If it contains "i feel", use "i feel *".
        Otherwise use a broad keyword pattern "* keyword *".
    */
    if (strstr(input, "i am") != NULL) {
        strcpy(pattern, "i am *");
    } else if (strstr(input, "i feel") != NULL) {
        strcpy(pattern, "i feel *");
    } else if (strstr(input, "i want") != NULL) {
        strcpy(pattern, "i want *");
    } else if (strstr(input, "i need") != NULL) {
        strcpy(pattern, "i need *");
    } else {
        snprintf(pattern, MAX_PATTERN, "* %s *", keyword);
    }
}

int isSignificantWord(const char *word) {
    const char *ignoreWords[] = {
        "the", "and", "but", "for", "you", "are", "was", "were",
        "this", "that", "with", "have", "has", "had", "not",
        "very", "really", "just", "about", "into", "from",
        "what", "when", "where", "why", "how", "can", "could",
        "would", "should", "i", "am", "is", "to", "of", "in",
        "on", "at", "it", "a", "an", "so"
    };

    int ignoreCount = sizeof(ignoreWords) / sizeof(ignoreWords[0]);

    if (strlen(word) <= 2) {
        return 0;
    }

    for (int i = 0; i < ignoreCount; i++) {
        if (strcmp(word, ignoreWords[i]) == 0) {
            return 0;
        }
    }

    return 1;
}

void botSay(const char *message) {
    setColor(COLOR_GREEN);
    printf("Bot: %s\n", message);
    resetColor();

    addHistory("Bot", message);
}

void userSay(const char *message) {
    setColor(COLOR_CYAN);
    printf("You: %s\n", message);
    resetColor();

    addHistory("You", message);
}

void systemSay(const char *message) {
    setColor(COLOR_YELLOW);
    printf("[MemoBot] %s\n", message);
    resetColor();

    addHistory("[MemoBot]", message);
}