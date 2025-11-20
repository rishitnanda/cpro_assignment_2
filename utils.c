#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "include/utils.h"
#include "include/songs.h"
#include "include/albums.h"
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>

Command parseCommand(char *line) {
    Command cmd;
    cmd.tokens = (char**)malloc(MAX_TOKENS * sizeof(char*));
    cmd.count = 0;
    
    char *ptr = line;
    
    while (*ptr && cmd.count < MAX_TOKENS) {
        while (*ptr == ' ' || *ptr == '\t') ptr++;
        
        if (*ptr == '\0') break;
        
        if (*ptr == '"') {
            ptr++;
            char *start = ptr;
            
            while (*ptr && *ptr != '"') ptr++;
            
            int len = ptr - start;
            cmd.tokens[cmd.count] = (char*)malloc(len + 1);
            strncpy(cmd.tokens[cmd.count], start, len);
            cmd.tokens[cmd.count][len] = '\0';
            cmd.count++;
            
            if (*ptr == '"') ptr++;
        } else {
            char *start = ptr;
            while (*ptr && *ptr != ' ' && *ptr != '\t' && *ptr != '\n') ptr++;
            
            int len = ptr - start;
            cmd.tokens[cmd.count] = (char*)malloc(len + 1);
            strncpy(cmd.tokens[cmd.count], start, len);
            cmd.tokens[cmd.count][len] = '\0';
            cmd.count++;
        }
    }
    
    return cmd;
}

void freeCommand(Command *cmd) {
    for (int i = 0; i < cmd->count; i++) {
        free(cmd->tokens[i]);
    }
    free(cmd->tokens);
} 

CommandDef commands[] = {
    {"HELP", "HELP", 1, 1, 1, handleHelp},
    {"LOAD", "LOAD", 1, 1, 1, handleLoad},
    {"LIST", "LIST SONGS", 2, 2, 2, handleListSongs},
    {"LIST", "LIST ALBUMS", 2, 2, 2, handleListAlbums},
    {"LIST", "LIST IN ALBUM", 3, 4, 4, handleListSongsInAlbum},
    {"LIST", "LIST PLAYLIST", 2, 2, 2, handleListPlaylist},
    {"CREATE", "CREATE", 1, 2, -1, handleCreateAlbum},
    {"MANAGE", "MANAGE ADD", 2, 4, 4, handleManageAddSong},
    {"MANAGE", "MANAGE SWAP", 2, 5, 5, handleManageSwapSongs},
    {"MANAGE", "MANAGE MOVE", 2, 5, 5, handleManageMoveSong},
    {"MANAGE", "MANAGE DELETE", 2, 4, 4, handleManageDeleteSong},
    {"DELETE", "DELETE ALBUM", 2, 3, 3, handleDeleteAlbum},
    {"NEXT", "NEXT SONG",  2, 3, -1, handleNextSongs},
    {"NEXT", "NEXT ALBUM", 2, 3, 3, handleNextAlbum},
    {"PAUSE", "PAUSE", 1, 1, 1, handlePause},
    {"RESUME", "RESUME", 1, 1, 1, handleResume},
    {"FWD", "FWD", 1, 1, 1, handleFwd},
    {"PREV", "PREV", 1, 1, 1, handlePrev},
    {"REPEAT", "REPEAT", 1, 1, 1, handleRepeat},
    {"SHUFFLE", "SHUFFLE", 1, 1, 1, handleShuffle},
    {"REMOVE", "REMOVE", 1, 2, 2, handleRemove},
    {"LOOP", "LOOP", 1, 1, 1, handleLoop},
    {"LOG", "LOG", 1, 1, 1, handleLog},
    {"EXIT", "EXIT", 1, 1, 1, handleExit},
    {NULL, NULL, 0, 0, 0, NULL}
};

int matchCommand(Command *cmd, CommandDef *def) {
    if (cmd->count == 0) return 0;
    
    if (strcmp(cmd->tokens[0], def->name) != 0) {
        return 0;
    }
    
    if (def->command_words > 1) {
        if (cmd->count < def->command_words) return 0;
        
        char full_copy[256];
        strncpy(full_copy, def->full_name, 255);
        full_copy[255] = '\0';
        
        char *word = strtok(full_copy, " ");
        for (int i = 0; i < def->command_words && word != NULL; i++) {
            if (strcmp(cmd->tokens[i], word) != 0) {
                return 0;
            }
            word = strtok(NULL, " ");
        }
    }
    
    if (cmd->count < def->min_args) {
        printf("\nError: Command requires at least %d tokens\n", def->min_args);
        return 0;
    }
    
    if (def->max_args != -1 && cmd->count > def->max_args) {
        printf("\nError: Command accepts at most %d tokens\n", def->max_args);
        return 0;
    }
    
    return 1;
}

