#ifndef ALBUMS_H
#define ALBUMS_H

#include "structures.h"
#include "songs.h"

extern Album *g_albums;
extern int g_next_album_id;

int iequals(const char *a, const char *b);
Album* find_album_interactive(const char *name);
Album** find_all_albums_by_name(const char *name, int *count);
Album* find_album_by_number(int number);
int is_number_album(const char *str);
Album* create_album_internal(const char *name);
AlbumNode* album_find_node(Album *a, const char *title, AlbumNode **prev_out);
int album_append_song(Album *a, Song *s);

int load_album_from_bin_by_id(int album_id, Album **out);
int save_album_to_bin(const Album *a);
void load_all_albums();

void listAlbums();
void handleListAlbums(Command *cmd);
void listSongsInAlbum(const char *albumname);
void handleListSongsInAlbum(Command *cmd);
void createAlbum(const char *albumname, const char *songs[], int count);
void handleCreateAlbum(Command *cmd);
void manageAddSong(const char *albumname, const char *songname);
void handleManageAddSong(Command *cmd);
void manageSwapSongs(const char *albumname, const char *song1, const char *song2);
void handleManageSwapSongs(Command *cmd);
void manageMoveSong(const char *albumname, const char *songname, int position);
void handleManageMoveSong(Command *cmd);
void manageDeleteSong(const char *albumname, const char *songname);
void handleManageDeleteSong(Command *cmd);
void deleteAlbum(const char *albumname);
void handleDeleteAlbum(Command *cmd);

#endif