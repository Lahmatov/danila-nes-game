// ============================================
//  ВЕРСИЯ 11: ПЛАТФОРМЕР!
//  Игра переродилась: теперь это 2D-приключение
//  как Марио. Четыре уровня-комнаты, вещи надо
//  доставать прыжками, между уровнями -- заставки
//  с напутствиями семьи, в финале Данила качает
//  тёмно-золотую коляску.
// ============================================

#include "neslib.h"        // библиотека NES: спрайты, палитры, геймпад
#include <peekpoke.h>      // POKE() -- пишем звук прямо в регистры APU

//#link "chr_ru.s"         // наш набор тайлов: буквы + вся графика игры

// --- Номера тайлов ---
#define T_WALL    0xA1     // кирпич
#define T_FLOOR   0xA2     // фон комнаты / звёзды на улице
#define T_BOX     0xA3     // коробка (можно стоять)
#define HERO_HEAD 0xA4
#define HERO_BODY 0xA5
#define T_STROL_L 0xB4     // коляска: левая половина
#define T_STROL_R 0xB5     // коляска: правая половина
#define T_WINDOW  0xBA     // окно со звёздами
#define T_RUG     0xBB     // коврик
#define T_CRIB    0xBC     // кроватка (можно стоять)
#define PET_A     0xAE     // котёнок-компаньон, кадр 1
#define PET_B     0xAF     // котёнок-компаньон, кадр 2 (хвостик)
#define PORTRAIT  0xC0     // портрет 6x6 тайлов

// --- Палитра ---
const unsigned char PALETTE[32] = {
  0x0F,                   // общий цвет фона: чёрный

  0x27,0x17,0x30,0x00,    // фон 0: кирпич, тёмный, белый (текст)
  0x28,0x36,0x30,0x00,    // фон 1: портрет
  0x00,0x10,0x20,0x00,    // фон 2
  0x06,0x16,0x26,0x00,    // фон 3

  0x2A,0x36,0x30,0x00,    // спрайты 0: Данила -- зелёный, кожа, белый-блонд
  0x25,0x36,0x17,0x00,    // спрайты 1: мама Карина -- розовый, кожа, КОРИЧНЕВЫЕ волосы
  0x26,0x36,0x0F,0x00,    // спрайты 2: бабушки, предметы, коляска --
                          //   тёмное золото/РЫЖИЙ, кожа, ТЁМНЫЙ
  0x21,0x36,0x28          // спрайты 3: папа Женя -- синий, кожа, СВЕТЛЫЕ волосы
};

// --- Сцены: 0 = заставка уровня, 1 = играем, 2 = финал ---
unsigned char scene = 0;
unsigned char level = 0;    // текущий уровень: 0..3

// ============================================
//  СЕМЬЯ (появляется на заставках уровней)
// ============================================
#define N_NPC 4
const unsigned char NPC_HEAD[N_NPC] = { 0xA6, 0xA8, 0xAA, 0xAB };
const unsigned char NPC_BODY[N_NPC] = { 0xA7, 0xA9, 0xAD, 0xAC };
const unsigned char NPC_PAL[N_NPC]  = { 1,    3,    2,    2    };
const char* NPC_NAME[N_NPC] = {
  "MAMA KARINA",    // МАМА КАРИНА
  "PAPA ZHENYA",    // ПАПА ЖЕНЯ
  "BABUWKA INNA",   // БАБУШКА ИННА
  "BABUWKA VIKA"    // БАБУШКА ВИКА
};

