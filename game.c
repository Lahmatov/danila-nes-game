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
#define T_WALL    0xA1     // кирпич (гостиная)
#define T_FLOOR   0xA2     // фон комнаты в точку (гостиная, детская)
#define T_BOX     0xA3     // коробка (можно стоять)
#define T_CHECKER 0xE4     // сине-белый кафель (кухня): и стена, и пол
#define T_STARS   0xE5     // ночное небо в белых звёздах (улица)
// Мебель покрупнее -- каждый предмет теперь из нескольких тайлов.
#define T_SOFA_TL  0xE6    // диван 3x2 (гостиная)
#define T_SOFA_TM  0xE7
#define T_SOFA_TR  0xE8
#define T_SOFA_BL  0xE9
#define T_SOFA_BM  0xEA
#define T_SOFA_BR  0xEB
#define T_LAMP_TOP 0xEC    // торшер 1x2 (гостиная)
#define T_LAMP_BOT 0xED
#define T_CAB_TL   0xEE    // шкафчик 2x2 (кухня)
#define T_CAB_TR   0xEF
#define T_CAB_BL   0xF0
#define T_CAB_BR   0xF1
#define T_STOVE_TL 0xF2    // плита 2x2 (кухня)
#define T_STOVE_TR 0xF3
#define T_STOVE_BL 0xF4
#define T_STOVE_BR 0xF5
#define T_FENCE_T  0xF6    // забор 1x2, повторяется (улица)
#define T_FENCE_B  0xF7
#define T_MOON_TL  0xF8    // луна 2x2 (улица)
#define T_MOON_TR  0xF9
#define T_MOON_BL  0xFA
#define T_MOON_BR  0xFB
#define T_SHELF_L  0xFC    // полка 2x1 (детская)
#define T_SHELF_R  0xFD
#define T_TV_TL    0x61    // телевизор 2x2 (гостиная)
#define T_TV_TR    0x62
#define T_TV_BL    0x63
#define T_TV_BR    0x64
#define T_FRIDGE_TL 0x65   // холодильник 2x3 (кухня)
#define T_FRIDGE_TR 0x66
#define T_FRIDGE_ML 0x67
#define T_FRIDGE_MR 0x68
#define T_FRIDGE_BL 0x69
#define T_FRIDGE_BR 0x6A
#define T_LAMPPOST_TOP 0x6B  // фонарь 1x3 (улица)
#define T_LAMPPOST_MID 0x6C
#define T_LAMPPOST_BOT 0x6D
#define T_MOBILE_L 0x6E    // мобиль 2x1 (детская)
#define T_MOBILE_R 0x6F
// Дверь-выход 2x2 у правого края: закрыта, пока не собраны все вещи
// уровня, потом открывается. 4 тайла подряд: TL, TR, BL, BR.
#define T_DOOR_CLOSED 0x70
#define T_DOOR_OPEN   0x74
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

  0x27,0x17,0x30,0x00,    // фон 0: гостиная -- тёплый кирпич, тёмный, белый (текст)
  0x28,0x36,0x30,0x00,    // фон 1: портрет
  0x24,0x21,0x30,0x00,    // фон 2: детская -- розовый, мягкий синий, белый
  0x11,0x30,0x30,0x00,    // фон 3: кухня -- синий кафель, белый, белый

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

// У каждой комнаты свой облик: тайл стены, тайл фона и цветовая палитра
// (индекс фоновой палитры 0..3 из PALETTE), чтобы комнаты узнавались
// с первого взгляда, а не были одинаковым кирпичом с точками.
const unsigned char LEVEL_WALL[N_LEVELS]  = { T_WALL,  T_WALL,  T_CHECKER, T_WALL  };
const unsigned char LEVEL_FLOOR[N_LEVELS] = { T_FLOOR, T_FLOOR, T_FLOOR,   T_STARS };
const unsigned char LEVEL_BGPAL[N_LEVELS] = { 0,       2,       3,         0       };
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
//  ФОНОВАЯ МУЗЫКА: играет на треугольном канале
//  (SQ1/SQ2 заняты звуковыми эффектами выше, TRI
//  свободен -- конфликтов нет, эффекты и музыка
//  звучат одновременно).
//  Собственная весёлая тема "про цыплёнка": не
//  повторяет чужую мелодию, просто в похожем
//  озорном скачущем настроении.
// ============================================
#define TRI_LINEAR 0x4008
#define TRI_LO     0x400A
#define TRI_HI     0x400B

