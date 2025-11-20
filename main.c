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
    
    printf("C-Unplugged\n\n");
    
    init_playback_state();
    
    printf("Loading song library...\n");
    load_all_songs_from_bin();
    
    printf("Loading albums...\n");
    load_all_albums();
    
    printf("\nType 'HELP' or '1' for available commands.\n");
    printf("TIP: Use song/album IDs OR names in commands!\n\n");
    
    while (1) {
        printf("> ");
        fflush(stdout);
        
        if (fgets(line, sizeof(line), stdin) == NULL) {
            break;
        }
        
        line[strcspn(line, "\n")] = 0;
        
        if (strlen(line) == 0) {
            continue;
        }
        
        logCommandToFile(line);
        
        Command cmd = parseCommand(line);
        
        if (cmd.count > 0 && strcmp(cmd.tokens[0], "EXIT") == 0) {
            freeCommand(&cmd);
            break;
        }
        
        dispatchCommand(&cmd);
        freeCommand(&cmd);
    }
    
    cleanup_playback_state();
    save_all_songs_to_bin();
    
    for (Album *a = g_albums; a; a = a->next) {
        save_album_to_bin(a);
    }
    
    printf("\nGoodbye!\n");
    return 0;
}