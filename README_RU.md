# Charge Guard

Sysmodule и оверлей для Nintendo Switch (Atmosphère), ограничивающие максимальный уровень заряда батареи.

## Возможности

- Ограничение заряда: 80%, 85%, 90%, 95%, 100%
- Настройка через оверлей (Ultrahand / Tesla)
- Автоматический контроль в фоне (sysmodule)
- Автозапуск при загрузке консоли

## Установка

1. Собери проект:

```
bash build_all.sh
```

2. Скопируй содержимое `out/` в корень SD-карты:

```
SD:/
├── atmosphere/contents/010000000000BC01/
│   ├── exefs.nsp
│   └── flags/boot2.flag
├── config/charge-guard/
└── switch/.overlays/ChargeGuard.ovl
```

3. Перезагрузи консоль.

## Использование

- Открой меню оверлея
- Открой Charge Guard
- Выбери нужный лимит скроллом
- Sysmodule автоматически применяет ограничение в фоне

## Структура проекта

```
├── build_all.sh              Сборка проекта
├── overlay/
│   ├── Makefile              Сборка оверлея
│   ├── source/main.cpp       Интерфейс оверлея
│   └── libs/libultrahand/    Tesla / Ultrahand фреймворк
├── sysmodule/
│   ├── Makefile              Сборка sysmodule
│   ├── source/main.c         Фоновый процесс
│   └── sysmodule.json        NPDM конфигурация
```

## Требования

- devkitPro (devkitA64 + libnx)
- Atmosphère CFW на Switch
- Ultrahand Overlay или Tesla Menu

## Title ID

Sysmodule: `010000000000BC01`

При конфликте с другим sysmodule измени в `sysmodule/sysmodule.json`.