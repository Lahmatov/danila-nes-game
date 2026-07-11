NAME     = game.nes
NESLIB   = neslib
CL65     = cl65
CFLAGS   = -t nes -Oisr -I $(NESLIB) -C $(NESLIB)/nes.cfg

SRC = $(NESLIB)/crt0.s chr_ru.s game.c $(NESLIB)/runtime.c

.PHONY: all clean run

all: $(NAME)

$(NAME): $(SRC) $(NESLIB)/nes.cfg $(NESLIB)/neslib.h $(NESLIB)/neslib.sinc $(NESLIB)/display.sinc $(NESLIB)/famitone2.sinc
	$(CL65) $(CFLAGS) -o $(NAME) $(SRC)

run: $(NAME)
	fceux $(NAME)

clean:
	rm -f $(NAME) *.o
