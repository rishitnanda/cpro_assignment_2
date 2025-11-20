#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <strings.h>
#include "include/albums.h"
#include "include/songs.h"
#include "include/utils.h"

Album *g_albums = NULL;
int g_next_album_id = 1;

int iequals(const char *a, const char *b) {
    if (!a || !b) return 0;
    return strcasecmp(a, b) == 0;
}

static Song* find_song_in_library_by_index(int index) {
    if (index < 1) return NULL;
    int idx = 1;
    for (Song *s = g_songs; s; s = s->next) {
        if (idx == index) return s;
        idx++;
    }
    return NULL;
}

static AlbumNode* album_node_at_index(Album *a, int index, AlbumNode **prev_out) {
    if (!a || index < 1) return NULL;
    AlbumNode *prev = NULL;
    AlbumNode *cur = a->head;
    int idx = 1;
    while (cur && idx < index) {
        prev = cur;
        cur = cur->next;
        idx++;
    }
    if (prev_out) *prev_out = prev;
    return cur;
}

Album** find_all_albums_by_name(const char *name, int *count) {
    if (!name || !count) return NULL;
    
    *count = 0;
    for (Album *a = g_albums; a; a = a->next) {
        if (iequals(a->name, name)) {
            (*count)++;
        }
    }
    
    if (*count == 0) return NULL;
    
    Album **matches = malloc(*count * sizeof(Album*));
    int idx = 0;
    for (Album *a = g_albums; a; a = a->next) {
        if (iequals(a->name, name)) {
            matches[idx++] = a;
        }
    }
    
    return matches;
}

Album* find_album_by_id(int id) {
    for (Album *a = g_albums; a; a = a->next) {
        if (a->album_id == id) {
            return a;
        }
    }
    return NULL;
}

int is_number_album(const char *str) {
    if (!str || *str == '\0') return 0;
    for (int i = 0; str[i]; i++) {
        if (str[i] < '0' || str[i] > '9') return 0;
    }
    return 1;
}

Album* find_album_interactive(const char *input) {
    if (!input) return NULL;

    if (is_number_album(input)) {
        int choice = atoi(input);
        int index = 1;

        for (Album *a = g_albums; a; a = a->next, index++) {
            if (index == choice) {
                printf("Selected album: %s\n", a->name);
                return a;
            }
        }

        printf("No album at position %d\n", choice);
        return NULL;
    }

    int count;
    Album **matches = find_all_albums_by_name(input, &count);

    if (!matches || count == 0) {
        return NULL;
    }

    if (count == 1) {
        Album *result = matches[0];
        free(matches);
        return result;
    }

    printf("\nMultiple albums found with name '%s':\n", input);
    for (int i = 0; i < count; i++) {
        int song_count = 0;
        for (AlbumNode *n = matches[i]->head; n; n = n->next) song_count++;
        printf("%d. %s (%d songs)\n", i + 1, matches[i]->name, song_count);
    }

    printf("Enter number (1-%d): ", count);
    int sel;
    if (scanf("%d", &sel) != 1 || sel < 1 || sel > count) {
        getchar();
        printf("Invalid choice.\n");
        free(matches);
        return NULL;
    }
    getchar();

    Album *result = matches[sel - 1];
    free(matches);
    return result;
}

Album* create_album_internal(const char *name) {
    if (!name) return NULL;
    Album *a = (Album*)malloc(sizeof(Album));
    if (!a) return NULL;
    
    a->name = strdup(name);
    if (!a->name) {
        free(a);
        return NULL;
    }
    
    a->album_id = g_next_album_id++;
    a->head = NULL;
    a->next = g_albums;
    a->prev = NULL;
    
    if (g_albums) g_albums->prev = a;
    g_albums = a;
    
    return a;
}

AlbumNode* album_find_node(Album *a, const char *title, AlbumNode **prev_out) {
    if (!a || !title) return NULL;
    AlbumNode *prev = NULL;
    for (AlbumNode *n = a->head; n; prev = n, n = n->next) {
        if (n->song && n->song->title && iequals(n->song->title, title)) {
            if (prev_out) *prev_out = prev;
            return n;
        }
    }
    return NULL;
}

int album_append_song(Album *a, Song *s) {
    if (!a || !s) return -1;
    AlbumNode *node = (AlbumNode*)malloc(sizeof(AlbumNode));
    if (!node) return -1;
    node->song = s;
    node->next = NULL;
    if (!a->head) a->head = node;
    else {
        AlbumNode *tail = a->head;
        while (tail->next) tail = tail->next;
        tail->next = node;
    }
    return 0;
}

