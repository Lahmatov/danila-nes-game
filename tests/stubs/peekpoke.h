// Заглушка peekpoke.h для юнит-тестов: POKE пишет не в настоящие
// регистры APU (на обычном компьютере это был бы segfault), а в
// тестовый массив test_mem -- тесты потом проверяют, что именно
// звуковой движок записал в "регистры".
#ifndef _PEEKPOKE_STUB_H
#define _PEEKPOKE_STUB_H

extern unsigned char test_mem[0x10000];

#define POKE(addr, val) (test_mem[(unsigned short)(addr)] = (unsigned char)(val))
#define PEEK(addr)      (test_mem[(unsigned short)(addr)])

#endif
