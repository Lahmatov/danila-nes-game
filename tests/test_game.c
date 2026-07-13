// ============================================
//  Юнит-тесты игровой логики game.c -- собираются обычным gcc
//  и гоняются на компьютере за доли секунды, без эмулятора:
//      make test
//  Приём: game.c включается прямо в этот файл, а вместо настоящей
//  neslib и регистров APU подставлены заглушки (tests/stubs/),
//  которые записывают всё в тестовые массивы. Логика (транслит,
//  коллизии, звук, музыка, мини-игры) при этом настоящая.
// ============================================
#include <stdio.h>
#include <string.h>

// game.c определяет main() с бесконечным циклом -- переименовываем,
// чтобы тесты могли объявить свой main(). game_main нигде не вызываем.
#define main game_main
#include "../game.c"
#undef main

// ---------- Заглушки железа NES ----------

unsigned char test_mem[0x10000];     // "регистры" APU для POKE()

static unsigned int  vram_cursor = 0;
static unsigned char vram_mem[0x4000];

static unsigned char test_pad = 0, test_pad_t = 0;

// Спрайты кадра: сколько и какие oam_spr() успели нарисовать.
#define MAX_OAM 64
static struct { unsigned char x, y, tile, attr; } oam_log[MAX_OAM];
static unsigned char oam_count = 0;

void pal_all(const unsigned char *data)              { (void)data; }
void pal_col(unsigned char index, unsigned char color) { (void)index; (void)color; }
void ppu_off(void)                                   {}
void ppu_on_all(void)                                {}
void ppu_wait_nmi(void)                              {}
void bank_spr(unsigned char n)                       { (void)n; }
void bank_bg(unsigned char n)                        { (void)n; }

void oam_clear(void) { oam_count = 0; }

unsigned char oam_spr(unsigned char x, unsigned char y,
                      unsigned char chrnum, unsigned char attr,
                      unsigned char sprid) {
  if (oam_count < MAX_OAM) {
    oam_log[oam_count].x = x;
    oam_log[oam_count].y = y;
    oam_log[oam_count].tile = chrnum;
    oam_log[oam_count].attr = attr;
    ++oam_count;
  }
  return sprid + 4;
}

void vram_adr(unsigned int adr) { vram_cursor = adr & 0x3FFF; }
void vram_put(unsigned char n)  { vram_mem[vram_cursor] = n; vram_cursor = (vram_cursor + 1) & 0x3FFF; }
void vram_fill(unsigned char n, unsigned int len) { while (len--) vram_put(n); }
void vram_write(const unsigned char *src, unsigned int size) { while (size--) vram_put(*src++); }

unsigned char pad_trigger(unsigned char pad) { (void)pad; return test_pad_t; }
unsigned char pad_state(unsigned char pad)   { (void)pad; return test_pad; }

static unsigned char rand_seq = 0;
unsigned char rand8(void) { return rand_seq++; }

// ---------- Мини-каркас тестов ----------

static int checks = 0, fails = 0;

#define ASSERT_EQ(what, got, want) do { \
  long g_ = (long)(got), w_ = (long)(want); \
  ++checks; \
  if (g_ != w_) { \
    ++fails; \
    printf("FAIL %s:%d %s: получили %ld, ждали %ld\n", __FILE__, __LINE__, what, g_, w_); \
  } \
} while (0)

#define ASSERT_TRUE(what, cond) ASSERT_EQ(what, !!(cond), 1)

// ---------- Тесты ----------

// Транслит -> тайлы русских букв (коды из RU_SINGLE и диграфы).
static void test_put_ru(void) {
  vram_adr(0);
  ASSERT_EQ("put_ru длина ZHENYA", put_ru("ZHENYA"), 4);
  ASSERT_EQ("Ж (ZH)", vram_mem[0], 0x87);
  ASSERT_EQ("Е",      vram_mem[1], 0x85);
  ASSERT_EQ("Н",      vram_mem[2], 0x8E);
  ASSERT_EQ("Я (YA)", vram_mem[3], 0xA0);

  vram_adr(0);
  ASSERT_EQ("put_ru длина DANILA", put_ru("DANILA"), 6);
  {
    static const unsigned char want[6] = { 0x84, 0x80, 0x8E, 0x89, 0x8C, 0x80 };
    int i;
    for (i = 0; i < 6; ++i) ASSERT_EQ("буква DANILA", vram_mem[i], want[i]);
  }

  vram_adr(0);
  ASSERT_EQ("put_ru длина спецсимволов", put_ru("W'~ 1!"), 6);
  ASSERT_EQ("Ш (W)", vram_mem[0], 0x99);
  ASSERT_EQ("Ь (')", vram_mem[1], 0x9D);
  ASSERT_EQ("Ъ (~)", vram_mem[2], 0x9B);
  ASSERT_EQ("пробел как есть", vram_mem[3], ' ');
  ASSERT_EQ("цифра как есть",  vram_mem[4], '1');
  ASSERT_EQ("! как есть",      vram_mem[5], '!');

  vram_adr(0);
  put_ru("CHYUYO");   // Ч, Ю, Ё
  ASSERT_EQ("Ч (CH)", vram_mem[0], 0x98);
  ASSERT_EQ("Ю (YU)", vram_mem[1], 0x9F);
  ASSERT_EQ("Ё (YO)", vram_mem[2], 0x86);
}

