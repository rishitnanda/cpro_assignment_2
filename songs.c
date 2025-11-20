#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include "include/songs.h"
#include "include/utils.h"
#include "include/albums.h"

Song *g_songs = NULL;
PlaybackState g_playback;
int g_next_song_id = 1;

volatile sig_atomic_t pause_requested = 0;
volatile sig_atomic_t resume_requested = 0;
volatile sig_atomic_t next_requested = 0;
volatile sig_atomic_t prev_requested = 0;
volatile sig_atomic_t repeat_requested = 0;
volatile sig_atomic_t loop_requested = 0;  

int parse_length(const char *s, SongLength *out) {
    if (!s || !out) return -1;
    int hh = 0, mm = 0, ss = 0;
    char c1 = 0, c2 = 0;
    int items = sscanf(s, "%d%c%d%c%d", &hh, &c1, &mm, &c2, &ss);
    if (items != 5 || c1 != ':' || c2 != ':') return -1;
    if (hh < 0 || mm < 0 || mm > 59 || ss < 0 || ss > 59) return -1;
    out->hh = hh; 
    out->mm = mm; 
    out->ss = ss;
    return 0;
}

void format_length(const SongLength *len, char *buf, size_t buflen) {
    if (!len || !buf || buflen == 0) return;
    snprintf(buf, buflen, "%02d:%02d:%02d", len->hh, len->mm, len->ss);
}

long length_to_seconds(const SongLength *len) {
    if (!len) return 0;
    return (long)len->hh * 3600L + len->mm * 60L + len->ss;
}

int song_init(Song *s, const char *title, const char *artist, const char *length_str, int year) {
    if (!s || !title || !artist || !length_str) return -1;
    s->title = strdup(title);
    s->artist = strdup(artist);
    if (!s->title || !s->artist) {
        free(s->title); 
        free(s->artist);
        return -1;
    }
    if (parse_length(length_str, &s->length) != 0) {
        free(s->title); 
        free(s->artist);
        return -1;
    }
    s->year = year;
    s->song_id = g_next_song_id++;
    s->next = NULL;
    s->prev = NULL;
    return 0;
}

void song_free(Song *s) {
    if (!s) return;
    free(s->title); 
    s->title = NULL;
    free(s->artist); 
    s->artist = NULL;
}

void song_print(const Song *s) {
    if (!s) return;
    char buf[16];
    format_length(&s->length, buf, sizeof(buf));
    printf("  %s - %s (%s) [%d]\n",
           s->title ? s->title : "(untitled)", 
           s->artist ? s->artist : "(unknown)", 
           buf, s->year);
}

Song** find_all_songs_by_title(const char *title, int *count) {
    if (!title || !count) return NULL;
    
    *count = 0;
    for (Song *s = g_songs; s; s = s->next) {
        if (s->title && strcasecmp(s->title, title) == 0) {
            (*count)++;
        }
    }
    
    if (*count == 0) return NULL;
    
    Song **matches = malloc(*count * sizeof(Song*));
    int idx = 0;
    for (Song *s = g_songs; s; s = s->next) {
        if (s->title && strcasecmp(s->title, title) == 0) {
            matches[idx++] = s;
        }
    }
    
    return matches;
}

Song* find_song_by_id(int id) {
    for (Song *s = g_songs; s; s = s->next) {
        if (s->song_id == id) {
            return s;
        }
    }
    return NULL;
}

int is_number(const char *str) {
    if (!str || *str == '\0') return 0;
    
    for (int i = 0; str[i]; i++) {
        if (str[i] < '0' || str[i] > '9') {
            return 0;
        }
    }
    return 1;
}