// ============================================
//  УРОВНИ: название, кто напутствует и что говорит
// ============================================
#define N_LEVELS 4
const char* LEVEL_NAME[N_LEVELS] = {
  "GOSTINAYA",   // ГОСТИНАЯ
  "DETSKAYA",    // ДЕТСКАЯ
  "KUHNYA",      // КУХНЯ
  "ULICA"        // УЛИЦА
};
const unsigned char LEVEL_NAME_COL[N_LEVELS] = { 12, 12, 13, 13 };
const unsigned char LEVEL_SPEAKER[N_LEVELS]  = { 0, 1, 2, 3 };
const char* LEVEL_LINE[N_LEVELS][2] = {
  { "SOBERI VEQI DLYA MALYWKI!",   // СОБЕРИ ВЕЩИ ДЛЯ МАЛЫШКИ!
    "PRYGAJ KNOPKOJ A!" },         // ПРЫГАЙ КНОПКОЙ А!
  { "KAKAYA KRASIVAYA KOMNATA!",   // КАКАЯ КРАСИВАЯ КОМНАТА!
    "IQI VEQI NAVERHU!" },         // ИЩИ ВЕЩИ НАВЕРХУ!
  { "ZAGLYANI NA POLKI!",          // ЗАГЛЯНИ НА ПОЛКИ!
    "ZALEZAJ POVYWE!" },           // ЗАЛЕЗАЙ ПОВЫШЕ!
  { "SESTRYONKA POD~EZZHAET!",     // СЕСТРЁНКА ПОДЪЕЗЖАЕТ!
    "BEGI VSTRECHAT'!" }           // БЕГИ ВСТРЕЧАТЬ!
};

// ============================================
//  ПРЕДМЕТЫ: теперь у каждого свой уровень
// ============================================
#define N_ITEMS 8
const unsigned char ITEM_LEVEL[N_ITEMS] = { 0,    0,    1,    1,    1,    2,    2,    3    };
const unsigned char ITEM_X[N_ITEMS]     = { 136,  192,  48,   112,  176,  44,   160,  104  };
const unsigned char ITEM_Y[N_ITEMS]     = { 160,  168,  184,  160,  120,  192,  144,  176  };
const unsigned char ITEM_TILE[N_ITEMS]  = { 0xB0, 0xB2, 0xB1, 0xB7, 0xB8, 0xB6, 0xB9, 0xBD };
const char* ITEM_NAME[N_ITEMS] = {
  "POGREMUWKU!",    // ПОГРЕМУШКУ!
  "MIWKU!",         // МИШКУ!
  "ODEYAL'CE!",     // ОДЕЯЛЬЦЕ!
  "KNIZHKU!",       // КНИЖКУ!
  "SOSKU!",         // СОСКУ!
  "BUTYLOCHKU!",    // БУТЫЛОЧКУ!
  "LOZHECHKU!",     // ЛОЖЕЧКУ!
  "PODAROK VIKI!"   // ПОДАРОК ВИКИ!
};

// ============================================
//  СЕКРЕТНЫЙ БОНУС: по одной звёздочке на уровень,
//  спрятана в стороне от обычного маршрута. Не мешает
//  пройти уровень -- просто приятная находка для тех,
//  кто заглянул куда не просили.
// ============================================
#define T_SECRET 0xBE
const unsigned char SECRET_X[N_LEVELS] = { 64,  208, 224, 224 };
const unsigned char SECRET_Y[N_LEVELS] = { 168, 184, 192, 176 };
unsigned char secret_taken[N_LEVELS] = { 0, 0, 0, 0 };
unsigned char secret_total = 0;   // сколько всего найдено за игру

// ============================================
//  МЯЧИК: катается туда-сюда по полу. Если задеть --
//  Данилу мягко отталкивает назад, никакого "game over".
// ============================================
#define T_BALL 0xBF
const unsigned char HAZARD_Y[N_LEVELS]    = { 208, 208, 208, 208 };
const unsigned char HAZARD_MINX[N_LEVELS] = { 150, 80,  90,  140 };
const unsigned char HAZARD_MAXX[N_LEVELS] = { 210, 140, 150, 200 };
unsigned char hazard_x;
signed char hazard_dir;
unsigned char hero_bump = 0;   // кадры неуязвимости после отскока
unsigned char t = 0;           // общий счётчик кадров (анимации, мигание)

// Память игры.
unsigned char item_taken[N_ITEMS] = { 0,0,0,0,0,0,0,0 };
unsigned char lvl_found = 0;   // собрано на этом уровне
unsigned char lvl_total = 0;   // сколько всего на этом уровне

// ============================================
//  ПРОСТОЙ ЗВУК: пишем прямо в APU, без FamiTone
//  (FamiToneUpdate выключен в neslib.sinc, чтобы не
//  затирал регистры каждый кадр).
// ============================================
#define APU_SND_CHN 0x4015
#define SQ1_VOL     0x4000
#define SQ1_SWEEP   0x4001
#define SQ1_LO      0x4002
#define SQ1_HI      0x4003
#define SQ2_VOL     0x4004
#define SQ2_LO      0x4006
#define SQ2_HI      0x4007