int load_album_from_bin_by_id(int album_id, Album **out) {
    if (!out) return -1;
    for (Album *a = g_albums; a; a = a->next) {
        if (a->album_id == album_id) {
            *out = a;
            return 0;
        }
    }
    return -1;
}

int save_album_to_bin(const Album *a) {
    if (!a || !a->name) return -1;
    char filepath[512];
    snprintf(filepath, sizeof(filepath), "utils/albums/%s_%d.bin", a->name, a->album_id);
    FILE *fp = fopen(filepath, "wb");
    if (!fp) {
        perror("Failed to create album file");
        return -1;
    }
    fwrite(&a->album_id, sizeof(int), 1, fp);
    int song_count = 0;
    for (AlbumNode *n = a->head; n; n = n->next) song_count++;
    fwrite(&song_count, sizeof(int), 1, fp);
    for (AlbumNode *n = a->head; n; n = n->next) {
        if (n->song) {
            fwrite(&n->song->song_id, sizeof(int), 1, fp);
        }
    }
    fclose(fp);
    return 0;
}

void load_all_albums() {
    DIR *d = opendir("utils/albums/");
    if (!d) {
        printf("No albums directory found.\n");
        return;
    }
    
    struct dirent *entry;
    int count = 0;
    
    while ((entry = readdir(d)) != NULL) {
        const char *name = entry->d_name;
        const char *ext = strrchr(name, '.');
        if (!ext || strcmp(ext, ".bin") != 0) continue;
        
        char filepath[512];
        snprintf(filepath, sizeof(filepath), "utils/albums/%s", name);
        
        FILE *fp = fopen(filepath, "rb");
        if (!fp) continue;
        
        int album_id;
        if (fread(&album_id, sizeof(int), 1, fp) != 1) {
            fclose(fp);
            continue;
        }
        
        char album_name[256];
        const char *underscore = strrchr(name, '_');
        if (underscore) {
            int len = underscore - name;
            strncpy(album_name, name, len);
            album_name[len] = '\0';
        } else {
            strncpy(album_name, name, ext - name);
            album_name[ext - name] = '\0';
        }
        
        Album *album = create_album_internal(album_name);
        if (!album) {
            fclose(fp);
            continue;
        }
        
        album->album_id = album_id;
        
        if (album_id >= g_next_album_id) g_next_album_id = album_id + 1;
        
        int song_count;
        if (fread(&song_count, sizeof(int), 1, fp) != 1) {
            fclose(fp);
            continue;
        }
        
        for (int i = 0; i < song_count; i++) {
            int song_id;
            if (fread(&song_id, sizeof(int), 1, fp) != 1) break;
            
            Song *song = NULL;
            for (Song *s = g_songs; s; s = s->next) {
                if (s->song_id == song_id) {
                    song = s;
                    break;
                }
            }
            
            if (song) album_append_song(album, song);
        }
        
        fclose(fp);
        count++;
    }
    
    closedir(d);
    printf("Loaded %d albums.\n", count);
}

void listAlbums() {
    printf("\nALBUMS\n\n");
    if (!g_albums) {
        printf("No albums found.\n");
        return;
    }
    int index = 1;
    for (Album *a = g_albums; a; a = a->next, index++) {
        printf("%d. %s\n", index, a->name);
    }
}

void handleListAlbums(Command *cmd) {
    if (cmd->count != 2) {
        printf("Error! Invalid command format.\n");
        return;
    }
    listAlbums();
}

void listSongsInAlbum(const char *albumname) {
    if (!albumname) {
        fprintf(stderr, "listSongsInAlbum: albumname is NULL\n");
        return;
    }
    
    Album *a = find_album_interactive(albumname);
    if (!a) {
        printf("Album \"%s\" not found\n", albumname);
        return;
    }
    
    if (!a->head) {
        printf("Album \"%s\" is empty\n", albumname);
        return;
    }
    
    printf("\nAlbum: %s\n\n", a->name);
    
    int idx = 1;
    for (AlbumNode *n = a->head; n; n = n->next, ++idx) {
        Song *s = n->song;
        if (s) {
            printf("%d. %s - %s (%02d:%02d:%02d)\n",
                   idx,
                   s->title ? s->title : "(untitled)",
                   s->artist ? s->artist : "(unknown)",
                   s->length.hh, s->length.mm, s->length.ss);
        } else {
            printf("%d. (missing song)\n", idx);
        }
    }
}

