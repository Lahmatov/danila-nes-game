// Заглушка neslib.h для юнит-тестов на обычном компьютере (gcc).
// Объявления совпадают с настоящей библиотекой, но вместо железа NES
// всё пишется в тестовые массивы (см. tests/test_game.c) -- так логику
// game.c можно проверять быстро, без эмулятора.
#ifndef _NESLIB_STUB_H
#define _NESLIB_STUB_H

// Кнопки и атрибуты -- те же значения, что в настоящей neslib.h.
#define PAD_A      0x01
#define PAD_B      0x02
#define PAD_SELECT 0x04
#define PAD_START  0x08
#define PAD_UP     0x10
#define PAD_DOWN   0x20
#define PAD_LEFT   0x40
#define PAD_RIGHT  0x80

#define OAM_FLIP_V 0x80
#define OAM_FLIP_H 0x40

#define NTADR_A(x, y) (0x2000 | (((y) & 0xFF) << 5) | ((x) & 0xFF))

void pal_all(const unsigned char *data);
void pal_col(unsigned char index, unsigned char color);
void ppu_off(void);
void ppu_on_all(void);
void ppu_wait_nmi(void);

void oam_clear(void);
unsigned char oam_spr(unsigned char x, unsigned char y,
                      unsigned char chrnum, unsigned char attr,
                      unsigned char sprid);

void vram_adr(unsigned int adr);
void vram_put(unsigned char n);
void vram_fill(unsigned char n, unsigned int len);
void vram_write(const unsigned char *src, unsigned int size);

unsigned char pad_trigger(unsigned char pad);
unsigned char pad_state(unsigned char pad);

void bank_spr(unsigned char n);
void bank_bg(unsigned char n);

unsigned char rand8(void);

#endif