void dispatchCommand(Command *cmd) {
    if (cmd->count == 0) return;
    
    for (int i = 0; commands[i].name != NULL; i++) {
        if (matchCommand(cmd, &commands[i])) {
            commands[i].handler(cmd);
            return;
        }
    }
    
    printf("\n✗ Unknown command: %s\n", cmd->tokens[0]);
    printf("  Type 'HELP' for available commands.\n");
}

void help() {
    printf("\nAVAILABLE COMMANDS\n\n");
    printf("1. HELP - Display this help message\n");
    printf("2. LOAD - Add a new song into library\n");
    printf("3. LIST SONGS - List all songs in library\n");
    printf("4. LIST ALBUMS - List all albums\n");
    printf("5. LIST IN ALBUM <albumname> - List all songs in an album\n");
    printf("6. LIST PLAYLIST - List all songs in current playlist\n");
    printf("7. CREATE <albumname> <song1> <song2>... - Create a new album\n");
    printf("   (songs can be song IDs or names)\n");
    printf("8. MANAGE ADD <albumname> <song> - Add a song to an album\n");
    printf("   (album/song can be ID or name)\n");
    printf("9. MANAGE SWAP <albumname> <song1> <song2> - Swap two songs\n");
    printf("10. MANAGE MOVE <albumname> <song> <position> - Move a song\n");
    printf("11. MANAGE DELETE <albumname> <song> - Delete a song from album\n");
    printf("12. DELETE ALBUM <albumname> - Delete an album\n");
    printf("   (album can be ID or name)\n");
    printf("13. NEXT SONG <song1> <song2>... - Add songs after current\n");
    printf("   (songs can be song IDs or names)\n");
    printf("14. NEXT ALBUM <albumname> - Add album after current\n");
    printf("   (album can be ID or name)\n");
    printf("15. PAUSE - Pause playback\n");
    printf("16. RESUME - Resume playback\n");
    printf("17. FWD - Skip to next song\n");
    printf("18. PREV - Go to previous song\n");
    printf("19. REPEAT - Toggle repeat mode\n");
    printf("20. SHUFFLE - Shuffle playlist\n");
    printf("21. REMOVE <songname> - Remove song from playlist\n");
    printf("22. LOOP - Loop current song indefinitely\n");
    printf("23. LOG - Display command history\n");
    printf("24. EXIT - Exit the program\n\n");
}

void handleHelp(Command *cmd) {
    if (cmd->count != 1) {
        printf("Error! Invalid command format.\n");
        return;
    }
    help();
}

void loadSong() {
    char title[256], artist[256], length_str[16];
    int year;
    
    printf("\nADD NEW SONG\n\n");

    printf("Enter title: ");
    fgets(title, sizeof(title), stdin);
    title[strcspn(title, "\n")] = 0;
    
    printf("Enter artist: ");
    fgets(artist, sizeof(artist), stdin);
    artist[strcspn(artist, "\n")] = 0;
    
    printf("Enter length (hh:mm:ss): ");
    fgets(length_str, sizeof(length_str), stdin);
    length_str[strcspn(length_str, "\n")] = 0;
    
    printf("Enter year: ");
    scanf("%d", &year);
    getchar();
    
    Song *s = malloc(sizeof(Song));
    if (song_init(s, title, artist, length_str, year) == 0) {
        if (add_song_to_library(s) == 0) {
            printf("\n✓ Added: %s - %s [ID: %d]\n", title, artist, s->song_id);
        } else {
            song_free(s);
            free(s);
            printf("\n✗ Failed to add song to library\n");
        }
    } else {
        free(s);
        printf("\n✗ Invalid song format\n");
    }
}

void handleLoad(Command *cmd) {
    if (cmd->count != 1) {
        printf("Error! Invalid command format.\n");
        printf("Usage: LOAD\n");
        return;
    }
    loadSong();
}

pid_t play_process_pid = -1;

