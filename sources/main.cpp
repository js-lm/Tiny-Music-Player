#include "music_player.hpp"

int main(int argumentCount, char *arguments[]){
	MusicPlayer musicPlayer{argumentCount, arguments};
    return musicPlayer.run();
}