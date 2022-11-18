# Solitaire
Sample solitaire game written in c++17 using SDL

To build on linux run
```
g++ --std=c++17 main.cpp -lSDL2 -lSDL2_image -o Solitaire
```

To build with the emscripten sdk run
```
emcc main.cpp -s USE_SDL=2 -s USE_SDL_IMAGE=2 -std=c++17 -o Solitaire.html --emrun -fexceptions --preload-file cards.png -sSDL2_IMAGE_FORMATS='["png"]' -O3
```
and then run with
```
emrun Solitaire.html
```
, you can also build it with asyncify by passing ```-s ASYNCIFY``` to the cli arguments.