Song* find_song_by_title_interactive(const char *title) {
    if (!title) return NULL;
    
    if (is_number(title)) {
        int id = atoi(title);
        Song *s = find_song_by_id(id);
        if (s) {
            printf("Selected: %s - %s\n", s->title, s->artist);
            return s;
        } else {
            printf("No song with ID %d\n", id);
            return NULL;
        }
    }
    
    int count;
    Song **matches = find_all_songs_by_title(title, &count);
    
    if (!matches || count == 0) {
        return NULL;
    }
    
    if (count == 1) {
        Song *result = matches[0];
        free(matches);
        return result;
    }
    
    printf("\nMultiple songs found with title '%s':\n", title);
    for (int i = 0; i < count; i++) {
        printf("%d. %s - %s (%02d:%02d:%02d) [%d] [ID: %d]\n",
               i + 1,
               matches[i]->title,
               matches[i]->artist,
               matches[i]->length.hh, matches[i]->length.mm, matches[i]->length.ss,
               matches[i]->year,
               matches[i]->song_id);
    }
    
    printf("Enter number (1-%d): ", count);
    int choice;
    if (scanf("%d", &choice) != 1 || choice < 1 || choice > count) {
        getchar();
        free(matches);
        printf("Invalid choice.\n");
        return NULL;
    }
    getchar();
    
    Song *result = matches[choice - 1];
    free(matches);
    return result;
}

int load_all_songs_from_bin() {
    FILE *fp = fopen("utils/songs.bin", "rb");
    if (!fp) {
        printf("No songs.bin found. Starting with empty library.\n");
        return 0;
    }
    
    int count = 0;
    while (1) {
        int title_len;
        if (fread(&title_len, sizeof(int), 1, fp) != 1) break;
        
        char *title = malloc(title_len + 1);
        if (fread(title, 1, title_len, fp) != title_len) {
            free(title);
            break;
        }
        title[title_len] = '\0';
        
        int artist_len;
        if (fread(&artist_len, sizeof(int), 1, fp) != 1) {
            free(title);
            break;
        }
        
        char *artist = malloc(artist_len + 1);
        if (fread(artist, 1, artist_len, fp) != artist_len) {
            free(title);
            free(artist);
            break;
        }
        artist[artist_len] = '\0';
        
        SongLength len;
        int year, song_id;
        if (fread(&len, sizeof(SongLength), 1, fp) != 1 ||
            fread(&year, sizeof(int), 1, fp) != 1 ||
            fread(&song_id, sizeof(int), 1, fp) != 1) {
            free(title);
            free(artist);
            break;
        }
        
        Song *s = malloc(sizeof(Song));
        s->title = title;
        s->artist = artist;
        s->length = len;
        s->year = year;
        s->song_id = song_id;
        s->next = g_songs;
        s->prev = NULL;
        
        if (g_songs) {
            g_songs->prev = s;
        }
        g_songs = s;
        
        if (song_id >= g_next_song_id) {
            g_next_song_id = song_id + 1;
        }
        
        count++;
    }
    
    fclose(fp);
    printf("Loaded %d songs from library.\n", count);
    return count;
}

int save_all_songs_to_bin() {
    FILE *fp = fopen("utils/songs.bin", "wb");
    if (!fp) {
        perror("Failed to open songs.bin for writing");
        return -1;
    }
    
    int count = 0;
    for (Song *s = g_songs; s; s = s->next) {
        if (!s->title || !s->artist) continue;
        
        int title_len = strlen(s->title);
        int artist_len = strlen(s->artist);
        
        fwrite(&title_len, sizeof(int), 1, fp);
        fwrite(s->title, 1, title_len, fp);
        fwrite(&artist_len, sizeof(int), 1, fp);
        fwrite(s->artist, 1, artist_len, fp);
        fwrite(&s->length, sizeof(SongLength), 1, fp);
        fwrite(&s->year, sizeof(int), 1, fp);
        fwrite(&s->song_id, sizeof(int), 1, fp);
        count++;
    }
    
    fclose(fp);
    return count;
}

int add_song_to_library(Song *s) {
    if (!s) return -1;
    
    s->next = g_songs;
    s->prev = NULL;
    if (g_songs) {
        g_songs->prev = s;
    }
    g_songs = s;
    
    save_all_songs_to_bin();
    return 0;
}

void handle_pause_signal(int sig) {
    pause_requested = 1;
}

void handle_resume_signal(int sig) {
    resume_requested = 1;
}

void handle_next_signal(int sig) {
    next_requested = 1;
}

void handle_prev_signal(int sig) {
    prev_requested = 1;
}

void handle_repeat_signal(int sig) {
    repeat_requested = 1;
}

void handle_loop_signal(int sig) {
    loop_requested = 1;
}

void init_playback_state() {
    memset(&g_playback, 0, sizeof(PlaybackState));
    g_playback.playback_pid = -1;
}