// Карта: твёрдое и проходимое.
static void test_is_wall(void) {
  level = 0;   // гостиная
  ASSERT_TRUE("рамка '#' твёрдая",         is_wall(0, 0));
  ASSERT_TRUE("пол (ряды 27+) твёрдый",    is_wall(100, 27 * 8));
  ASSERT_TRUE("коробка 'B' твёрдая",       is_wall(3 * 8, 26 * 8));
  ASSERT_EQ("воздух проходим",             is_wall(16, 16), 0);
  ASSERT_EQ("диван 'd' -- декор, проходим", is_wall(20 * 8, 26 * 8), 0);
  ASSERT_EQ("окно 'w' -- декор, проходимо", is_wall(8 * 8, 5 * 8), 0);

  level = 1;   // детская
  ASSERT_TRUE("кроватка 'K' твёрдая", is_wall(24 * 8, 25 * 8));
  ASSERT_EQ("коврик 'r' проходим",    is_wall(8 * 8, 26 * 8), 0);
  level = 0;
}

// Старт уровня: счётчик предметов по таблице ITEM_LEVEL, позиция героя.
static void test_start_level(void) {
  level = 0;
  start_level();
  ASSERT_EQ("на уровне 1 два предмета", lvl_total, 2);
  ASSERT_EQ("счёт собранного обнулён",  lvl_found, 0);
  ASSERT_EQ("герой у левого края",      hero_x, 16);
  ASSERT_EQ("мячик на старте слева",    hazard_x, HAZARD_MINX[0]);

  level = 1;
  start_level();
  ASSERT_EQ("на уровне 2 три предмета", lvl_total, 3);
  level = 0;
  start_level();
}

// Мячик-опасность патрулирует строго между MINX и MAXX.
static void test_hazard_patrol(void) {
  int i, flips = 0;
  signed char prev_dir;
  level = 0;
  start_level();
  prev_dir = hazard_dir;
  for (i = 0; i < 1000; ++i) {
    ++t;
    update_hazard();
    ASSERT_TRUE("мячик в границах полосы",
                hazard_x >= HAZARD_MINX[0] && hazard_x <= HAZARD_MAXX[0]);
    if (fails) return;   // не спамить 1000 одинаковых падений
    if (hazard_dir != prev_dir) { ++flips; prev_dir = hazard_dir; }
  }
  ASSERT_TRUE("мячик развернулся хотя бы дважды", flips >= 2);
}

// Котёнок бежит к точке за героем и разворачивается.
static void test_pet_follow(void) {
  level = 0;
  start_level();
  hero_x = 100; hero_y = 208;
  pet_x = 50; pet_y = 208; pet_vy = 0; pet_vsub = 0;
  update_pet();
  ASSERT_EQ("котёнок бежит вправо к герою", pet_x, 51);
  ASSERT_EQ("смотрит вправо", pet_facing, 0);

  pet_x = 120;
  update_pet();
  ASSERT_EQ("котёнок бежит влево к точке за героем", pet_x, 119);
  ASSERT_EQ("смотрит влево", pet_facing, 1);

  pet_x = 84;   // ровно target = hero_x - 16
  update_pet();
  ASSERT_EQ("котёнок стоит, когда добежал", pet_x, 84);
}

