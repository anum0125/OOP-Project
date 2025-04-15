#include "raylib.h"
#include <iostream>
#include <vector>
#include <memory>
#include <string>

using namespace std;

// Abstract class: AudioFile
class AudioFile {
protected:
    string filename;
    string title;

public:
    AudioFile(const string& fname, const string& t) : filename(fname), title(t) {}
    virtual void play() = 0;
    virtual void stop() = 0;
    virtual string getTitle() const { return title; }
    virtual ~AudioFile() {}
};

// MP3 (actually OGG) class using Raylib
class MP3 : public AudioFile {
    Music music;
    bool loaded = false;

public:
    MP3(const string& fname, const string& t) : AudioFile(fname, t) {
        music = LoadMusicStream(fname.c_str());
        loaded = true;
    }

    void play() override {
        if (!loaded) return;
        PlayMusicStream(music);
        cout << "Now playing: " << title << endl;
    }

    void stop() override {
        if (!loaded) return;
        StopMusicStream(music);
        cout << "Stopped: " << title << endl;
    }

    Music& getMusic() { return music; }

    ~MP3() {
        if (loaded) UnloadMusicStream(music);
    }
};

// Playlist
class Playlist {
private:
    vector<shared_ptr<AudioFile>> songs;

public:
    void addSong(shared_ptr<AudioFile> song) {
        songs.push_back(song);
    }

    vector<shared_ptr<AudioFile>>& getSongs() {
        return songs;
    }

    size_t getSize() const { return songs.size(); }
};

// Player
class Player {
private:
    Playlist playlist;
    size_t currentSongIndex;
    bool isPlaying;

public:
    Player() : currentSongIndex(0), isPlaying(false) {}

    void addSong(shared_ptr<AudioFile> song) {
        playlist.addSong(song);
    }

    void playPause() {
        auto& currentSong = playlist.getSongs()[currentSongIndex];
        if (isPlaying) {
            currentSong->stop();
        } else {
            currentSong->play();
        }
        isPlaying = !isPlaying;
    }

    void stop() {
        playlist.getSongs()[currentSongIndex]->stop();
        isPlaying = false;
    }

    void next() {
        if (currentSongIndex < playlist.getSize() - 1) {
            stop();
            currentSongIndex++;
            playPause();
        }
    }

    void previous() {
        if (currentSongIndex > 0) {
            stop();
            currentSongIndex--;
            playPause();
        }
    }

    void displayCurrentSong() {
        cout << "Currently playing: " << playlist.getSongs()[currentSongIndex]->getTitle() << endl;
    }

    bool isPlayingSong() const {
        return isPlaying;
    }

    size_t& getCurrentSongIndex() { return currentSongIndex; }
    vector<shared_ptr<AudioFile>>& getSongs() { return playlist.getSongs(); }
};

// Button helper
bool IsMouseOver(Rectangle rect) {
    return CheckCollisionPointRec(GetMousePosition(), rect);
}

// Main
int main() {
    InitWindow(800, 600, "Raylib Music Player");
    InitAudioDevice();
    SetTargetFPS(60);

    Font font = LoadFont("resources/arial.ttf");

    Player player;
    player.addSong(make_shared<MP3>("resources/song1.ogg", "Song One"));
    player.addSong(make_shared<MP3>("resources/song2.ogg", "Song Two"));
    player.addSong(make_shared<MP3>("resources/song3.ogg", "Song Three"));

    Rectangle playPauseBtn = { 50, 500, 120, 40 };
    Rectangle stopBtn     = { 200, 500, 120, 40 };
    Rectangle nextBtn     = { 350, 500, 120, 40 };
    Rectangle prevBtn     = { 500, 500, 120, 40 };

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(BLACK);

        // Draw song list
        auto& songs = player.getSongs();
        for (size_t i = 0; i < songs.size(); ++i) {
            string title = songs[i]->getTitle();
            Vector2 pos = { 50, 50 + (float)i * 30 };
            DrawTextEx(font, title.c_str(), pos, 20, 2, RAYWHITE);

            Rectangle songRect = { pos.x, pos.y, 300, 25 };
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && IsMouseOver(songRect)) {
                if (player.getCurrentSongIndex() != i) {
                    player.stop();
                    player.getCurrentSongIndex() = i;
                    player.playPause();
                    player.displayCurrentSong();
                }
            }
        }

        // Buttons
        DrawRectangleRec(playPauseBtn, DARKGRAY);
        DrawText("Play/Pause", playPauseBtn.x + 10, playPauseBtn.y + 10, 20, WHITE);
        DrawRectangleRec(stopBtn, DARKGRAY);
        DrawText("Stop", stopBtn.x + 30, stopBtn.y + 10, 20, WHITE);
        DrawRectangleRec(nextBtn, DARKGRAY);
        DrawText("Next", nextBtn.x + 30, nextBtn.y + 10, 20, WHITE);
        DrawRectangleRec(prevBtn, DARKGRAY);
        DrawText("Previous", prevBtn.x + 10, prevBtn.y + 10, 20, WHITE);

        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            if (IsMouseOver(playPauseBtn)) {
                player.playPause();
                player.displayCurrentSong();
            }
            if (IsMouseOver(stopBtn)) {
                player.stop();
            }
            if (IsMouseOver(nextBtn)) {
                player.next();
                player.displayCurrentSong();
            }
            if (IsMouseOver(prevBtn)) {
                player.previous();
                player.displayCurrentSong();
            }
        }

        // Stream music updates
        if (player.isPlayingSong()) {
            auto& music = dynamic_pointer_cast<MP3>(player.getSongs()[player.getCurrentSongIndex()])->getMusic();
            UpdateMusicStream(music);
        }

        EndDrawing();
    }

    CloseAudioDevice();
    CloseWindow();
    return 0;
}
