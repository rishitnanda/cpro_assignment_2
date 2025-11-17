#ifndef UTILS_H
#define UTILS_H

#include "structures.h"

#define MAX_LINE 1024
#define MAX_TOKENS 128

extern CommandDef commands[];

Command parseCommand(char *line);
void freeCommand(Command *cmd);
int matchCommand(Command *cmd, CommandDef *def);
void dispatchCommand(Command *cmd);

// Command handlers
void help();
void handleHelp(Command *cmd);
void loadSong();
void handleLoad(Command *cmd);
void playsong(const char *songname);
void handlePlay(Command *cmd);
void listSongs();
void handleListSongs(Command *cmd);
void showLog();
void handleLog(Command *cmd);
void exitProgram();
void handleExit(Command *cmd);
void stopPlay();
void handleStop(Command *cmd);

#endif