void playsong(const char *songname) {
    if (!songname) {
        printf("No song specified.\n");
        return;
    }
    
    Song *s = find_song_by_title_interactive(songname);
    if (!s) {
        printf("Song '%s' not found in library.\n", songname);
        return;
    }
    
    if (play_process_pid > 0) {
        kill(play_process_pid, SIGKILL);
        waitpid(play_process_pid, NULL, 0);
        play_process_pid = -1;
    }
    
    int was_playing = g_playback.is_playing;
    if (g_playback.playback_pid > 0) {
        kill(g_playback.playback_pid, SIGSTOP);
    }
    
    g_playback.is_playing = 0;
    g_playback.is_paused = 1;
    
    printf("\nPLAYING SINGLE SONG\n\n");
    printf("Title:  %s\n", s->title);
    printf("Artist: %s\n", s->artist);
    printf("Length: %02d:%02d:%02d\n", s->length.hh, s->length.mm, s->length.ss);
    printf("Year:   %d\n\n", s->year);
    
    pid_t pid = fork();
    
    if (pid < 0) {
        perror("fork failed");
        return;
    }
    
    if (pid == 0) {
        int total_seconds = length_to_seconds(&s->length);
        for (int i = 0; i <= total_seconds; i++) {
            printf("\r\033[K");
            printf("⏸ [");
            int bar_width = 30;
            int filled = (total_seconds > 0) ? (i * bar_width / total_seconds) : 0;
            for (int j = 0; j < bar_width; j++) {
                if (j < filled) printf("█");
                else printf("░");
            }
            printf("] %02d:%02d:%02d / %02d:%02d:%02d",
                   i / 3600, (i % 3600) / 60, i % 60,
                   total_seconds / 3600, (total_seconds % 3600) / 60, total_seconds % 60);
            fflush(stdout);
            sleep(1);
        }
        
        printf("\n\nSong finished.\n");
        exit(0);
    } else {
        play_process_pid = pid;
        
        int status;
        waitpid(pid, &status, 0);
        play_process_pid = -1;
        
        if (was_playing) {
            g_playback.is_playing = 1;
            g_playback.is_paused = 0;
            if (g_playback.playback_pid > 0) {
                kill(g_playback.playback_pid, SIGCONT);
            }
            printf("Resuming playlist...\n");
        }
    }
}

void handlePlay(Command *cmd) {
    if (cmd->count != 2) {
        printf("Error! Invalid command format.\n");
        printf("Usage: PLAY <songname>\n");
        return;
    }
    playsong(cmd->tokens[1]);
}

void listSongs() {
    printf("\nSONG LIBRARY\n\n");
    
    if (!g_songs) {
        printf("Library is empty.\n");
        printf("Use 'LOAD' to add songs.\n");
        return;
    }
    
    int count = 0;
    for (Song *s = g_songs; s; s = s->next) {
        count++;
        printf("%d. %s - %s (%02d:%02d:%02d) [%d] [ID: %d]\n",
               count,
               s->title ? s->title : "(untitled)",
               s->artist ? s->artist : "(unknown)",
               s->length.hh, s->length.mm, s->length.ss,
               s->year,
               s->song_id);
    }
    
    printf("\nTotal songs: %d\n", count);
}

void handleListSongs(Command *cmd) {
    if (cmd->count != 2) {
        printf("Error! Invalid command format.\n");
        return;
    }
    listSongs();
}

void showLog() {
    FILE *logFile = fopen("utils/command_log.txt", "r");
    if (logFile != NULL) {
        char line[MAX_LINE];
        
        printf("\nCOMMAND LOG\n\n");
        
        int count = 0;
        while (fgets(line, sizeof(line), logFile)) {
            count++;
            printf("%d. %s", count, line);
        }
        fclose(logFile);
        
        if (count == 0) {
            printf("No commands have been logged yet.\n");
        }
    } else {
        printf("No commands have been logged yet.\n");
    }
}

void handleLog(Command *cmd) {
    if (cmd->count != 1) {
        printf("Error! Invalid command format.\n");
        return;
    }
    showLog();
}

void exitProgram() {
    printf("\nSaving and exiting...\n\n");
    
    save_all_songs_to_bin();
    
    for (Album *a = g_albums; a; a = a->next) {
        save_album_to_bin(a);
    }
    
    cleanup_playback_state();
    
    printf("Goodbye!\n");
    exit(0);
}

void handleExit(Command *cmd) {
    if (cmd->count != 1) {
        printf("Error! Invalid command format.\n");
        return;
    }
    exitProgram();
}

void stopPlay() {
    if (play_process_pid > 0) {
        kill(play_process_pid, SIGKILL);
        waitpid(play_process_pid, NULL, 0);
        play_process_pid = -1;
        printf("\nStopped playback.\n");
        
        if (g_playback.is_playing) {
            if (g_playback.playback_pid > 0) {
                kill(g_playback.playback_pid, SIGCONT);
            }
            printf("Resuming playlist...\n");
        }
    } else {
        printf("No song is currently playing.\n");
    }
}

void handleStop(Command *cmd) {
    if (cmd->count != 1) {
        printf("Error! Invalid command format.\n");
        return;
    }
    stopPlay();
}