NAME     = game.nes
NESLIB   = neslib
CL65     = cl65
CFLAGS   = -t nes -Oisr -I $(NESLIB) -C $(NESLIB)/nes.cfg

SRC = $(NESLIB)/crt0.s chr_ru.s game.c $(NESLIB)/runtime.c

.PHONY: all clean run test

all: $(NAME)

# Юнит-тесты логики: game.c собирается обычным gcc с заглушками
# вместо железа NES (tests/stubs/) и гоняется на компьютере.
test: game.c tests/test_game.c tests/stubs/neslib.h tests/stubs/peekpoke.h
	mkdir -p build
	cc -std=c99 -Wall -Wextra -Wno-unused-parameter -I tests/stubs -o build/unit_tests tests/test_game.c
	./build/unit_tests

$(NAME): $(SRC) $(NESLIB)/nes.cfg $(NESLIB)/neslib.h $(NESLIB)/neslib.sinc $(NESLIB)/display.sinc $(NESLIB)/famitone2.sinc
	$(CL65) $(CFLAGS) -o $(NAME) $(SRC)

run: $(NAME)
	fceux $(NAME)

clean:
	rm -f $(NAME) *.o