void cleanup_playback_state() {
    if (g_playback.playback_pid > 0) {
        kill(g_playback.playback_pid, SIGKILL);
        waitpid(g_playback.playback_pid, NULL, 0);
    }
    
    if (g_playback.head) {
        PlaylistNode *start = g_playback.head;
        PlaylistNode *curr = g_playback.head;
        do {
            PlaylistNode *next = curr->next;
            free(curr);
            curr = next;
        } while (curr && curr != start);
    }
}

void make_playlist_circular() {
    if (g_playback.head) {
        PlaylistNode *tail = g_playback.head;
        while (tail->next && tail->next != g_playback.head) {
            tail = tail->next;
        }
        tail->next = g_playback.head;
    }
}

int playlist_insert_after_current(Song *songs[], int count) {
    if (!songs || count <= 0) return -1;
    
    PlaylistNode *insert_point = g_playback.current;
    if (!insert_point) {
        insert_point = g_playback.head;
    }
    
    for (int i = 0; i < count; i++) {
        PlaylistNode *node = malloc(sizeof(PlaylistNode));
        if (!node) return -1;
        
        node->song = songs[i];
        
        if (!g_playback.head) {
            g_playback.head = node;
            node->next = node;
            g_playback.current = node;
            insert_point = node;
        } else {
            node->next = insert_point->next;
            insert_point->next = node;
            insert_point = node;
        }
    }
    
    return 0;
}

void display_progress_bar() {
    if (!g_playback.current || !g_playback.current->song) return;
    
    Song *s = g_playback.current->song;
    int elapsed = g_playback.elapsed_seconds;
    int total = g_playback.total_seconds;
    
    printf("\r\033[K");
    
    const char *symbol = g_playback.is_paused ? "▶" : "⏸";
    
    int bar_width = 30;
    int filled = (total > 0) ? (elapsed * bar_width / total) : 0;
    
    printf("%s [", symbol);
    for (int i = 0; i < bar_width; i++) {
        if (i < filled) printf("█");
        else printf("░");
    }
    printf("] %02d:%02d:%02d / %02d:%02d:%02d - %s",
           elapsed / 3600, (elapsed % 3600) / 60, elapsed % 60,
           total / 3600, (total % 3600) / 60, total % 60,
           s->title ? s->title : "(untitled)");
    
    fflush(stdout);
}

void playback_loop() {
    signal(SIGUSR1, handle_pause_signal);
    signal(SIGUSR2, handle_resume_signal);
    signal(SIGCONT, handle_next_signal);
    signal(SIGTERM, handle_prev_signal);
    signal(SIGURG, handle_repeat_signal);
    signal(SIGIO, handle_loop_signal);
    
    while (1) {
        if (pause_requested) {
            g_playback.is_paused = 1;
            pause_requested = 0;
        }
        
        if (resume_requested) {
            g_playback.is_paused = 0;
            resume_requested = 0;
        }
        
        if (repeat_requested) {
            if (g_playback.repeat_mode == 1) {
                g_playback.repeat_mode = 0;
                printf("\n⟲ Repeat mode: OFF\n");
            } else {
                g_playback.repeat_mode = 1;
                printf("\n⟲ Repeat mode: ON (current song will repeat once)\n");
            }
            repeat_requested = 0;
        }
        
        if (loop_requested) {
            g_playback.repeat_mode = 2;
            printf("\n⟳ Loop mode: ON (current song will repeat forever)\n");
            loop_requested = 0;
        }
        
        if (next_requested) {
            if (g_playback.current && g_playback.current->next) {
                g_playback.current = g_playback.current->next;
                g_playback.elapsed_seconds = 0;
                if (g_playback.current->song) {
                    g_playback.total_seconds = length_to_seconds(&g_playback.current->song->length);
                    printf("\n");
                }
            }
            next_requested = 0;
        }
        
        if (prev_requested) {
            if (g_playback.head && g_playback.current) {
                PlaylistNode *prev = g_playback.head;
                while (prev->next != g_playback.current) {
                    prev = prev->next;
                }
                g_playback.current = prev;
                g_playback.elapsed_seconds = 0;
                if (g_playback.current->song) {
                    g_playback.total_seconds = length_to_seconds(&g_playback.current->song->length);
                    printf("\n");
                }
            }
            prev_requested = 0;
        }
        
        if (g_playback.is_playing && !g_playback.is_paused && g_playback.current) {
            if (!g_playback.current->song) {
                if (g_playback.current->next != g_playback.current) {
                    g_playback.current = g_playback.current->next;
                    g_playback.elapsed_seconds = 0;
                    if (g_playback.current->song) {
                        g_playback.total_seconds = length_to_seconds(&g_playback.current->song->length);
                    }
                }
            }
            
            g_playback.elapsed_seconds++;
            
            if (g_playback.elapsed_seconds >= g_playback.total_seconds) {
                if (g_playback.repeat_mode == 1) {
                    g_playback.elapsed_seconds = 0;
                    g_playback.repeat_mode = 0;
                    printf("\n⟲ Repeating song once\n");
                } else if (g_playback.repeat_mode == 2) {
                    g_playback.elapsed_seconds = 0;
                    printf("\n⟳ Looping song\n");
                } else {
                    if (g_playback.current->next && g_playback.current->next->song) {
                        g_playback.current = g_playback.current->next;
                        g_playback.elapsed_seconds = 0;
                        g_playback.total_seconds = length_to_seconds(&g_playback.current->song->length);
                        
                        if (g_playback.current == g_playback.head) {
                            printf("\n🔄 Playlist wrapped to beginning\n");
                        }
                        
                        printf("▶ Now playing: %s\n", g_playback.current->song->title);
                    }
                }
            }
        }
        
        display_progress_bar();
        sleep(1);
    }
}