// Звуковой автомат: длительности и тишина в конце.
static void test_sfx(void) {
  int i;
  sfx_start(SFX_JUMP);
  ASSERT_EQ("прыжок: 7 кадров", sfx_timer, 7);
  for (i = 0; i < 7; ++i) sfx_update();
  ASSERT_EQ("после прыжка звук выключен", sfx_kind, SFX_NONE);
  ASSERT_EQ("канал 1 заглушен", test_mem[0x4000], 0xB0);
  ASSERT_EQ("канал 2 заглушен", test_mem[0x4004], 0xB0);

  sfx_start(SFX_ITEM);
  ASSERT_EQ("находка: 12 кадров", sfx_timer, 12);
  sfx_update();
  ASSERT_EQ("находка играет на канале 2", test_mem[0x4004], 0xBF);
  for (i = 0; i < 11; ++i) sfx_update();
  ASSERT_EQ("после находки тишина", sfx_kind, SFX_NONE);

  sfx_start(SFX_SECRET);
  ASSERT_EQ("секрет: 16 кадров", sfx_timer, 16);
  sfx_start(SFX_BUMP);
  ASSERT_EQ("бонк: 5 кадров", sfx_timer, 5);
  for (i = 0; i < 5; ++i) sfx_update();
  ASSERT_EQ("после бонка тишина", sfx_kind, SFX_NONE);
}

// Музыка: все 30 нот в заданном порядке и зацикливание.
static void test_music_sequence(void) {
  int i;
  music_pos = 0;
  music_timer = 0;
  for (i = 0; i < MUSIC_LEN * 2; ++i) {
    unsigned int want = MUSIC_NOTE[i % MUSIC_LEN];
    music_timer = 0;             // форсируем следующую ноту сразу
    music_update();
    ASSERT_EQ("нота: младший байт периода", test_mem[0x400A], want & 0xFF);
    ASSERT_EQ("нота: старший байт периода", test_mem[0x400B], (want >> 8) & 0x07);
    ASSERT_EQ("треугольный канал открыт",   test_mem[0x4008], 0xFF);
    if (fails) return;
    ASSERT_EQ("позиция сдвинулась по кругу", music_pos, (i + 1) % MUSIC_LEN);
  }
}

// Велогонка: полосы, таймер, столкновение и неуязвимость.
static void test_bike(void) {
  rand_seq = 0;
  start_bike();
  ASSERT_EQ("старт в средней полосе", bike_lane, 1);
  ASSERT_EQ("таймер 10 секунд", bike_timer, 600);

  t = 0;   // чётный кадр: препятствия не двигаются, удобно проверять полосы
  update_bike(0, PAD_LEFT);
  ASSERT_EQ("влево -> полоса 0", bike_lane, 0);
  update_bike(0, PAD_LEFT);
  ASSERT_EQ("за край влево не уехать", bike_lane, 0);
  update_bike(0, PAD_RIGHT);
  update_bike(0, PAD_RIGHT);
  update_bike(0, PAD_RIGHT);
  ASSERT_EQ("за край вправо не уехать", bike_lane, 2);

  bike_bump = 0;
  obst_lane[0] = bike_lane;
  obst_y[0] = BIKE_Y;
  t = 0;
  update_bike(0, 0);
  ASSERT_EQ("столкновение даёт 30 кадров неуязвимости", bike_bump, 30);
  update_bike(0, 0);
  ASSERT_EQ("повторного столкновения в неуязвимости нет", bike_bump, 29);

  bike_timer = 1;
  ASSERT_EQ("на последнем тике игра завершается", update_bike(0, 0), 1);
  ASSERT_EQ("таймер не уходит в минус", bike_timer, 0);
}

// Теннис: границы ракетки, отбитие, промах, победа.
static void test_tennis(void) {
  start_tennis();
  ASSERT_EQ("старт: счёт 0", tennis_rally, 0);

  tennis_paddle_y = 26;
  update_tennis(PAD_UP, 0);
  ASSERT_EQ("ракетка упирается в верх", tennis_paddle_y, 24);
  update_tennis(PAD_UP, 0);
  ASSERT_EQ("выше верха не уходит", tennis_paddle_y, 24);
  tennis_paddle_y = 198;
  update_tennis(PAD_DOWN, 0);
  update_tennis(PAD_DOWN, 0);
  ASSERT_EQ("ниже низа не уходит", tennis_paddle_y, 200);

  // Отбитие: мяч подлетает к Дане, выровнен по нему.
  tennis_paddle_y = 100;
  tennis_ball_x = 215; tennis_ball_y = 100;
  tennis_vx = 1; tennis_vy = 1;
  update_tennis(0, 0);
  ASSERT_EQ("мяч отбит: летит обратно", tennis_vx, -1);
  ASSERT_EQ("счёт вырос", tennis_rally, 1);

  // Промах: мяч далеко от Дани и улетел за край.
  tennis_paddle_y = 24;
  tennis_ball_x = 250; tennis_ball_y = 150;
  tennis_vx = 1;
  update_tennis(0, 0);
  ASSERT_EQ("промах: мяч подают заново", tennis_ball_x, 40);
  ASSERT_EQ("промах не убавляет счёт", tennis_rally, 1);

  tennis_rally = TENNIS_GOAL;
  ASSERT_EQ("5 отбитых -- победа", update_tennis(0, 0), 1);
}

