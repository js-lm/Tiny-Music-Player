for size in 16 32 64 128 256 512; do
    mkdir -p icons/hicolor/${size}x${size}/apps
    convert tiny-music-player.png -filter Point -resize ${size}x${size} icons/hicolor/${size}x${size}/apps/tiny-music-player.png
done
