// ============================================
//  ДЕНЬ 10: ВТОРОЙ АКТ -- ПРОБЕЖКА С ПРЫЖКАМИ
//  Новое: сцены (дом / улица / финал), гравитация
//  и прыжок, финальный экран с семьёй и рестарт
// ============================================

#include "neslib.h"        // библиотека NES: спрайты, палитры, геймпад

//#link "chr_ru.s"         // наш набор тайлов: буквы + вся графика игры

// --- Номера наших тайлов ---
#define T_WALL    0xA1     // кирпичная стена
#define T_FLOOR   0xA2     // пол (на улице -- звёздное небо)
#define T_BOX     0xA3     // коробка, через которую прыгаем
#define HERO_HEAD 0xA4
#define HERO_BODY 0xA5
#define T_BABY    0xB3     // малышка-свёрток
#define PORTRAIT  0xC0     // портрет 6x6 тайлов

// --- Палитра ---
const unsigned char PALETTE[32] = {
  0x0F,                   // общий цвет фона: чёрный

  0x27,0x17,0x30,0x00,    // фон 0: кирпич, тёмный шов, белый (текст)
  0x28,0x36,0x30,0x00,    // фон 1: портрет -- волосы, кожа, белый
  0x00,0x10,0x20,0x00,    // фон 2
  0x06,0x16,0x26,0x00,    // фон 3

  0x2A,0x36,0x30,0x00,    // спрайты 0: Данила
  0x25,0x36,0x17,0x00,    // спрайты 1: мама (и малышка)
  0x16,0x36,0x2D,0x00,    // спрайты 2: бабушки и предметы
  0x21,0x36,0x0F          // спрайты 3: папа
};

// ============================================
//  СЦЕНЫ
//  0 -- дом (собираем вещи), 1 -- улица (бежим
//  и прыгаем), 2 -- финал (семья в сборе).
//  Тот же приём, что talking, только для всей игры.
// ============================================
unsigned char scene = 0;

// ============================================
//  ТАБЛИЦА ПЕРСОНАЖЕЙ
// ============================================
#define N_NPC 4

const unsigned char NPC_ROOM[N_NPC] = { 0,    0,    1,    1    };
const unsigned char NPC_X[N_NPC]    = { 40,   200,  200,  60   };
const unsigned char NPC_Y[N_NPC]    = { 48,   170,  60,   150  };
const unsigned char NPC_HEAD[N_NPC] = { 0xA6, 0xA8, 0xAA, 0xAB };
const unsigned char NPC_BODY[N_NPC] = { 0xA7, 0xA9, 0xAC, 0xAC };
const unsigned char NPC_PAL[N_NPC]  = { 1,    3,    2,    2    };

const char* NPC_NAME[N_NPC] = {
  "MAMA KARINA",    // МАМА КАРИНА
  "PAPA ZHENYA",    // ПАПА ЖЕНЯ
  "BABUWKA INNA",   // БАБУШКА ИННА
  "BABUWKA VIKA"    // БАБУШКА ВИКА
};