#define SFX_NONE   0
#define SFX_JUMP   1
#define SFX_ITEM   2
#define SFX_SELECT 3
#define SFX_SECRET 4
#define SFX_BUMP   5

unsigned char sfx_kind  = SFX_NONE;
unsigned char sfx_timer = 0;

// Мелодия находки: три ноты вверх (до-ми-соль), период таймера APU.
const unsigned int ITEM_NOTES[3] = { 507, 402, 319 };
// Мелодия секрета: четыре ноты, повеселее и подольше (до-ми-соль-до).
const unsigned int SECRET_NOTES[4] = { 507, 402, 319, 253 };

void sfx_start(unsigned char kind) {
  sfx_kind  = kind;
  if (kind == SFX_ITEM)        sfx_timer = 12;
  else if (kind == SFX_SECRET) sfx_timer = 16;
  else if (kind == SFX_BUMP)   sfx_timer = 5;
  else                          sfx_timer = 7;
}

// Вызывается ровно раз за кадр (перед ppu_wait_nmi), крутит текущий звук.
void sfx_update(void) {
  unsigned char step, vol;
  unsigned int period;

  if (!sfx_timer) return;
  --sfx_timer;

  if (sfx_kind == SFX_JUMP) {
    // Пикирующий свист вверх: период уменьшается -- тон растёт.
    period = 300 - (unsigned int)(6 - sfx_timer) * 28;
    if (period < 110) period = 110;
    vol = sfx_timer + 8;
    if (vol > 15) vol = 15;
    POKE(SQ1_VOL, 0xB0 | vol);
    POKE(SQ1_LO,  period & 0xFF);
    POKE(SQ1_HI,  (period >> 8) & 0x07);
  }
  else if (sfx_kind == SFX_ITEM) {
    step = sfx_timer >> 2;
    if (step > 2) step = 2;
    period = ITEM_NOTES[2 - step];
    POKE(SQ2_VOL, 0xBF);
    POKE(SQ2_LO,  period & 0xFF);
    POKE(SQ2_HI,  (period >> 8) & 0x07);
  }
  else if (sfx_kind == SFX_SELECT) {
    POKE(SQ1_VOL, 0xBA);
    POKE(SQ1_LO,  214);
    POKE(SQ1_HI,  0);
  }
  else if (sfx_kind == SFX_SECRET) {
    step = sfx_timer >> 2;
    if (step > 3) step = 3;
    period = SECRET_NOTES[3 - step];
    POKE(SQ2_VOL, 0xBF);
    POKE(SQ2_LO,  period & 0xFF);
    POKE(SQ2_HI,  (period >> 8) & 0x07);
  }
  else if (sfx_kind == SFX_BUMP) {
    // короткий низкий "бонк" -- не страшно, просто отскок
    vol = sfx_timer + 9;
    if (vol > 15) vol = 15;
    POKE(SQ1_VOL, 0xB0 | vol);
    POKE(SQ1_LO,  900 & 0xFF);
    POKE(SQ1_HI,  (900 >> 8) & 0x07);
  }

  if (!sfx_timer) {
    POKE(SQ1_VOL, 0xB0);   // тишина в конце
    POKE(SQ2_VOL, 0xB0);
    sfx_kind = SFX_NONE;
  }
}

