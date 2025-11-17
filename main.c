#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "include/utils.h"
#include "include/songs.h"
#include "include/albums.h"

void logCommandToFile(const char *command) {
    FILE *logFile = fopen("utils/command_log.txt", "a");
    if (logFile != NULL) {
        fprintf(logFile, "%s\n", command);
        fclose(logFile);
    }
}

int main() {
    char line[MAX_LINE];
    
    printf("╔════════════════════════════════════════╗\n");
    printf("║   MUSIC PLAYLIST MANAGER               ║\n");
    printf("╚════════════════════════════════════════╝\n\n");
    
    // Initialize playback state
    init_playback_state();
    
    // Load songs from binary into doubly linked list
    printf("Loading song library...\n");
    load_all_songs_from_bin();
    
    // Load all albums into memory
    printf("Loading albums...\n");
    load_all_albums();
    
    printf("\nType 'HELP' for available commands.\n");
    printf("TIP: Use song/album numbers OR names in commands!\n\n");
    
    while (1) {
        printf("> ");
        fflush(stdout);
        
        if (fgets(line, sizeof(line), stdin) == NULL) {
            break;
        }
        
        // Remove newline
        line[strcspn(line, "\n")] = 0;
        
        // Skip empty lines
        if (strlen(line) == 0) {
            continue;
        }
        
        // Log command
        logCommandToFile(line);
        
        // Parse command
        Command cmd = parseCommand(line);
        
        // Check for exit
        if (cmd.count > 0 && strcmp(cmd.tokens[0], "EXIT") == 0) {
            freeCommand(&cmd);
            break;
        }
        
        // Execute command
        dispatchCommand(&cmd);
        freeCommand(&cmd);
    }
    
    // Cleanup and save
    cleanup_playback_state();
    save_all_songs_to_bin();
    
    // Save all albums
    for (Album *a = g_albums; a; a = a->next) {
        save_album_to_bin(a);
    }
    
    printf("\nGoodbye!\n");
    return 0;
}