#define N_REST 0
#define N_C4  213
#define N_D4  190
#define N_E4  169
#define N_F4  159
#define N_G4  142
#define N_A4  126
#define N_B4  112
#define N_C5  106

#define MUSIC_LEN 30
const unsigned int MUSIC_NOTE[MUSIC_LEN] = {
  N_C4, N_C4, N_E4, N_E4, N_G4, N_G4, N_E4,               // куд-куда, куд-куда
  N_C4, N_D4, N_E4, N_F4, N_G4, N_A4, N_B4, N_C5,          // взлетаем вверх
  N_G4, N_G4, N_E4, N_E4, N_C4, N_C4, N_D4,                // куд-куда ещё раз
  N_C5, N_B4, N_A4, N_G4, N_F4, N_E4, N_D4, N_C4           // и спускаемся домой
};
const unsigned char MUSIC_DUR[MUSIC_LEN] = {
  6,6,6,6,6,6,9,
  7,7,7,7,7,7,7,9,
  6,6,6,6,6,6,9,
  7,7,7,7,7,7,7,24
};

unsigned char music_pos   = 0;
unsigned char music_timer = 0;

// Раз в кадр продвигает мелодию и переигрывает ноту на треугольнике.
void music_update(void) {
  unsigned int period;

  if (music_timer) {
    --music_timer;
    return;
  }

  period = MUSIC_NOTE[music_pos];
  POKE(TRI_LINEAR, 0xFF);
  POKE(TRI_LO, period & 0xFF);
  POKE(TRI_HI, (period >> 8) & 0x07);

  music_timer = MUSIC_DUR[music_pos];
  ++music_pos;
  if (music_pos >= MUSIC_LEN) music_pos = 0;
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
  "#.mM...........................#",
  "#.AC...........................#",
  "#.............######...........#",
  "#..............................#",
  "#..............................#",
  "#.....######..........######...#",
  "#...................abc..g.....#",
  "#..BB...............def..i......",
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
  "#..............nN..............#",
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
  "#...######...............PQ....#",
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
  "#.........######...........DE..#",
  "#..........................FG..#",
  "#..........................HI..#",
  "#....BB.............jk.pq.......",
  "#....BB.............lo.st.......",
  "################################",
  "################################",
  "################################"
};
// --- Уровень 4: УЛИЦА (ночь, звёзды) ---
const char MAP_L4[30][33] = {
  "#.........................xy...#",
  "#.........................zZ...#",
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
  "#............................J.#",
  "#.uuuu.......BB..........BB..L.#",
  "#.vvvv..B....BB.....B....BB..O.#",
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
// Стена, фон и палитра берутся из LEVEL_WALL/LEVEL_FLOOR/LEVEL_BGPAL --
// у каждой комнаты свой облик.
void draw_room(void) {
  unsigned char row, col, t;
  unsigned char buf[32];
  unsigned char wall  = LEVEL_WALL[level];
  unsigned char floor = LEVEL_FLOOR[level];
  ppu_off();
  for (row = 0; row < 30; ++row) {
    for (col = 0; col < 32; ++col) {
      t = map_at(row, col);
      if (t == '#')      buf[col] = wall;
      else if (t == 'B') buf[col] = T_BOX;
      else if (t == 'K') buf[col] = T_CRIB;
      else if (t == 'w') buf[col] = T_WINDOW;
      else if (t == 'r') buf[col] = T_RUG;
      else if (t == 'h') buf[col] = 0x2A;   // сердечко: тайл звёздочки '*'
      else if (t == 'a') buf[col] = T_SOFA_TL;
      else if (t == 'b') buf[col] = T_SOFA_TM;
      else if (t == 'c') buf[col] = T_SOFA_TR;
      else if (t == 'd') buf[col] = T_SOFA_BL;
      else if (t == 'e') buf[col] = T_SOFA_BM;
      else if (t == 'f') buf[col] = T_SOFA_BR;
      else if (t == 'g') buf[col] = T_LAMP_TOP;
      else if (t == 'i') buf[col] = T_LAMP_BOT;
      else if (t == 'j') buf[col] = T_CAB_TL;
      else if (t == 'k') buf[col] = T_CAB_TR;
      else if (t == 'l') buf[col] = T_CAB_BL;
      else if (t == 'o') buf[col] = T_CAB_BR;
      else if (t == 'p') buf[col] = T_STOVE_TL;
      else if (t == 'q') buf[col] = T_STOVE_TR;
      else if (t == 's') buf[col] = T_STOVE_BL;
      else if (t == 't') buf[col] = T_STOVE_BR;
      else if (t == 'u') buf[col] = T_FENCE_T;
      else if (t == 'v') buf[col] = T_FENCE_B;
      else if (t == 'x') buf[col] = T_MOON_TL;
      else if (t == 'y') buf[col] = T_MOON_TR;
      else if (t == 'z') buf[col] = T_MOON_BL;
      else if (t == 'Z') buf[col] = T_MOON_BR;
      else if (t == 'n') buf[col] = T_SHELF_L;
      else if (t == 'N') buf[col] = T_SHELF_R;
      else if (t == 'm') buf[col] = T_TV_TL;
      else if (t == 'M') buf[col] = T_TV_TR;
      else if (t == 'A') buf[col] = T_TV_BL;
      else if (t == 'C') buf[col] = T_TV_BR;
      else if (t == 'D') buf[col] = T_FRIDGE_TL;
      else if (t == 'E') buf[col] = T_FRIDGE_TR;
      else if (t == 'F') buf[col] = T_FRIDGE_ML;
      else if (t == 'G') buf[col] = T_FRIDGE_MR;
      else if (t == 'H') buf[col] = T_FRIDGE_BL;
      else if (t == 'I') buf[col] = T_FRIDGE_BR;
      else if (t == 'J') buf[col] = T_LAMPPOST_TOP;
      else if (t == 'L') buf[col] = T_LAMPPOST_MID;
      else if (t == 'O') buf[col] = T_LAMPPOST_BOT;
      else if (t == 'P') buf[col] = T_MOBILE_L;
      else if (t == 'Q') buf[col] = T_MOBILE_R;
      else               buf[col] = floor;
    }
    vram_adr(NTADR_A(0, row));
    vram_write(buf, 32);
  }
  // Дверь-выход у правого края (колонки 30-31, ряды 25-26): закрыта,
  // пока не собраны все вещи уровня, потом открывается -- видно, куда идти.
  {
    unsigned char d = (lvl_found >= lvl_total) ? T_DOOR_OPEN : T_DOOR_CLOSED;
    vram_adr(NTADR_A(30, 25));
    vram_put(d);
    vram_put(d + 1);
    vram_adr(NTADR_A(30, 26));
    vram_put(d + 2);
    vram_put(d + 3);
  }
  vram_adr(0x23C0);
  vram_fill(LEVEL_BGPAL[level] * 0x55, 64);
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

// ============================================
//  МИНИ-ИГРЫ: между "детской" и "кухней" (level==2).
//  Свой набор графики во втором банке тайлов (256..511,
//  см. build/minigames_chr.py) -- переключается bank_bg()/
//  bank_spr(), поэтому не мешает основной комнатной графике.
//  Цифры 0..9 продублированы в банке 2 на тех же местах, так
//  что счёт рисуется тем же приёмом oam_spr(x,y,'0'+n,...).
// ============================================
#define MG_BIKE     0
#define MG_TENNIS   1
#define MG_FOOTBALL 2
#define MG_DONE     3

unsigned char minigame_stage = MG_BIKE;
unsigned char minigames_done = 0;

// Фоновые тайлы мини-игр во втором банке. Спрайты (Даня, папа, мяч,
// коробки, цифры счёта) -- обычные, из первого банка.
#define MG_ROAD     0x04
#define MG_LANE     0x05
#define MG_NET      0x07
#define MG_GOAL_L   0x09
#define MG_GOAL_R   0x0A
#define MG_GRASS    0x0E

// Рисует экран-заглушку перед мини-игрой (обычным, первым банком).
void draw_minigame_intro(void) {
  ppu_off();
  vram_adr(NTADR_A(0, 0));
  vram_fill(' ', 960);
  if (minigame_stage == MG_BIKE) {
    vram_adr(NTADR_A(7, 8));
    put_ru("DANILA KATAETSYA NA MTB!");   // ДАНИЛА КАТАЕТСЯ НА МТБ!
    vram_adr(NTADR_A(3, 11));
    put_ru("KNOPKI VLEVO/VPRAVO --");     // КНОПКИ ВЛЕВО/ВПРАВО --
    vram_adr(NTADR_A(3, 13));
    put_ru("PEREPRYGIVAJ MEZHDU POLOS!"); // ПЕРЕПРЫГИВАЙ МЕЖДУ ПОЛОС!
  } else if (minigame_stage == MG_TENNIS) {
    vram_adr(NTADR_A(10, 8));
    put_ru("TENNIS!");                    // ТЕННИС!
    vram_adr(NTADR_A(3, 11));
    put_ru("KNOPKI VVERH/VNIZ --");       // КНОПКИ ВВЕРХ/ВНИЗ --
    vram_adr(NTADR_A(3, 13));
    put_ru("LOVI MYACH RAKETKOJ!");       // ЛОВИ МЯЧ РАКЕТКОЙ!
  } else {
    vram_adr(NTADR_A(10, 8));
    put_ru("FUTBOL!");                    // ФУТБОЛ!
    vram_adr(NTADR_A(3, 11));
    put_ru("VLEVO/VPRAVO -- CEL',");      // ВЛЕВО/ВПРАВО -- ЦЕЛЬ,
    vram_adr(NTADR_A(3, 13));
    put_ru("KNOPKA A -- UDAR!");          // КНОПКА A -- УДАР!
  }
  vram_adr(NTADR_A(9, 20));
  put_ru("NAZHMI KNOPKU A");              // НАЖМИ КНОПКУ А
  vram_adr(0x23C0);
  vram_fill(0, 64);
  ppu_on_all();
}

// ------------ ВЕЛОИГРА: 3 полосы, уклоняемся от камней ------------
#define N_OBST 3
const unsigned char BIKE_LANE_X[3] = { 56, 120, 184 };
#define BIKE_Y 192
unsigned char bike_lane;
unsigned char obst_lane[N_OBST];
int obst_y[N_OBST];
unsigned int bike_timer;
unsigned char bike_bump;

void draw_minigame_bg(unsigned char fill_tile, unsigned char lane_col) {
  unsigned char row, col;
  unsigned char buf[32];
  for (col = 0; col < 32; ++col) buf[col] = fill_tile;
  ppu_off();
  for (row = 0; row < 30; ++row) {
    vram_adr(NTADR_A(0, row));
    vram_write(buf, 32);
  }
  if (lane_col) {
    for (row = 0; row < 30; ++row) {
      vram_adr(NTADR_A(9, row));
      vram_put(MG_LANE);
      vram_adr(NTADR_A(17, row));
      vram_put(MG_LANE);
    }
  }
  vram_adr(0x23C0);
  vram_fill(0, 64);
  ppu_on_all();
}

void start_bike(void) {
  unsigned char i;
  bank_bg(1);   // спрайты остаются в обычном банке -- Даню видно как в игре
  bike_lane = 1;
  bike_timer = 600;   // 10 секунд при 60 к/с
  bike_bump = 0;
  for (i = 0; i < N_OBST; ++i) {
    obst_lane[i] = rand8() % 3;
    obst_y[i] = -((int)i * 60) - 16;
  }
  draw_minigame_bg(MG_ROAD, 1);
}

// Возвращает 1, когда мини-игра завершена.
unsigned char update_bike(unsigned char pad, unsigned char pad_t) {
  unsigned char i;

  if (pad_t & PAD_LEFT)  { if (bike_lane > 0) --bike_lane; }
  if (pad_t & PAD_RIGHT) { if (bike_lane < 2) ++bike_lane; }

  if (t & 1) {
    for (i = 0; i < N_OBST; ++i) {
      obst_y[i] += 2;
      if (obst_y[i] > 224) {
        obst_y[i] = -16;
        obst_lane[i] = rand8() % 3;
      }
    }
  }

  if (bike_bump) --bike_bump;
  for (i = 0; i < N_OBST; ++i) {
    if (!bike_bump && obst_lane[i] == bike_lane &&
        obst_y[i] + 8 > BIKE_Y && BIKE_Y + 8 > obst_y[i]) {
      bike_bump = 30;
      sfx_start(SFX_BUMP);
    }
  }

  if (bike_timer) --bike_timer;
  return bike_timer == 0;
}

void draw_bike(void) {
  unsigned char i, oam_id;
  oam_clear();
  oam_id = 0;
  if (!bike_bump || (bike_bump & 2)) {   // тот же Даня, что и в игре
    oam_id = draw_person(BIKE_LANE_X[bike_lane], BIKE_Y, HERO_HEAD, HERO_BODY, 0, oam_id);
  }
  for (i = 0; i < N_OBST; ++i) {
    if (obst_y[i] >= 0 && obst_y[i] < 224) {
      oam_id = oam_spr(BIKE_LANE_X[obst_lane[i]], (unsigned char)obst_y[i],
                        T_BOX, 2, oam_id);
    }
  }
  i = bike_timer / 60;
  oam_id = oam_spr(16, 16, '0' + i, 0, oam_id);
}

// ------------ ТЕННИС: отбивай мяч ракеткой ------------
#define TENNIS_GOAL 5
#define TENNIS_PADDLE_X 224
unsigned char tennis_paddle_y;
int tennis_ball_x, tennis_ball_y;
signed char tennis_vx, tennis_vy;
unsigned char tennis_rally;

void start_tennis(void) {
  bank_bg(1);   // спрайты остаются в обычном банке -- Даню видно как в игре
  pal_col(1, 0x11);   // фон 0 временно -- синий корт
  pal_col(2, 0x01);
  pal_col(3, 0x30);
  tennis_paddle_y = 100;
  tennis_ball_x = 40;
  tennis_ball_y = 100;
  tennis_vx = 1;
  tennis_vy = 1;
  tennis_rally = 0;
  draw_minigame_bg(MG_ROAD, 0);
  ppu_off();
  { unsigned char row;
    for (row = 0; row < 30; ++row) {
      vram_adr(NTADR_A(2, row));
      vram_put(MG_NET);
    }
  }
  ppu_on_all();
}

unsigned char update_tennis(unsigned char pad, unsigned char pad_t) {
  if (pad & PAD_UP)   { if (tennis_paddle_y > 24)  tennis_paddle_y -= 2; }
  if (pad & PAD_DOWN) { if (tennis_paddle_y < 200) tennis_paddle_y += 2; }

  tennis_ball_x += tennis_vx;
  tennis_ball_y += tennis_vy;
  if (tennis_ball_y <= 16 || tennis_ball_y >= 216) tennis_vy = -tennis_vy;
  if (tennis_ball_x <= 20) tennis_vx = 1;   // отскок от стены слева

  if (tennis_ball_x >= TENNIS_PADDLE_X - 8) {
    if (tennis_ball_y + 8 > tennis_paddle_y && tennis_paddle_y + 16 > tennis_ball_y) {
      tennis_vx = -1;
      ++tennis_rally;
      sfx_start(SFX_SELECT);
    } else if (tennis_ball_x > 250) {
      tennis_ball_x = 40;
      tennis_ball_y = 100;
      tennis_vx = 1;
    }
  }

  return tennis_rally >= TENNIS_GOAL;
}

void draw_tennis(void) {
  unsigned char oam_id;
  oam_clear();
  oam_id = 0;
  oam_id = draw_person(TENNIS_PADDLE_X, tennis_paddle_y + 8, HERO_HEAD, HERO_BODY, 0, oam_id);
  oam_id = oam_spr((unsigned char)tennis_ball_x, (unsigned char)tennis_ball_y, T_BALL, 1, oam_id);
  oam_id = oam_spr(16, 16, '0' + tennis_rally, 0, oam_id);
  oam_id = oam_spr(24, 16, '/', 0, oam_id);
  oam_id = oam_spr(32, 16, '0' + TENNIS_GOAL, 0, oam_id);
}

// ------------ ФУТБОЛ: выбери сторону и бей ------------
#define FOOT_GOAL 3
#define FOOT_MAX_ATTEMPTS 6
const unsigned char FOOT_LANE_X[3] = { 56, 120, 184 };
#define FOOT_KEEPER_Y 32
#define FOOT_BALL_Y0  192
unsigned char foot_target;
unsigned char foot_keeper_lane;
signed char foot_keeper_dir;
unsigned char foot_phase;        // 0 -- выбор цели, 1 -- удар летит, 2 -- пауза
unsigned char foot_phase_timer;
int foot_ball_y;
unsigned char foot_scored;
unsigned char foot_attempts;

void start_football(void) {
  bank_bg(1);   // спрайты остаются в обычном банке -- Даню видно как в игре
  pal_col(1, 0x1A);   // фон 0 временно -- зелёная трава
  pal_col(2, 0x0A);
  pal_col(3, 0x30);
  foot_target = 1;
  foot_keeper_lane = 0;
  foot_keeper_dir = 1;
  foot_phase = 0;
  foot_phase_timer = 0;
  foot_ball_y = FOOT_BALL_Y0;
  foot_scored = 0;
  foot_attempts = 0;
  draw_minigame_bg(MG_GRASS, 0);
  ppu_off();
  vram_adr(NTADR_A(5, 3));
  vram_put(MG_GOAL_L);
  vram_adr(NTADR_A(25, 3));
  vram_put(MG_GOAL_R);
  ppu_on_all();
}

unsigned char update_football(unsigned char pad, unsigned char pad_t) {
  if (foot_phase == 0) {
    if (pad_t & PAD_LEFT)  { if (foot_target > 0) --foot_target; }
    if (pad_t & PAD_RIGHT) { if (foot_target < 2) ++foot_target; }
    if (pad_t & PAD_A) {
      foot_phase = 1;
      foot_ball_y = FOOT_BALL_Y0;
      sfx_start(SFX_JUMP);
      return 0;   // в кадре удара мяч ещё у ноги, летит со следующего
    }
  }

  // Вратарь переходит на соседнюю полосу раз в четверть секунды --
  // достаточно медленно, чтобы шестилетка успел прицелиться в пустой угол.
  if ((t & 15) == 0) {
    if (foot_keeper_dir > 0) {
      if (foot_keeper_lane < 2) ++foot_keeper_lane; else foot_keeper_dir = -1;
    } else {
      if (foot_keeper_lane > 0) --foot_keeper_lane; else foot_keeper_dir = 1;
    }
  }

  if (foot_phase == 1) {
    foot_ball_y -= 4;
    if (foot_ball_y <= FOOT_KEEPER_Y) {
      ++foot_attempts;
      if (foot_keeper_lane != foot_target) {
        ++foot_scored;
        sfx_start(SFX_SECRET);
      } else {
        sfx_start(SFX_BUMP);
      }
      foot_phase = 2;
      foot_phase_timer = 40;
    }
  } else if (foot_phase == 2) {
    if (foot_phase_timer) --foot_phase_timer;
    else foot_phase = 0;
  }

  return foot_scored >= FOOT_GOAL || foot_attempts >= FOOT_MAX_ATTEMPTS;
}

void draw_football(void) {
  unsigned char oam_id;
  oam_clear();
  oam_id = 0;
  // Папа Женя -- вратарь, тот же спрайт, что и на заставках уровней.
  oam_id = draw_person(FOOT_LANE_X[foot_keeper_lane], FOOT_KEEPER_Y + 8, 0xA8, 0xA9, 3, oam_id);
  // Даня -- бьёт по мячу, стоит в выбранной полосе.
  oam_id = draw_person(FOOT_LANE_X[foot_target], FOOT_BALL_Y0 + 16, HERO_HEAD, HERO_BODY, 0, oam_id);
  if (foot_phase == 1) {
    oam_id = oam_spr(FOOT_LANE_X[foot_target], (unsigned char)foot_ball_y, T_BALL, 1, oam_id);
  } else {
    oam_id = oam_spr(FOOT_LANE_X[foot_target], FOOT_BALL_Y0, T_BALL, 1, oam_id);
  }
  oam_id = oam_spr(16, 16, '0' + foot_scored, 0, oam_id);
  oam_id = oam_spr(24, 16, '/', 0, oam_id);
  oam_id = oam_spr(32, 16, '0' + FOOT_GOAL, 0, oam_id);
}

// Экран "молодец!" между мини-играми -- обычным банком тайлов.
void draw_minigame_congrats(void) {
  bank_bg(0);
  bank_spr(0);
  pal_all(PALETTE);
  ppu_off();
  vram_adr(NTADR_A(0, 0));
  vram_fill(' ', 960);
  vram_adr(0x23C0);
  vram_fill(0, 64);
  ppu_on_all();
  if (minigame_stage == MG_BIKE) {
    show_window("MOLODEC!", "PROEHAL VELOTRASSU!", "EDEM DAL'WE!");
  } else if (minigame_stage == MG_TENNIS) {
    show_window("MOLODEC!", "OTLICHNAYA IGRA!", "EDEM DAL'WE!");
  } else {
    show_window("MOLODEC!", "SLAVNYJ FUTBOLIST!", "EDEM DAL'WE!");
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
    music_update();
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
        // Прыжок: -56 даёт пик ~33px -- запас в целый тайл над
        // платформами через 3 ряда (с -52 запас был всего 5px,
        // шестилетке почти не попасть).
        if (pad_t & PAD_A) { vy = -56; sfx_start(SFX_JUMP); }
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
        if (level == 2 && !minigames_done) {
          // Между "детской" и "кухней" -- три мини-игры подряд.
          minigame_stage = MG_BIKE;
          scene = 3;
          draw_minigame_intro();
        } else if (level < N_LEVELS) {
          scene = 0;
          draw_card();
        } else {
          scene = 2;
          draw_final();
        }
      }
    }
    else if (scene == 3) {
      // ===== МИНИ-ИГРА: экран-заглушка, ждём A =====
      if (pad_t & PAD_A) {
        scene = 4;
        sfx_start(SFX_SELECT);
        if (minigame_stage == MG_BIKE) start_bike();
        else if (minigame_stage == MG_TENNIS) start_tennis();
        else start_football();
      }
    }
    else if (scene == 4) {
      // ===== МИНИ-ИГРА: сама игра =====
      unsigned char mg_done = 0;
      if (minigame_stage == MG_BIKE) mg_done = update_bike(pad, pad_t);
      else if (minigame_stage == MG_TENNIS) mg_done = update_tennis(pad, pad_t);
      else mg_done = update_football(pad, pad_t);
      if (mg_done) {
        scene = 5;
        draw_minigame_congrats();
      }
    }
    else if (scene == 5) {
      // ===== МИНИ-ИГРА: "молодец!", ждём A и едем дальше =====
      if (pad_t & PAD_A) {
        if (minigame_stage < MG_FOOTBALL) {
          ++minigame_stage;
          scene = 3;
          draw_minigame_intro();
        } else {
          minigames_done = 1;
          scene = 0;
          draw_card();
        }
      }
    }
    else if (scene == 2) {
      // ===== ФИНАЛ: START -- начать заново =====
      if (pad_t & PAD_START) {
        for (i = 0; i < N_ITEMS; ++i) item_taken[i] = 0;
        for (i = 0; i < N_LEVELS; ++i) secret_taken[i] = 0;
        secret_total = 0;
        level = 0;
        minigames_done = 0;
        scene = 0;
        draw_title();
        while (1) {
          if (pad_trigger(0) & (PAD_A | PAD_START)) break;
          music_update();
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
      // Несобранные предметы переливаются двумя палитрами (рыжая <->
      // зелёно-белая), чтобы их было видно на любом тёмном фоне --
      // мишка в одной рыжей палитре терялся.
      for (i = 0; i < N_ITEMS; ++i) {
        if (!item_taken[i] && ITEM_LEVEL[i] == level) {
          oam_id = oam_spr(ITEM_X[i], ITEM_Y[i], ITEM_TILE[i],
                            (t & 16) ? 2 : 0, oam_id);
        }
      }
      if (!secret_taken[level] && (t & 8)) {   // мерцает -- всё-таки секрет
        oam_id = oam_spr(SECRET_X[level], SECRET_Y[level], T_SECRET, 2, oam_id);
      }
      // Все вещи собраны: над открытой дверью мигает звёздочка-маячок.
      if (lvl_found == lvl_total && (t & 8)) {
        oam_id = oam_spr(240, 192, T_SECRET, 0, oam_id);
      }
      oam_id = oam_spr(16, 16, '0' + lvl_found, 0, oam_id);
      oam_id = oam_spr(24, 16, '/', 0, oam_id);
      oam_id = oam_spr(32, 16, '0' + lvl_total, 0, oam_id);
    }
    else if (scene == 2) {
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
    else if (scene == 4) {
      // ===== МИНИ-ИГРА: сама отрисовка (свой банк тайлов) =====
      if (minigame_stage == MG_BIKE) draw_bike();
      else if (minigame_stage == MG_TENNIS) draw_tennis();
      else draw_football();
    }
    // scene == 3 и scene == 5 -- текстовые экраны мини-игр, спрайты не нужны

    sfx_update();
    music_update();
    ppu_wait_nmi();
  }
}