// Футбол: цель, медленный вратарь, гол, сейв, лимит попыток.
static void test_football(void) {
  start_football();
  ASSERT_EQ("старт: цель в центре", foot_target, 1);

  t = 1;   // не кратно 16 -- вратарь в этом кадре не двигается
  update_football(0, PAD_LEFT);
  ASSERT_EQ("цель влево", foot_target, 0);
  update_football(0, PAD_LEFT);
  ASSERT_EQ("за край влево не уходит", foot_target, 0);
  update_football(0, PAD_RIGHT);
  update_football(0, PAD_RIGHT);
  update_football(0, PAD_RIGHT);
  ASSERT_EQ("за край вправо не уходит", foot_target, 2);

  // Вратарь ходит только каждый 16-й кадр.
  foot_keeper_lane = 0; foot_keeper_dir = 1;
  t = 1;
  update_football(0, 0);
  ASSERT_EQ("между тиками вратарь стоит", foot_keeper_lane, 0);
  t = 16;
  update_football(0, 0);
  ASSERT_EQ("на тике вратарь шагает", foot_keeper_lane, 1);

  // Гол: вратарь не в полосе удара.
  foot_target = 2; foot_keeper_lane = 0; foot_keeper_dir = 1;
  foot_phase = 1; foot_ball_y = FOOT_KEEPER_Y + 4;
  foot_scored = 0; foot_attempts = 0;
  t = 1;
  update_football(0, 0);
  ASSERT_EQ("гол засчитан", foot_scored, 1);
  ASSERT_EQ("попытка засчитана", foot_attempts, 1);
  ASSERT_EQ("после удара пауза", foot_phase, 2);

  // Сейв: вратарь в полосе удара.
  foot_keeper_lane = 2;
  foot_phase = 1; foot_ball_y = FOOT_KEEPER_Y + 4;
  update_football(0, 0);
  ASSERT_EQ("сейв: гола нет", foot_scored, 1);
  ASSERT_EQ("сейв: попытка потрачена", foot_attempts, 2);

  // Пауза после удара заканчивается и можно бить снова.
  foot_phase = 2; foot_phase_timer = 1;
  update_football(0, 0);
  update_football(0, 0);
  ASSERT_EQ("после паузы снова выбор цели", foot_phase, 0);

  // Кнопка A запускает удар.
  update_football(0, PAD_A);
  ASSERT_EQ("A -> мяч полетел", foot_phase, 1);
  ASSERT_EQ("мяч стартует от Дани", foot_ball_y, FOOT_BALL_Y0);

  foot_scored = FOOT_GOAL; foot_phase = 0;
  ASSERT_EQ("3 гола -- победа", update_football(0, 0), 1);
  foot_scored = 0; foot_attempts = FOOT_MAX_ATTEMPTS;
  ASSERT_EQ("6 попыток -- тоже завершение", update_football(0, 0), 1);
}

// Подбор предмета и секрета: та же формула пересечения, что в main().
static void test_overlap_formula(void) {
  // Повторяем выражение из main() дословно, с данными предмета 0.
  unsigned char i = 0;
  level = ITEM_LEVEL[0];
  hero_x = ITEM_X[0]; hero_y = ITEM_Y[0];
  ASSERT_TRUE("герой на предмете -- пересечение есть",
              hero_x + 8 > ITEM_X[i] && ITEM_X[i] + 8 > hero_x &&
              hero_y + 8 > ITEM_Y[i] && ITEM_Y[i] + 8 > hero_y);
  hero_x = ITEM_X[0] + 8;   // впритык справа -- уже не касается
  ASSERT_EQ("касание краем не засчитывается",
            hero_x + 8 > ITEM_X[i] && ITEM_X[i] + 8 > hero_x, 0);
  level = 0;
}

int main(void) {
  test_put_ru();
  test_is_wall();
  test_start_level();
  test_hazard_patrol();
  test_pet_follow();
  test_sfx();
  test_music_sequence();
  test_bike();
  test_tennis();
  test_football();
  test_overlap_formula();

  printf("%s: %d проверок, %d упало\n", fails ? "ПРОВАЛ" : "OK", checks, fails);
  return fails ? 1 : 0;
}
