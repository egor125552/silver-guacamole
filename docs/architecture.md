# Architecture

## Composition

`src/bootstrap.ts` создаёт UI, save, diagnostics, три input adapter, Aaja и Phaser. `src/scenes/GameScene.ts` координирует runtime, но не содержит реализацию всех подсистем.

- `src/world/WorldMap.ts` — карта, зоны, объекты и координатные преобразования;
- `src/game/rules.ts` — framework-independent правила, фазы, нагрев, ресурсы и счёт;
- `src/game/pathfinding.ts` — детерминированный A*;
- `src/game/noise.ts` — события шума;
- `src/entities/` — игрок, ядра и AI дронов;
- `src/world/DoorSystem.ts` — двери и динамические blocker-клетки;
- `src/input/` — клавиатура, Pointer Events и VoiceOver, объединённые `InputHub`;
- `src/audio/` — Aaja adapter, акустический мир, объявления и runtime-синхронизация;
- `src/save/SaveStore.ts` — versioned save v2 с миграцией и валидацией;
- `src/ui/AppUI.ts` — семантический HTML;
- `src/testing/TestBridge.ts` — read-only snapshot, target discovery и A*-план для browser tests.

## Ownership boundaries

Phaser владеет игровым циклом, Arcade Physics, координатами, коллизиями и визуальным canvas. Aaja владеет единственным production `AudioContext`, HRTF, источниками, rooms, occlusion, categories, priorities, voice caps, ducking и lifecycle. Игровые правила не импортируют Phaser или Web Audio. Input adapter не изменяет модель напрямую: каждый создаёт `GameCommand` для общего `InputHub`.

## Test integrity

Test bridge не предоставляет setters координат, velocity, phase, `won` или save. Browser helper читает A*-маршрут, но проходит его удержаниями клавиш, кликами семантических кнопок или Pointer Events. Test-mode ускорение меняет только множитель скорости игрока в playthrough-сценарии.

## Release integrity

`vendor/aaja-package` содержит материализованный пакет точного Aaja commit и реальный WASM. `scripts/verify-release-engine.mjs` запрещает mock. `scripts/copy-release-wasm.mjs` копирует WASM рядом с Vite chunk по фактическому URL `new URL(...)`. Репозиторный verifier запрещает bootstrap, self-writing workflows, второй `AudioContext`, прямые e2e setters и незафиксированные версии.