// Диалоги: [персонаж][стадия][строка]
const char* NPC_TEXT[N_NPC][3][2] = {
  { // --- мама Карина ---
    { "PRIVET, SYNOK!",             // ПРИВЕТ, СЫНОК!
      "SOBERI VEQI DLYA MALYWKI!" },// СОБЕРИ ВЕЩИ ДЛЯ МАЛЫШКИ!
    { "KAK ZDOROVO POLUCHAETSYA!",  // КАК ЗДОРОВО ПОЛУЧАЕТСЯ!
      "IQI OSTAL'NYE VEQI!" },      // ИЩИ ОСТАЛЬНЫЕ ВЕЩИ!
    { "SESTRYONKU UZHE VEZUT!",     // СЕСТРЁНКУ УЖЕ ВЕЗУТ!
      "BEGI VSTRECHAT'!" }          // БЕГИ ВСТРЕЧАТЬ!  <- запускает пробежку
  },
  { // --- папа Женя ---
    { "PRIVET! YA VEWAYU POLKU.",   // ПРИВЕТ! Я ВЕШАЮ ПОЛКУ.
      "POMOGI MAME, HOROWO?" },     // ПОМОГИ МАМЕ, ХОРОШО?
    { "UZHE KOE-CHTO NAWYOL!",      // УЖЕ КОЕ-ЧТО НАШЁЛ!
      "TAK DERZHAT'!" },            // ТАК ДЕРЖАТЬ!
    { "VSYO GOTOVO? ZDOROVO!",      // ВСЁ ГОТОВО? ЗДОРОВО!
      "GORZHUS' TOBOJ, SYN!" }      // ГОРЖУСЬ ТОБОЙ, СЫН!
  },
  { // --- бабушка Инна ---
    { "TUT BUDET ZHIT' MALYWKA.",   // ТУТ БУДЕТ ЖИТЬ МАЛЫШКА.
      "A YA SVYAZHU EJ NOSOCHKI." },// А Я СВЯЖУ ЕЙ НОСОЧКИ.
    { "IQEW' VEQI? UMNICA!",        // ИЩЕШЬ ВЕЩИ? УМНИЦА!
      "BABUWKA V TEBYA VERIT!" },   // БАБУШКА В ТЕБЯ ВЕРИТ!
    { "NU I BYSTRYJ ZHE TY!",       // НУ И БЫСТРЫЙ ЖЕ ТЫ!
      "VSYA SEM'YA GORDITSYA!" }    // ВСЯ СЕМЬЯ ГОРДИТСЯ!
  },
  { // --- бабушка Вика ---
    { "PRIVEZU MALYWKE PODAROK!",   // ПРИВЕЗУ МАЛЫШКЕ ПОДАРОК!
      "A TY POKA POMOGAJ MAME." },  // А ТЫ ПОКА ПОМОГАЙ МАМЕ.
    { "VOT XTO PRAVIL'NO!",         // ВОТ ЭТО ПРАВИЛЬНО!
      "NASTOYAQIJ POMOQNIK!" },     // НАСТОЯЩИЙ ПОМОЩНИК!
    { "VSYO SOBRAL? VOT MOLODEC!",  // ВСЁ СОБРАЛ? ВОТ МОЛОДЕЦ!
      "ZHDYOM SESTRYONKU VMESTE!" } // ЖДЁМ СЕСТРЁНКУ ВМЕСТЕ!
  }
};

// ============================================
//  ТАБЛИЦА ПРЕДМЕТОВ
// ============================================
#define N_ITEMS 3

const unsigned char ITEM_ROOM[N_ITEMS] = { 0,    1,    1    };
const unsigned char ITEM_X[N_ITEMS]    = { 200,  60,   180  };
const unsigned char ITEM_Y[N_ITEMS]    = { 60,   56,   180  };
const unsigned char ITEM_TILE[N_ITEMS] = { 0xB0, 0xB1, 0xB2 };
const char* ITEM_NAME[N_ITEMS] = {
  "POGREMUWKU!",   // ПОГРЕМУШКУ!
  "ODEYAL'CE!",    // ОДЕЯЛЬЦЕ!
  "MIWKU!"         // МИШКУ!
};

// Память игры.
unsigned char item_taken[N_ITEMS] = { 0, 0, 0 };
unsigned char items_count = 0;
unsigned char go_run = 0;   // мама сказала бежать: после закрытия окна стартует улица

// --- Комната A: гостиная ---
const char MAP_A[30][33] = {
  "################################",
  "#..............................#",
  "#..####........................#",
  "#..####........................#",
  "#..............................#",
  "#..............................#",
  "#..............................#",
  "#..............................#",
  "#..............................#",
  "#..............................#",
  "#..............................#",
  "#..............................#",
  "#............####..............#",
  "#............####...............",
  "#...............................",
  "#...............................",
  "#..............................#",
  "#..............................#",
  "#..............................#",
  "#..................#...........#",
  "#..................#...........#",
  "#..................#...........#",
  "#..................#...........#",
  "#..................#...........#",
  "#..............................#",
  "#..............................#",
  "#..............................#",
  "#..............................#",
  "#..............................#",
  "################################"
};

