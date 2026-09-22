# MONTERRA - build
#   make assets  - regenerate art/tiles/font from tools/
#   make native  - native binary (needs libsdl2-dev), run with ./bin/monterra
#   make wasm    - web build (needs emsdk), serves files in web/
#   make test    - headless engine tests

CC      ?= gcc
EMCC    ?= /opt/emsdk/upstream/emscripten/emcc
CFLAGS  ?= -std=c11 -O2 -Wall -Wextra -Isrc
NATIVE_CFLAGS := $(CFLAGS) $(shell sdl2-config --cflags 2>/dev/null)
LDFLAGS ?=
SDL_LIBS = $(shell sdl2-config --libs 2>/dev/null || echo -lSDL2)

SRC := src/main.c src/core.c src/state.c src/save.c src/assets.c src/text.c src/ui.c \
       src/overworld.c src/battle.c src/data/tables.c src/data/maps.c

.PHONY: all native wasm test assets clean serve

all: native

native: bin/monterra

bin/monterra: $(SRC) | assets
	@mkdir -p bin
	$(CC) $(NATIVE_CFLAGS) -o $@ $(SRC) $(SDL_LIBS) -lm $(LDFLAGS)

test: bin/test_core
	./bin/test_core

bin/test_core: tests/test_core.c src/core.c src/save.c src/state.c src/data/tables.c src/data/maps.c
	@mkdir -p bin
	$(CC) $(CFLAGS) -o $@ tests/test_core.c src/core.c src/save.c src/state.c src/data/tables.c src/data/maps.c -lm

wasm: web/monterra.js

web/monterra.js: $(SRC) | assets
	$(EMCC) $(CFLAGS) -o web/monterra.js $(SRC) \
	    -sUSE_SDL=2 -sALLOW_MEMORY_GROWTH=1 \
	    --preload-file assets -O2 --closure 0

assets:
	python3 tools/bake_font.py
	python3 tools/gen_chars.py
	python3 tools/gen_tiles.py
	python3 tools/gen_maps.py
	python3 tools/process_art.py

serve: wasm
	cd web && python3 -m http.server 8060 --bind 0.0.0.0

clean:
	rm -rf bin web/monterra.js web/monterra.wasm web/monterra.data