// ============================================
//  КАРТЫ УРОВНЕЙ (вид сбоку)
//  '#' стена/платформа, 'B' коробка, 'K' кроватка,
//  'w' окно, 'r' коврик, 'h' сердечко на обоях
// ============================================
// --- Уровень 1: ГОСТИНАЯ ---
const char MAP_L1[30][33] = {
  "################################",
  "#..............................#",
  "#..............................#",
  "#..............................#",
  "#..............................#",
  "#.......ww............ww.......#",
  "#.......ww............ww.......#",
  "#.......ww............ww.......#",
  "#..............................#",
  "#..............................#",
  "#..............................#",
  "#..............................#",
  "#..............................#",
  "#..............................#",
  "#..............................#",
  "#..............................#",
  "#..............................#",
  "#..............................#",
  "#..............................#",
  "#..............................#",
  "#..............................#",
  "#.............######...........#",
  "#..............................#",
  "#..............................#",
  "#.....######..........######...#",
  "#..............................#",
  "#..BB...........................",
  "################################",
  "################################",
  "################################"
};
// --- Уровень 2: ДЕТСКАЯ (красивая комната) ---
const char MAP_L2[30][33] = {
  "################################",
  "#..............................#",
  "#..............................#",
  "#..............................#",
  "#..........h.......h...........#",
  "#....ww........................#",
  "#....ww........................#",
  "#........................h.....#",
  "#..............................#",
  "#..............................#",
  "#..............................#",
  "#..............................#",
  "#..............................#",
  "#..............................#",
  "#..............................#",
  "#..............................#",
  "#..............................#",
  "#..............................#",
  "#...................######.....#",
  "#..............................#",
  "#..............................#",
  "#...........######.............#",
  "#..............................#",
  "#..............................#",
  "#...######.....................#",
  "#.......................KKK....#",
  "#.......rrrrrrr.........KKK....#",
  "################################",
  "################################",
  "################################"
};
// --- Уровень 3: КУХНЯ ---
const char MAP_L3[30][33] = {
  "################################",
  "#..............................#",
  "#..............................#",
  "#..............................#",
  "#..............................#",
  "#..............ww..............#",
  "#..............ww..............#",
  "#..............................#",
  "#..............................#",
  "#..............................#",
  "#..............................#",
  "#..............................#",
  "#..............................#",
  "#..............................#",
  "#..............................#",
  "#..............................#",
  "#..............................#",
  "#..............................#",
  "#..............................#",
  "#.................######.......#",
  "#..............................#",
  "#..............................#",
  "#.........######...............#",
  "#..............................#",
  "#..............................#",
  "#....BB.........................",
  "#....BB.........................",
  "################################",
  "################################",
  "################################"
};
// --- Уровень 4: УЛИЦА (ночь, звёзды) ---
const char MAP_L4[30][33] = {
  "#..............................#",
  "#..............................#",
  "#..............................#",
  "#..............................#",
  "#..............................#",
  "#..............................#",
  "#..............................#",
  "#..............................#",
  "#..............................#",
  "#..............................#",
  "#..............................#",
  "#..............................#",
  "#..............................#",
  "#..............................#",
  "#..............................#",
  "#..............................#",
  "#..............................#",
  "#..............................#",
  "#..............................#",
  "#..............................#",
  "#..............................#",
  "#..............................#",
  "#..............................#",
  "#..............................#",
  "#..............................#",
  "#............BB..........BB....#",
  "#.......B....BB.....B....BB....#",
  "################################",
  "################################",
  "################################"
};

// Латинская буква (A..Z) -> номер тайла с русской буквой.
// Особые: J=Й C=Ц W=Ш Q=Щ Y=Ы X=Э '=Ь ~=Ъ, пары: ZH CH YA YU YO
const unsigned char RU_SINGLE[26] = {
//   A     B     C     D     E     F     G     H     I     J
  0x80, 0x81, 0x97, 0x84, 0x85, 0x95, 0x83, 0x96, 0x89, 0x8A,
//   K     L     M     N     O     P     Q     R     S     T
  0x8B, 0x8C, 0x8D, 0x8E, 0x8F, 0x90, 0x9A, 0x91, 0x92, 0x93,
//   U     V     W     X     Y     Z
  0x94, 0x82, 0x99, 0x9E, 0x9C, 0x88
};

// Символ карты текущего уровня.
unsigned char map_at(unsigned char r, unsigned char c) {
  if (level == 0) return MAP_L1[r][c];
  if (level == 1) return MAP_L2[r][c];
  if (level == 2) return MAP_L3[r][c];
  return MAP_L4[r][c];
}

// Твёрдое: стены, коробки и кроватка. Окна, коврики
// и сердечки -- просто украшения, сквозь них ходим.
unsigned char is_wall(unsigned char px, unsigned char py) {
  unsigned char t = map_at(py / 8, px / 8);
  return t == '#' || t == 'B' || t == 'K';
}