// --- Комната B: детская ---
const char MAP_B[30][33] = {
  "################################",
  "#..............................#",
  "#..............................#",
  "#.......................####...#",
  "#.......................####...#",
  "#..............................#",
  "#..............................#",
  "#..............................#",
  "#..............................#",
  "#..............................#",
  "#..............................#",
  "#..............................#",
  "#..............................#",
  "...............................#",
  "...............................#",
  "...............................#",
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
  "################################"
};

// --- Улица: вид СБОКУ. 'B' -- коробки, через них прыгаем.
// Слева стена дома, внизу земля, точки пола стали звёздами.
const char MAP_RUN[30][33] = {
  "#...............................",
  "#...............................",
  "#...............................",
  "#...............................",
  "#...............................",
  "#...............................",
  "#...............................",
  "#...............................",
  "#...............................",
  "#...............................",
  "#...............................",
  "#...............................",
  "#...............................",
  "#...............................",
  "#...............................",
  "#...............................",
  "#...............................",
  "#...............................",
  "#...............................",
  "#...............................",
  "#...............................",
  "#...............................",
  "#...............................",
  "#...............................",
  "#...............................",
  "#............BB..........BB.....",
  "#.......B....BB.....B....BB.....",
  "################################",
  "################################",
  "################################"
};

// --- В какой комнате мы (для сцены 0): 0 = гостиная, 1 = детская ---
unsigned char room = 0;

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

// Какой символ карты в клетке (row, col) -- с учётом сцены.
unsigned char map_at(unsigned char r, unsigned char c) {
  if (scene == 1) return MAP_RUN[r][c];
  if (room == 0)  return MAP_A[r][c];
  return MAP_B[r][c];
}

// Стена -- это '#' ИЛИ коробка 'B'.
unsigned char is_wall(unsigned char px, unsigned char py) {
  unsigned char t = map_at(py / 8, px / 8);
  return t == '#' || t == 'B';
}

