gcc src/linux_main.c src/linux_common.c src/game.c src/pcgrandom.c src/platform.c src/renderer.c src/events.c -o chess -O3 -I include -lX11 -Wall