void start_playback_process() {
    if (g_playback.playback_pid > 0) return;
    
    pid_t pid = fork();
    
    if (pid < 0) {
        perror("fork failed");
        return;
    }
    
    if (pid == 0) {
        playback_loop();
        exit(0);
    } else {
        g_playback.playback_pid = pid;
    }
}

void stop_playback_process() {
    if (g_playback.playback_pid > 0) {
        kill(g_playback.playback_pid, SIGKILL);
        waitpid(g_playback.playback_pid, NULL, 0);
        g_playback.playback_pid = -1;
    }
}

void listPlaylist() {
    printf("\nPLAYLIST\n\n");
    
    if (!g_playback.head) {
        printf("Playlist is empty.\n");
        return;
    }
    
    int idx = 1;
    PlaylistNode *start = g_playback.head;
    PlaylistNode *node = g_playback.head;
    
    do {
        Song *s = node->song;
        if (s) {
            char marker = (node == g_playback.current) ? '>' : ' ';
            printf("%c %d. %s - %s (%02d:%02d:%02d)\n",
                   marker, idx,
                   s->title ? s->title : "(untitled)",
                   s->artist ? s->artist : "(unknown)",
                   s->length.hh, s->length.mm, s->length.ss);
        }
        node = node->next;
        idx++;
    } while (node != start);
}

void handleListPlaylist(Command *cmd) {
    if (cmd->count != 2) {
        printf("Error! Invalid command format.\n");
        return;
    }
    listPlaylist();
}

void nextSongs(const char *songs[], int count) {
    if (!songs || count <= 0) {
        printf("No songs specified.\n");
        return;
    }
    
    typedef struct SongListNode {
        Song *song;
        struct SongListNode *next;
    } SongListNode;
    
    SongListNode *head = NULL;
    SongListNode *tail = NULL;
    int found_count = 0;
    
    printf("Adding songs to playlist:\n");
    for (int i = 0; i < count; i++) {
        Song *s = find_song_by_title_interactive(songs[i]);
        if (!s) {
            printf("  ✗ %s - not found in library\n", songs[i]);
            continue;
        }
        
        SongListNode *node = malloc(sizeof(SongListNode));
        node->song = s;
        node->next = NULL;
        
        if (!head) {
            head = tail = node;
        } else {
            tail->next = node;
            tail = node;
        }
        
        found_count++;
        printf("  ✓ %s\n", s->title);
    }
    
    if (found_count > 0) {
        Song **found_songs = malloc(found_count * sizeof(Song*));
        SongListNode *curr = head;
        int idx = 0;
        while (curr) {
            found_songs[idx++] = curr->song;
            SongListNode *next = curr->next;
            free(curr);
            curr = next;
        }
        
        int was_playing = g_playback.is_playing;
        
        if (g_playback.playback_pid > 0) {
            kill(g_playback.playback_pid, SIGKILL);
            waitpid(g_playback.playback_pid, NULL, 0);
            g_playback.playback_pid = -1;
        }
        
        playlist_insert_after_current(found_songs, found_count);
        make_playlist_circular();
        
        if (was_playing || !g_playback.is_playing) {
            g_playback.is_playing = 1;
            g_playback.is_paused = 0;
            
            if (g_playback.current && g_playback.current->song) {
                g_playback.total_seconds = length_to_seconds(&g_playback.current->song->length);
                g_playback.elapsed_seconds = 0;
            }
            
            start_playback_process();
        }
        
        free(found_songs);
    }
    
    printf("\nAdded %d/%d songs after current position.\n", found_count, count);
}