// Рисует текущий экран: '#' -> кирпич, 'B' -> коробка, '.' -> пол.
void draw_room(void) {
  unsigned char row, col, t;
  unsigned char buf[32];
  ppu_off();
  for (row = 0; row < 30; ++row) {
    for (col = 0; col < 32; ++col) {
      t = map_at(row, col);
      if (t == '#')      buf[col] = T_WALL;
      else if (t == 'B') buf[col] = T_BOX;
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

// Окно диалога: имя говорящего + две строки текста.
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

// --- Финальный экран ---
void draw_final(void) {
  ppu_off();
  vram_adr(NTADR_A(0, 0));
  vram_fill(' ', 960);
  vram_adr(NTADR_A(6, 6));
  put_ru("URA! SESTRYONKA DOMA!");        // УРА! СЕСТРЁНКА ДОМА!
  vram_adr(NTADR_A(8, 10));
  put_ru("SPASIBO, DANILA!");             // СПАСИБО, ДАНИЛА!
  vram_adr(NTADR_A(4, 12));
  put_ru("TY LUCHWIJ STARWIJ BRAT!");     // ТЫ ЛУЧШИЙ СТАРШИЙ БРАТ!
  vram_adr(NTADR_A(6, 26));
  put_ru("NAZHMI START - ZANOVO");        // НАЖМИ СТАРТ - ЗАНОВО
  vram_adr(0x23C0);
  vram_fill(0, 64);
  ppu_on_all();
}

// Персонаж = два спрайта: голова над туловищем.
unsigned char draw_person(unsigned char px, unsigned char py,
                          unsigned char head, unsigned char body,
                          unsigned char pal, unsigned char id) {
  id = oam_spr(px, py - 8, head, pal, id);
  return oam_spr(px, py, body, pal, id);
}

void main(void) {
  unsigned char x = 124;
  unsigned char y = 116;
  unsigned char pad;
  unsigned char pad_t;
  unsigned char talking = 0;
  unsigned char i;
  unsigned char stage;
  unsigned char oam_id;
  unsigned char on_ground;
  int vy = 0;     // скорость падения/взлёта: в 16-х долях пикселя за кадр
  int vsub = 0;   // копилка накопленных долей

  pal_all(PALETTE);
  bank_spr(0);

  // Титульный экран: ждём A или START.
  draw_title();
  while (1) {
    if (pad_trigger(0) & (PAD_A | PAD_START)) break;
    ppu_wait_nmi();
  }
  draw_room();

  while (1) {
    pad_t = pad_trigger(0);
    pad   = pad_state(0);

    if (talking) {
      // Окно открыто: ждём A.
      if (pad_t & PAD_A) {
        talking = 0;
        if (go_run) {
          // Мама сказала бежать: переносимся на улицу.
          go_run = 0;
          scene = 1;
          x = 16;
          y = 208;
          vy = 0;
          vsub = 0;
          draw_room();
          talking = 1;   // и сразу объясняем правила
          show_window("SKOREE!",                  // СКОРЕЕ!
                      "SESTRYONKA UZHE EDET!",    // СЕСТРЁНКА УЖЕ ЕДЕТ!
                      "PRYGAJ CHEREZ KOROBKI!");  // ПРЫГАЙ ЧЕРЕЗ КОРОБКИ!
        } else {
          draw_room();
        }
      }
    }
    else if (scene == 0) {
      // ===== ДОМ: вид сверху, как раньше =====
      if ((pad & PAD_LEFT)  && !is_wall(x - 1, y) && !is_wall(x - 1, y + 7)) x -= 1;
      if ((pad & PAD_RIGHT) && !is_wall(x + 8, y) && !is_wall(x + 8, y + 7)) x += 1;
      if ((pad & PAD_UP)    && !is_wall(x, y - 1) && !is_wall(x + 7, y - 1)) y -= 1;
      if ((pad & PAD_DOWN)  && !is_wall(x, y + 8) && !is_wall(x + 7, y + 8)) y += 1;

      // Подбор предметов
      for (i = 0; i < N_ITEMS; ++i) {
        if (!item_taken[i] && ITEM_ROOM[i] == room &&
            x + 8 > ITEM_X[i] && ITEM_X[i] + 8 > x &&
            y + 8 > ITEM_Y[i] && ITEM_Y[i] + 8 > y) {
          item_taken[i] = 1;
          ++items_count;
          talking = 1;
          show_window("NAHODKA!", "TY NAWYOL:", ITEM_NAME[i]);
        }
      }

      // Разговоры
      if (items_count == 0)            stage = 0;
      else if (items_count < N_ITEMS)  stage = 1;
      else                             stage = 2;

      if (pad_t & PAD_A) {
        for (i = 0; i < N_NPC; ++i) {
          if (NPC_ROOM[i] == room &&
              x + 20 > NPC_X[i] && NPC_X[i] + 20 > x &&
              y + 20 > NPC_Y[i] && NPC_Y[i] + 20 > y) {
            talking = 1;
            show_window(NPC_NAME[i], NPC_TEXT[i][stage][0], NPC_TEXT[i][stage][1]);
            // Финальная реплика мамы запускает пробежку.
            if (i == 0 && stage == 2) go_run = 1;
          }
        }
      }

      // Двери
      if (room == 0 && x >= 244) {
        room = 1;
        x = 12;
        draw_room();
      }
      else if (room == 1 && x <= 4) {
        room = 0;
        x = 236;
        draw_room();
      }
    }
    else if (scene == 1) {
      // ===== УЛИЦА: вид сбоку, гравитация и прыжок =====
      if ((pad & PAD_LEFT)  && !is_wall(x - 1, y) && !is_wall(x - 1, y + 7)) x -= 1;
      if ((pad & PAD_RIGHT) && !is_wall(x + 8, y) && !is_wall(x + 8, y + 7)) x += 1;

      // Стоим ли на чём-то? Проверяем клетки прямо под ногами.
      on_ground = is_wall(x, y + 8) || is_wall(x + 7, y + 8);

      if (on_ground) {
        if (vy > 0) { vy = 0; vsub = 0; }    // приземлились
        if (pad_t & PAD_A) vy = -52;         // прыжок: толчок вверх
      } else {
        vy += 3;                             // гравитация тянет вниз
        if (vy > 64) vy = 64;                // но не быстрее 4 пикс/кадр
      }

      // Скорость в 16-х долях: копим в vsub и двигаемся по
      // одному пикселю, каждый раз проверяя стены.
      vsub += vy;
      while (vsub <= -16) {                  // летим вверх
        vsub += 16;
        if (!is_wall(x, y - 1) && !is_wall(x + 7, y - 1)) --y;
        else { vy = 0; vsub = 0; }           // стукнулись головой
      }
      while (vsub >= 16) {                   // падаем вниз
        vsub -= 16;
        if (!is_wall(x, y + 8) && !is_wall(x + 7, y + 8)) ++y;
        else { vsub = 0; }                   // встали на землю
      }

      // Добежал до правого края -- встретил!
      if (x >= 240) {
        scene = 2;
        draw_final();
      }
    }
    else {
      // ===== ФИНАЛ: семья в сборе, START -- начать заново =====
      if (pad_t & PAD_START) {
        for (i = 0; i < N_ITEMS; ++i) item_taken[i] = 0;
        items_count = 0;
        room = 0;
        scene = 0;
        x = 124;
        y = 116;
        draw_title();
        while (1) {
          if (pad_trigger(0) & (PAD_A | PAD_START)) break;
          ppu_wait_nmi();
        }
        draw_room();
      }
    }

    // --- Спрайты кадра ---
    oam_clear();
    oam_id = 0;
    if (scene == 2) {
      // Вся семья в ряд, у мамы на руках -- малышка.
      oam_id = draw_person(84, 160, 0xA8, 0xA9, 3, oam_id);              // папа
      oam_id = draw_person(106, 160, 0xA6, 0xA7, 1, oam_id);             // мама
      oam_id = oam_spr(118, 156, T_BABY, 1, oam_id);                     // сестрёнка
      oam_id = draw_person(140, 160, HERO_HEAD, HERO_BODY, 0, oam_id);   // Данила
      oam_id = draw_person(162, 160, 0xAA, 0xAC, 2, oam_id);             // Инна
      oam_id = draw_person(184, 160, 0xAB, 0xAC, 2, oam_id);             // Вика
    } else {
      oam_id = draw_person(x, y, HERO_HEAD, HERO_BODY, 0, oam_id);       // Данила
      if (scene == 0) {
        for (i = 0; i < N_NPC; ++i) {
          if (NPC_ROOM[i] == room) {
            oam_id = draw_person(NPC_X[i], NPC_Y[i], NPC_HEAD[i], NPC_BODY[i], NPC_PAL[i], oam_id);
          }
        }
        for (i = 0; i < N_ITEMS; ++i) {
          if (!item_taken[i] && ITEM_ROOM[i] == room) {
            oam_id = oam_spr(ITEM_X[i], ITEM_Y[i], ITEM_TILE[i], 2, oam_id);
          }
        }
        oam_id = oam_spr(16, 16, '0' + items_count, 0, oam_id);
        oam_id = oam_spr(24, 16, '/', 0, oam_id);
        oam_id = oam_spr(32, 16, '0' + N_ITEMS, 0, oam_id);
      }
    }

    ppu_wait_nmi();
  }
}