// Рисует уровень: каждому символу карты -- свой тайл.
void draw_room(void) {
  unsigned char row, col, t;
  unsigned char buf[32];
  ppu_off();
  for (row = 0; row < 30; ++row) {
    for (col = 0; col < 32; ++col) {
      t = map_at(row, col);
      if (t == '#')      buf[col] = T_WALL;
      else if (t == 'B') buf[col] = T_BOX;
      else if (t == 'K') buf[col] = T_CRIB;
      else if (t == 'w') buf[col] = T_WINDOW;
      else if (t == 'r') buf[col] = T_RUG;
      else if (t == 'h') buf[col] = 0x2A;   // сердечко: тайл звёздочки '*'
      else               buf[col] = T_FLOOR;
    }
    vram_adr(NTADR_A(0, row));
    vram_write(buf, 32);
  }
  vram_adr(0x23C0);
  vram_fill(0, 64);
  ppu_on_all();
}

// Пишет строку транслитом, превращая её в кириллицу. Возвращает длину.
unsigned char put_ru(const char* s) {
  unsigned char n = 0;
  unsigned char c, next;
  while (*s) {
    c = *s;
    next = *(s + 1);
    if (c >= 'A' && c <= 'Z') {
      if      (c == 'Z' && next == 'H') { vram_put(0x87); ++s; }  // ZH -> Ж
      else if (c == 'C' && next == 'H') { vram_put(0x98); ++s; }  // CH -> Ч
      else if (c == 'Y' && next == 'A') { vram_put(0xA0); ++s; }  // YA -> Я
      else if (c == 'Y' && next == 'U') { vram_put(0x9F); ++s; }  // YU -> Ю
      else if (c == 'Y' && next == 'O') { vram_put(0x86); ++s; }  // YO -> Ё
      else vram_put(RU_SINGLE[c - 'A']);
    }
    else if (c == 0x27) vram_put(0x9D);   // ' -> Ь
    else if (c == '~')  vram_put(0x9B);   // ~ -> Ъ
    else vram_put(c);
    ++s;
    ++n;
  }
  return n;
}

// Одна строка окна: рамка, текст, добивка пробелами, рамка.
void window_row(const char* s) {
  unsigned char i;
  vram_put('#');
  vram_put(' ');
  i = put_ru(s);
  while (i < 24) { vram_put(' '); ++i; }
  vram_put(' ');
  vram_put('#');
}

// Окно с тремя строками поверх уровня.
void show_window(const char* name, const char* line1, const char* line2) {
  unsigned char i;
  ppu_off();
  vram_adr(NTADR_A(2, 22));
  for (i = 0; i < 28; ++i) vram_put('#');
  vram_adr(NTADR_A(2, 23));
  window_row(name);
  vram_adr(NTADR_A(2, 24));
  window_row(line1);
  vram_adr(NTADR_A(2, 25));
  window_row(line2);
  vram_adr(NTADR_A(2, 26));
  for (i = 0; i < 28; ++i) vram_put('#');
  ppu_on_all();
}

// --- Титульный экран ---
void draw_title(void) {
  unsigned char r, c;
  ppu_off();
  vram_adr(NTADR_A(0, 0));
  vram_fill(' ', 960);
  for (r = 0; r < 6; ++r) {
    vram_adr(NTADR_A(13, 4 + r));
    for (c = 0; c < 6; ++c) vram_put(PORTRAIT + r * 6 + c);
  }
  vram_adr(NTADR_A(3, 13));
  put_ru("DANILA STAL STARWIM BRATOM");   // ДАНИЛА СТАЛ СТАРШИМ БРАТОМ
  vram_adr(NTADR_A(9, 18));
  put_ru("NAZHMI KNOPKU A");              // НАЖМИ КНОПКУ А
  vram_adr(0x23C0);
  vram_fill(0, 64);
  vram_adr(0x23C0 + 8 + 3);
  vram_put(0x55); vram_put(0x55);
  vram_adr(0x23C0 + 16 + 3);
  vram_put(0x55); vram_put(0x55);
  ppu_on_all();
}

// --- Заставка перед уровнем: номер, имя комнаты, напутствие ---
void draw_card(void) {
  unsigned char sp = LEVEL_SPEAKER[level];
  ppu_off();
  vram_adr(NTADR_A(0, 0));
  vram_fill(' ', 960);
  vram_adr(NTADR_A(11, 6));
  put_ru("UROVEN'");                      // УРОВЕНЬ
  vram_put(' ');
  vram_put('1' + level);
  vram_adr(NTADR_A(LEVEL_NAME_COL[level], 8));
  put_ru(LEVEL_NAME[level]);
  vram_adr(NTADR_A(11, 14));
  put_ru(NPC_NAME[sp]);
  vram_adr(NTADR_A(4, 16));
  put_ru(LEVEL_LINE[level][0]);
  vram_adr(NTADR_A(4, 18));
  put_ru(LEVEL_LINE[level][1]);
  vram_adr(NTADR_A(9, 24));
  put_ru("NAZHMI KNOPKU A");
  vram_adr(0x23C0);
  vram_fill(0, 64);
  ppu_on_all();
}