void handleNextSongs(Command *cmd) {
    if (cmd->count < 3) {
        printf("Error! Invalid command format.\n");
        return;
    }
    
    int song_count = cmd->count - 2;
    const char **songs = malloc(song_count * sizeof(char*));
    
    for (int i = 0; i < song_count; i++) {
        songs[i] = cmd->tokens[i + 2];
    }
    
    nextSongs(songs, song_count);
    free(songs);
}

void nextAlbum(const char *albumname) {
    if (!albumname) {
        printf("No album specified.\n");
        return;
    }
    
    Album *album = find_album_interactive(albumname);
    if (!album) {
        printf("Album '%s' not found.\n", albumname);
        return;
    }
    
    int count = 0;
    for (AlbumNode *n = album->head; n; n = n->next) count++;
    
    if (count == 0) {
        printf("Album '%s' is empty.\n", albumname);
        return;
    }
    
    Song **songs = malloc(count * sizeof(Song*));
    int idx = 0;
    for (AlbumNode *n = album->head; n; n = n->next) {
        songs[idx++] = n->song;
    }
    
    playlist_insert_after_current(songs, count);
    make_playlist_circular();
    
    free(songs);
    
    printf("Added %d songs from album '%s' to playlist.\n", count, albumname);
    
    if (!g_playback.is_playing && g_playback.current) {
        g_playback.is_playing = 1;
        g_playback.total_seconds = length_to_seconds(&g_playback.current->song->length);
        g_playback.elapsed_seconds = 0;
        start_playback_process();
    }
}

void handleNextAlbum(Command *cmd) {
    if (cmd->count != 3) {
        printf("Error! Invalid command format.\n");
        return;
    }
    nextAlbum(cmd->tokens[2]);
}

void pausePlayback() {
    if (!g_playback.is_playing) {
        printf("\nNo song is currently playing.\n");
        return;
    }
    
    if (g_playback.is_paused) {
        printf("\nPlayback is already paused.\n");
        return;
    }
    
    if (g_playback.playback_pid > 0) {
        kill(g_playback.playback_pid, SIGUSR1);
    }
    
    g_playback.is_paused = 1;
    printf("\nPaused.\n");
}

void handlePause(Command *cmd) {
    if (cmd->count != 1) {
        printf("Error! Invalid command format.\n");
        return;
    }
    pausePlayback();
}

void resumePlayback() {
    if (!g_playback.is_playing) {
        printf("\nNo song to resume.\n");
        return;
    }
    
    if (!g_playback.is_paused) {
        printf("\nPlayback is not paused.\n");
        return;
    }
    
    if (g_playback.playback_pid > 0) {
        kill(g_playback.playback_pid, SIGUSR2);
    }
    
    g_playback.is_paused = 0;
    printf("\nResumed.\n");
}

void handleResume(Command *cmd) {
    if (cmd->count != 1) {
        printf("Error! Invalid command format.\n");
        return;
    }
    resumePlayback();
}

void fwd() {
    if (!g_playback.head) {
        printf("\nPlaylist is empty.\n");
        return;
    }
    
    if (g_playback.playback_pid > 0) {
        kill(g_playback.playback_pid, SIGCONT);
    }
    
    printf("\nSkipped to next song.\n");
}

void handleFwd(Command *cmd) {
    if (cmd->count != 1) {
        printf("Error! Invalid command format.\n");
        return;
    }
    fwd();
}

void prev() {
    if (!g_playback.head) {
        printf("\nPlaylist is empty.\n");
        return;
    }
    
    if (g_playback.playback_pid > 0) {
        kill(g_playback.playback_pid, SIGTERM);
    }
    
    printf("\nWent back to previous song.\n");
}

