#ifndef STRUCTURES_H
#define STRUCTURES_H

#include <sys/types.h>

typedef struct SongLength {
    int hh; 
    int mm;
    int ss;
} SongLength;

typedef struct Song {
    char *title;
    char *artist;
    SongLength length;
    int year;
    int song_id;
    struct Song *next;
    struct Song *prev;
} Song;

typedef struct PlaylistNode {
    Song *song;
    struct PlaylistNode *next;
} PlaylistNode;

typedef struct PlaybackState {
    PlaylistNode *head;
    PlaylistNode *current;
    int is_playing;
    int is_paused;
    int elapsed_seconds;
    int total_seconds;
    int repeat_mode;
    pid_t playback_pid;
} PlaybackState;

typedef struct Command {
    char **tokens;
    int count;
} Command;

typedef void (*CommandHandler)(Command *cmd);

typedef struct CommandDef {
    const char *name;
    const char *full_name;
    int command_words;
    int min_args;
    int max_args;
    void (*handler)(Command*);
} CommandDef;

typedef struct AlbumNode {
    Song *song;
    struct AlbumNode *next;
} AlbumNode;

typedef struct Album {
    char *name;
    int album_id;
    AlbumNode *head;
    struct Album *next;
    struct Album *prev;
} Album;

#endif