# Verification record

## Восстановление

Восемь base64-чанков были объединены и распакованы в отдельном read-only GitHub Actions artifact. Внутри находились 53 файла TypeScript/Vite/Phaser-проекта, тесты и документация, но production-аудио и реальный Aaja/WASM отсутствовали; `materialize.yml` должен был сгенерировать их, создать commit и push. Эта схема признана неприемлемой и не входит в финальное дерево.

## Локально подтверждено

- `npm ci` с точными версиями;
- format, lint и TypeScript;
- 14 unit/contract-тестов, включая A*, четыре доставки, save и release contracts;
- 31/31 production OGG: source/license/output SHA-256, decode, format, duration, size, silence, loudness, peaks, clipping, DC, spectrum, boundaries и loop seams;
- repository verifier: нет bootstrap, self-writing CI, второго `AudioContext`, mock release или direct-state e2e hooks;
- production Vite build с реальным Aaja package;
- `dist/assets/audio_game_core_bg.wasm`, 115669 байт, по фактически запрашиваемому browser URL.

Закреплённый Aaja commit отдельно прошёл Rust tests, TypeScript tests, production build и WASM verification в read-only GitHub Actions job.

## Исправленные первопричины

- прямолинейное движение дронов заменено A* и состояниями расследования/поиска/возврата;
- browser helper больше не задаёт velocity/coordinates или победу напрямую;
- Aaja JS ожидал WASM рядом с Vite chunk, но Vite его не копировал — добавлен проверяемый postbuild copy;
- stationarity ядра не обновлялась после pickup/drop/delivery — жизненный цикл loop синхронизирован с моделью;
- старый materialization CI переписывал ветку — заменён read-only CI;
- аудиопроверка трижды полностью декодировала каждый файл и была непрактичной — один полный FFmpeg pass объединён с PCM-анализом без ослабления критериев.

## Ожидает внешней проверки

Локальный Chromium заблокирован политикой среды (`ERR_BLOCKED_BY_ADMINISTRATOR`). Полные Chromium-прохождения, Firefox/WebKit smoke, mobile WebKit gestures, merge, Pages и live smoke должны быть подтверждены после загрузки production-ветки. Физический iPhone VoiceOver и субъективный headphone/HRTF pass автоматизацией не подменяются.