// --- Финальный экран ---
void draw_final(void) {
  ppu_off();
  vram_adr(NTADR_A(0, 0));
  vram_fill(' ', 960);
  vram_adr(NTADR_A(6, 4));
  put_ru("URA! SESTRYONKA DOMA!");        // УРА! СЕСТРЁНКА ДОМА!
  vram_adr(NTADR_A(8, 7));
  put_ru("SPASIBO, DANILA!");             // СПАСИБО, ДАНИЛА!
  vram_adr(NTADR_A(4, 9));
  put_ru("TY LUCHWIJ STARWIJ BRAT!");     // ТЫ ЛУЧШИЙ СТАРШИЙ БРАТ!
  vram_adr(NTADR_A(6, 27));
  put_ru("NAZHMI START - ZANOVO");        // НАЖМИ СТАРТ - ЗАНОВО
  vram_adr(0x23C0);
  vram_fill(0, 64);
  ppu_on_all();
}

// Персонаж = голова + туловище.
unsigned char draw_person(unsigned char px, unsigned char py,
                          unsigned char head, unsigned char body,
                          unsigned char pal, unsigned char id) {
  id = oam_spr(px, py - 8, head, pal, id);
  return oam_spr(px, py, body, pal, id);
}

// Начало уровня: считаем предметы, ставим героя у левого края.
unsigned char hero_x, hero_y;

// Котёнок-компаньон: бежит следом за Данилой, держась чуть позади.
unsigned char pet_x, pet_y;
int pet_vy, pet_vsub;
unsigned char pet_facing;   // 0 -- смотрит вправо, 1 -- влево

void start_level(void) {
  unsigned char i;
  lvl_found = 0;
  lvl_total = 0;
  for (i = 0; i < N_ITEMS; ++i) {
    if (ITEM_LEVEL[i] == level) ++lvl_total;
  }
  hero_x = 16;
  hero_y = 208;
  pet_x = 0;
  pet_y = 208;
  pet_vy = 0;
  pet_vsub = 0;
  pet_facing = 0;
  hazard_x = HAZARD_MINX[level];
  hazard_dir = 1;
  hero_bump = 0;
  draw_room();
}

// Катает мячик туда-сюда по полу между HAZARD_MINX и HAZARD_MAXX.
void update_hazard(void) {
  if (!(t & 1)) return;   // едет не спеша -- через кадр
  hazard_x += hazard_dir;
  if (hazard_x <= HAZARD_MINX[level]) { hazard_x = HAZARD_MINX[level]; hazard_dir = 1; }
  if (hazard_x >= HAZARD_MAXX[level]) { hazard_x = HAZARD_MAXX[level]; hazard_dir = -1; }
}

// Двигает котёнка к точке чуть позади героя, с той же гравитацией,
// что и у героя, но без прыжков -- просто бежит и падает на пол.
void update_pet(void) {
  unsigned char on_ground_pet;
  int target;

  target = hero_x - 16;
  if (target < 0) target = 0;

  if (pet_x < target && !is_wall(pet_x + 8, pet_y) && !is_wall(pet_x + 8, pet_y + 7)) {
    ++pet_x;
    pet_facing = 0;
  } else if (pet_x > target && !is_wall(pet_x - 1, pet_y) && !is_wall(pet_x - 1, pet_y + 7)) {
    --pet_x;
    pet_facing = 1;
  }

  on_ground_pet = is_wall(pet_x, pet_y + 8) || is_wall(pet_x + 7, pet_y + 8);
  if (on_ground_pet) {
    pet_vy = 0;
    pet_vsub = 0;
  } else {
    pet_vy += 3;
    if (pet_vy > 64) pet_vy = 64;
  }
  pet_vsub += pet_vy;
  while (pet_vsub >= 16) {
    pet_vsub -= 16;
    if (!is_wall(pet_x, pet_y + 8) && !is_wall(pet_x + 7, pet_y + 8)) ++pet_y;
    else pet_vsub = 0;
  }
}

