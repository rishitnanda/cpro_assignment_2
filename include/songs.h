#ifndef SONGS_H
#define SONGS_H

#include <signal.h>
#include <sys/types.h>
#include "structures.h"

extern Song *g_songs;
extern PlaybackState g_playback;
extern int g_next_song_id;

extern volatile sig_atomic_t pause_requested;
extern volatile sig_atomic_t resume_requested;
extern volatile sig_atomic_t next_requested;
extern volatile sig_atomic_t prev_requested;
extern volatile sig_atomic_t repeat_requested;
extern volatile sig_atomic_t loop_requested;

int parse_length(const char *s, SongLength *out);
void format_length(const SongLength *len, char *buf, size_t buflen);
long length_to_seconds(const SongLength *len);
int song_init(Song *s, const char *title, const char *artist, const char *length_str, int year);
void song_free(Song *s);
void song_print(const Song *s);

Song* find_song_by_title_interactive(const char *title);
Song** find_all_songs_by_title(const char *title, int *count);
Song* find_song_by_number(int number);
int is_number(const char *str);

int load_all_songs_from_bin();
int save_all_songs_to_bin();
int add_song_to_library(Song *s);

void handle_pause_signal(int sig);
void handle_resume_signal(int sig);
void handle_next_signal(int sig);
void handle_prev_signal(int sig);
void handle_repeat_signal(int sig);
void handle_loop_signal(int sig);

void init_playback_state();
void cleanup_playback_state();
void make_playlist_circular();
int playlist_insert_after_current(Song *songs[], int count);
void display_progress_bar();
void playback_loop();
void start_playback_process();
void stop_playback_process();

void listPlaylist();
void handleListPlaylist(Command *cmd);
void nextSongs(const char *songs[], int count);
void handleNextSongs(Command *cmd);
void nextAlbum(const char *albumname);
void handleNextAlbum(Command *cmd);
void pausePlayback();
void handlePause(Command *cmd);
void resumePlayback();
void handleResume(Command *cmd);
void fwd();
void handleFwd(Command *cmd);
void prev();
void handlePrev(Command *cmd);
void repeat();
void handleRepeat(Command *cmd);
void shuffle();
void handleShuffle(Command *cmd);
void removeSong(const char *songname);
void handleRemove(Command *cmd);
void loop();
void handleLoop(Command *cmd);

#endif