void handlePrev(Command *cmd) {
    if (cmd->count != 1) {
        printf("Error! Invalid command format.\n");
        return;
    }
    prev();
}

void repeat() {
    if (!g_playback.current || !g_playback.current->song) {
        printf("\nNo song is currently playing.\n");
        return;
    }
    
    if (g_playback.playback_pid > 0) {
        kill(g_playback.playback_pid, SIGURG);
    }
}

void handleRepeat(Command *cmd) {
    if (cmd->count != 1) {
        printf("Error! Invalid command format.\n");
        return;
    }
    repeat();
}

void shuffle() {
    if (!g_playback.head) {
        printf("\nPlaylist is empty.\n");
        return;
    }
    
    int count = 0;
    PlaylistNode *node = g_playback.head;
    do {
        count++;
        node = node->next;
    } while (node != g_playback.head);
    
    if (count < 2) {
        printf("\nNeed at least 2 songs to shuffle.\n");
        return;
    }
    
    Song **songs = malloc(count * sizeof(Song*));
    node = g_playback.head;
    for (int i = 0; i < count; i++) {
        songs[i] = node->song;
        node = node->next;
    }
    
    srand(time(NULL));
    for (int i = count - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        Song *temp = songs[i];
        songs[i] = songs[j];
        songs[j] = temp;
    }
    
    node = g_playback.head;
    for (int i = 0; i < count; i++) {
        node->song = songs[i];
        node = node->next;
    }
    
    free(songs);
    printf("\nPlaylist shuffled (%d songs).\n", count);
}

void handleShuffle(Command *cmd) {
    if (cmd->count != 1) {
        printf("Error! Invalid command format.\n");
        return;
    }
    shuffle();
}

void removeSong(const char *songname) {
    if (!songname) {
        printf("\nNo song specified.\n");
        return;
    }
    
    if (!g_playback.head) {
        printf("\nPlaylist is empty.\n");
        return;
    }
    
    if (g_playback.playback_pid > 0) {
        kill(g_playback.playback_pid, SIGKILL);
        waitpid(g_playback.playback_pid, NULL, 0);
        g_playback.playback_pid = -1;
    }
    
    PlaylistNode *prev = NULL;
    PlaylistNode *curr = g_playback.head;
    PlaylistNode *start = g_playback.head;
    int found = 0;
    
    do {
        if (curr->song && curr->song->title && 
            strcasecmp(curr->song->title, songname) == 0) {
            found = 1;
            break;
        }
        prev = curr;
        curr = curr->next;
    } while (curr != start);
    
    if (!found) {
        printf("\nSong '%s' not found in playlist.\n", songname);
        
        if (g_playback.is_playing) {
            start_playback_process();
        }
        return;
    }
    
    int was_playing = g_playback.is_playing;
    
    if (curr == g_playback.head && curr->next == g_playback.head) {
        free(curr);
        g_playback.head = NULL;
        g_playback.current = NULL;
        g_playback.is_playing = 0;
    } else {
        if (curr == g_playback.current) {
            g_playback.current = curr->next;
            g_playback.elapsed_seconds = 0;
            if (g_playback.current->song) {
                g_playback.total_seconds = length_to_seconds(&g_playback.current->song->length);
            }
        }
        
        if (curr == g_playback.head) {
            g_playback.head = curr->next;
        }
        
        prev->next = curr->next;
        free(curr);
    }
    
    printf("\nRemoved '%s' from playlist.\n", songname);
    
    if (was_playing && g_playback.head) {
        g_playback.is_playing = 1;
        start_playback_process();
    }
}

void handleRemove(Command *cmd) {
    if (cmd->count != 2) {
        printf("Error! Invalid command format.\n");
        return;
    }
    removeSong(cmd->tokens[1]);
}

void loop() {
    if (!g_playback.current || !g_playback.current->song) {
        printf("\nNo song is currently playing.\n");
        return;
    }
    
    if (g_playback.playback_pid > 0) {
        kill(g_playback.playback_pid, SIGIO);
    }
}

void handleLoop(Command *cmd) {
    if (cmd->count != 1) {
        printf("Error! Invalid command format.\n");
        return;
    }
    loop();
}