void handleListSongsInAlbum(Command *cmd) {
    if (cmd->count != 4) {
        printf("Error! Invalid command format.\n");
        return;
    }
    listSongsInAlbum(cmd->tokens[3]);
}

static Song* resolve_library_song_token(const char *token) {
    if (!token) return NULL;
    if (is_number(token)) {
        int idx = atoi(token);
        return find_song_in_library_by_index(idx);
    }
    return find_song_by_title_interactive(token);
}

void createAlbum(const char *albumname, const char *songs[], int count) {
    if (!albumname) {
        fprintf(stderr, "createAlbum: albumname is NULL\n");
        return;
    }

    Album *a = create_album_internal(albumname);
    if (!a) {
        fprintf(stderr, "createAlbum: failed to create album \"%s\"\n", albumname);
        return;
    }
    printf("Created album \"%s\"\n", albumname);

    if (!songs || count <= 0) {
        save_album_to_bin(a);
        return;
    }

    for (int i = 0; i < count; ++i) {
        const char *token = songs[i];
        if (!token) continue;

        Song *s = resolve_library_song_token(token);
        if (!s) {
            printf("Skipping \"%s\": not found in library\n", token);
            continue;
        }

        if (album_find_node(a, s->title, NULL)) {
            printf("Skipping \"%s\": already in album \"%s\"\n", s->title, albumname);
            continue;
        }

        if (album_append_song(a, s) != 0) {
            fprintf(stderr, "createAlbum: failed to add \"%s\" to album \"%s\" (OOM)\n",
                    s->title, albumname);
            return;
        }

        printf("Added \"%s\" to album \"%s\"\n", s->title, albumname);
    }
    
    save_album_to_bin(a);
}

void handleCreateAlbum(Command *cmd) {
    if (cmd->count < 3) {
        printf("Error! Invalid command format.\n");
        return;
    }

    const char *albumname = cmd->tokens[1];
    int song_count = cmd->count - 2;
    const char **songs = malloc(song_count * sizeof(char*));

    for (int i = 0; i < song_count; i++) songs[i] = cmd->tokens[i + 2];

    createAlbum(albumname, songs, song_count);
    free(songs);
}

static AlbumNode* resolve_album_song_token(Album *a, const char *token, AlbumNode **prev_out) {
    if (!a || !token) return NULL;
    if (is_number(token)) {
        int pos = atoi(token);
        return album_node_at_index(a, pos, prev_out);
    }
    return album_find_node(a, token, prev_out);
}

void manageAddSong(const char *albumname, const char *songname) {
    if (!albumname || !songname) {
        fprintf(stderr, "manageAddSong: NULL argument\n");
        return;
    }

    Song *s = resolve_library_song_token(songname);
    if (!s) {
        printf("Song \"%s\" not found in library. Use LOAD to add it first.\n", songname);
        return;
    }

    Album *a = find_album_interactive(albumname);
    
    if (!a) {
        a = create_album_internal(albumname);
        if (!a) {
            fprintf(stderr, "manageAddSong: failed to create album\n");
            return;
        }
        printf("Created album \"%s\"\n", albumname);
    }

    if (album_find_node(a, s->title, NULL)) {
        printf("Song \"%s\" already in album \"%s\"\n", s->title, albumname);
        return;
    }

    if (album_append_song(a, s) != 0) {
        fprintf(stderr, "manageAddSong: memory allocation failed\n");
        return;
    }
    
    save_album_to_bin(a);
    printf("Added \"%s\" to album \"%s\"\n", s->title, albumname);
}

void handleManageAddSong(Command *cmd) {
    if (cmd->count != 4) {
        printf("Error! Invalid command format.\n");
        return;
    }
    manageAddSong(cmd->tokens[2], cmd->tokens[3]);
}

void manageSwapSongs(const char *albumname, const char *song1, const char *song2) {
    if (!albumname || !song1 || !song2) {
        fprintf(stderr, "manageSwapSongs: NULL argument\n");
        return;
    }
    if (iequals(song1, song2)) {
        printf("Songs are identical; nothing to swap\n");
        return;
    }

    Album *a = find_album_interactive(albumname);
    if (!a) {
        printf("Album \"%s\" not found\n", albumname);
        return;
    }

    AlbumNode *prev1 = NULL, *prev2 = NULL;
    AlbumNode *n1 = resolve_album_song_token(a, song1, &prev1);
    AlbumNode *n2 = resolve_album_song_token(a, song2, &prev2);

    if (!n1 || !n2) {
        printf("One or both songs not found in album \"%s\"\n", albumname);
        return;
    }

    if (n1 == n2) {
        printf("Both references refer to the same entry; nothing to swap\n");
        return;
    }

    AlbumNode *p1 = prev1, *p2 = prev2;
    AlbumNode *a1 = n1->next, *a2 = n2->next;

    if (p1) p1->next = n2; else a->head = n2;
    if (p2) p2->next = n1; else a->head = n1;

    if (n1->next == n2) {
        n2->next = n1;
        n1->next = a2;
    } else if (n2->next == n1) {
        n1->next = n2;
        n2->next = a1;
    } else {
        AlbumNode *tmp = n1->next;
        n1->next = n2->next;
        n2->next = tmp;
    }

    save_album_to_bin(a);
    printf("Swapped entries in album \"%s\".\n", albumname);
}

