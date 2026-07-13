// Обязательная для neslib переменная: смещение в буфере спрайтов (OAM).
// В библиотеке она объявлена как .importzp "_oam_off" и должна быть
// определена где-то в C-коде проекта -- в 8bitworkshop это делает сама
// платформа за кулисами, при локальной сборке cc65 это нужно руками.
#pragma bss-name (push, "ZEROPAGE")
unsigned char oam_off;
#pragma bss-name (pop)