void main(void) {
  unsigned char pad;
  unsigned char pad_t;
  unsigned char talking = 0;
  unsigned char i;
  unsigned char oam_id;
  unsigned char on_ground;
  int vy = 0;             // вертикальная скорость: 16-е доли пикселя за кадр
  int vsub = 0;           // копилка накопленных долей

  pal_all(PALETTE);
  bank_spr(0);

  // Титул -> заставка первого уровня.
  draw_title();
  while (1) {
    if (pad_trigger(0) & (PAD_A | PAD_START)) break;
    ppu_wait_nmi();
  }
  scene = 0;
  draw_card();

  while (1) {
    pad_t = pad_trigger(0);
    pad   = pad_state(0);
    ++t;

    if (talking) {
      // Открыто окно: ждём A. Заметь: этот же A НЕ станет
      // прыжком -- нажатие уже "потрачено" на закрытие окна.
      if (pad_t & PAD_A) {
        talking = 0;
        draw_room();
      }
    }
    else if (scene == 0) {
      // ===== ЗАСТАВКА УРОВНЯ =====
      if (pad_t & (PAD_A | PAD_START)) {
        scene = 1;
        vy = 0;
        vsub = 0;
        sfx_start(SFX_SELECT);
        start_level();
      }
    }
    else if (scene == 1) {
      // ===== УРОВЕНЬ: бег и прыжки =====
      if ((pad & PAD_LEFT)  && !is_wall(hero_x - 1, hero_y) && !is_wall(hero_x - 1, hero_y + 7)) hero_x -= 1;
      if ((pad & PAD_RIGHT) && !is_wall(hero_x + 8, hero_y) && !is_wall(hero_x + 8, hero_y + 7)) hero_x += 1;

      // Пока не всё собрано, дальше правого края не пускаем.
      if (lvl_found < lvl_total && hero_x > 228) hero_x = 228;

      // Стоим ли на чём-то?
      on_ground = is_wall(hero_x, hero_y + 8) || is_wall(hero_x + 7, hero_y + 8);
      if (on_ground) {
        if (vy > 0) { vy = 0; vsub = 0; }
        if (pad_t & PAD_A) { vy = -52; sfx_start(SFX_JUMP); }        // прыжок
      } else {
        vy += 3;                            // гравитация
        if (vy > 64) vy = 64;
      }
      vsub += vy;
      while (vsub <= -16) {                 // вверх по пикселю
        vsub += 16;
        if (!is_wall(hero_x, hero_y - 1) && !is_wall(hero_x + 7, hero_y - 1)) --hero_y;
        else { vy = 0; vsub = 0; }
      }
      while (vsub >= 16) {                  // вниз по пикселю
        vsub -= 16;
        if (!is_wall(hero_x, hero_y + 8) && !is_wall(hero_x + 7, hero_y + 8)) ++hero_y;
        else { vsub = 0; }
      }

      // Подбор предметов (в том числе в прыжке!)
      for (i = 0; i < N_ITEMS; ++i) {
        if (!item_taken[i] && ITEM_LEVEL[i] == level &&
            hero_x + 8 > ITEM_X[i] && ITEM_X[i] + 8 > hero_x &&
            hero_y + 8 > ITEM_Y[i] && ITEM_Y[i] + 8 > hero_y) {
          item_taken[i] = 1;
          ++lvl_found;
          talking = 1;
          sfx_start(SFX_ITEM);
          if (lvl_found == lvl_total) {
            show_window("NAHODKA!", ITEM_NAME[i], "SOBRANO! BEGI VPRAVO!");   // СОБРАНО! БЕГИ ВПРАВО!
          } else {
            show_window("NAHODKA!", "TY NAWYOL:", ITEM_NAME[i]);
          }
        }
      }

      // Секретная звёздочка -- необязательная, не мешает пройти уровень.
      if (!secret_taken[level] &&
          hero_x + 8 > SECRET_X[level] && SECRET_X[level] + 8 > hero_x &&
          hero_y + 8 > SECRET_Y[level] && SECRET_Y[level] + 8 > hero_y) {
        secret_taken[level] = 1;
        ++secret_total;
        talking = 1;
        sfx_start(SFX_SECRET);
        show_window("SEKRET!", "TY NAWYOL ZVEZDOCHKU!", "MOLODEC, ISSLEDOVATEL'!");
      }

      update_pet();
      update_hazard();

      // Мячик заде́л Данилу -- мягкий отскок назад, без потерь и "game over".
      if (hero_bump) {
        --hero_bump;
      } else if (hero_x + 8 > hazard_x && hazard_x + 8 > hero_x &&
                 hero_y + 8 > HAZARD_Y[level] && HAZARD_Y[level] + 8 > hero_y) {
        if (hero_x < hazard_x) { if (hero_x > 10) hero_x -= 10; else hero_x = 0; }
        else                    { hero_x += 10; }
        hero_bump = 30;
        sfx_start(SFX_BUMP);
      }

      // Всё собрано и добежал до правого края -- уровень пройден.
      if (lvl_found == lvl_total && hero_x >= 240) {
        ++level;
        if (level < N_LEVELS) {
          scene = 0;
          draw_card();
        } else {
          scene = 2;
          draw_final();
        }
      }
    }
    else {
      // ===== ФИНАЛ: START -- начать заново =====
      if (pad_t & PAD_START) {
        for (i = 0; i < N_ITEMS; ++i) item_taken[i] = 0;
        level = 0;
        scene = 0;
        draw_title();
        while (1) {
          if (pad_trigger(0) & (PAD_A | PAD_START)) break;
          ppu_wait_nmi();
        }
        draw_card();
      }
    }

    // --- Спрайты кадра ---
    oam_clear();
    oam_id = 0;
    if (scene == 0) {
      // на заставке стоит тот, кто напутствует
      i = LEVEL_SPEAKER[level];
      oam_id = draw_person(124, 96, NPC_HEAD[i], NPC_BODY[i], NPC_PAL[i], oam_id);
    }
    else if (scene == 1) {
      if (!hero_bump || (hero_bump & 2)) {   // во время отскока герой мигает
        oam_id = draw_person(hero_x, hero_y, HERO_HEAD, HERO_BODY, 0, oam_id);
      }
      oam_id = oam_spr(pet_x, pet_y, (t & 16) ? PET_B : PET_A,
                        2 | (pet_facing ? OAM_FLIP_H : 0), oam_id);
      oam_id = oam_spr(hazard_x, HAZARD_Y[level], T_BALL, 1, oam_id);
      for (i = 0; i < N_ITEMS; ++i) {
        if (!item_taken[i] && ITEM_LEVEL[i] == level) {
          oam_id = oam_spr(ITEM_X[i], ITEM_Y[i], ITEM_TILE[i], 2, oam_id);
        }
      }
      if (!secret_taken[level] && (t & 8)) {   // мерцает -- всё-таки секрет
        oam_id = oam_spr(SECRET_X[level], SECRET_Y[level], T_SECRET, 2, oam_id);
      }
      oam_id = oam_spr(16, 16, '0' + lvl_found, 0, oam_id);
      oam_id = oam_spr(24, 16, '/', 0, oam_id);
      oam_id = oam_spr(32, 16, '0' + lvl_total, 0, oam_id);
    }
    else {
      // ===== МИНИ-ВИДЕО: Данила качает коляску =====
      // Каждые полсекунды коляска смещается на пиксель --
      // получается покачивание. i здесь -- смещение 0 или 1.
      i = (t & 32) ? 1 : 0;
      oam_id = draw_person(64,  128, 0xA6, 0xA7, 1, oam_id);   // мама
      oam_id = draw_person(88,  128, 0xA8, 0xA9, 3, oam_id);   // папа
      oam_id = draw_person(168, 128, 0xAA, 0xAD, 2, oam_id);   // Инна
      oam_id = draw_person(192, 128, 0xAB, 0xAC, 2, oam_id);   // Вика
      oam_id = draw_person(100, 184, HERO_HEAD, HERO_BODY, 0, oam_id);  // Данила
      oam_id = oam_spr(120 + i, 180, T_STROL_L, 2, oam_id);    // коляска
      oam_id = oam_spr(128 + i, 180, T_STROL_R, 2, oam_id);
    }

    sfx_update();
    ppu_wait_nmi();
  }
}