void handleManageSwapSongs(Command *cmd) {
    if (cmd->count != 5) {
        printf("Error! Invalid command format.\n");
        return;
    }
    manageSwapSongs(cmd->tokens[2], cmd->tokens[3], cmd->tokens[4]);
}

void manageMoveSong(const char *albumname, const char *songname, int position) {
    if (!albumname || !songname) {
        fprintf(stderr, "manageMoveSong: NULL argument\n");
        return;
    }
    if (position < 1) position = 1;

    Album *a = find_album_interactive(albumname);
    if (!a) {
        printf("Album \"%s\" not found\n", albumname);
        return;
    }

    AlbumNode *prev = NULL;
    AlbumNode *node = resolve_album_song_token(a, songname, &prev);
    if (!node) {
        printf("Song \"%s\" not found in album \"%s\"\n", songname, albumname);
        return;
    }

    if (!prev) a->head = node->next;
    else prev->next = node->next;
    node->next = NULL;

    if (position == 1 || !a->head) {
        node->next = a->head;
        a->head = node;
    } else {
        AlbumNode *cur = a->head;
        int idx = 1;
        while (cur->next && idx < position - 1) {
            cur = cur->next;
            idx++;
        }
        node->next = cur->next;
        cur->next = node;
    }
    
    save_album_to_bin(a);
    printf("Moved entry to position %d in album \"%s\"\n", position, albumname);
}

void handleManageMoveSong(Command *cmd) {
    if (cmd->count != 5) {
        printf("Error! Invalid command format.\n");
        return;
    }
    int position = atoi(cmd->tokens[4]);
    manageMoveSong(cmd->tokens[2], cmd->tokens[3], position);
}

void manageDeleteSong(const char *albumname, const char *songname) {
    if (!albumname || !songname) {
        fprintf(stderr, "manageDeleteSong: NULL argument\n");
        return;
    }

    Album *a = find_album_interactive(albumname);
    if (!a) {
        printf("Album \"%s\" not found\n", albumname);
        return;
    }

    AlbumNode *prev = NULL;
    AlbumNode *node = resolve_album_song_token(a, songname, &prev);
    if (!node) {
        printf("Song \"%s\" not found in album \"%s\"\n", songname, albumname);
        return;
    }

    if (!prev) a->head = node->next;
    else prev->next = node->next;
    free(node);
    
    save_album_to_bin(a);
    printf("Deleted entry from album \"%s\"\n", albumname);
}

void handleManageDeleteSong(Command *cmd) {
    if (cmd->count != 4) {
        printf("Error! Invalid command format.\n");
        return;
    }
    manageDeleteSong(cmd->tokens[2], cmd->tokens[3]);
}

void deleteAlbum(const char *albumname) {
    if (!albumname) {
        fprintf(stderr, "deleteAlbum: NULL argument\n");
        return;
    }

    Album *a = find_album_interactive(albumname);
    if (!a) {
        printf("Album \"%s\" not found\n", albumname);
        return;
    }
    
    char filepath[512];
    snprintf(filepath, sizeof(filepath), "utils/albums/%s_%d.bin", a->name, a->album_id);
    
    if (a->prev) a->prev->next = a->next;
    else g_albums = a->next;
    
    if (a->next) a->next->prev = a->prev;
    
    AlbumNode *node = a->head;
    while (node) {
        AlbumNode *next = node->next;
        free(node);
        node = next;
    }
    
    free(a->name);
    free(a);
    
    if (remove(filepath) == 0) {
        printf("Deleted album \"%s\"\n", albumname);
    } else {
        printf("Failed to delete album file, but removed from memory.\n");
    }
}

void handleDeleteAlbum(Command *cmd) {
    if (cmd->count != 3) {
        printf("Error! Invalid command format.\n");
        return;
    }
    deleteAlbum(cmd->tokens[2]);
}
