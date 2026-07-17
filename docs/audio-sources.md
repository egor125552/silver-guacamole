# Production audio sources

Production использует 31 OGG, собранный из четырёх официальных Kenney CC0 1.0-пакетов: Sci-Fi Sounds, Impact Sounds, Interface Sounds и RPG Audio. В репозитории сохранены 54 исходных OGG, лицензии пакетов, производные WAV и полный manifest.

## Покрытие

- шесть поверхностей шагов: дерево, бетон, резина, гравий, металл и решётка;
- открытие/закрытие двери, ворота, переключатель и столкновение;
- поднятие, сброс, болт, тревога, победа, поражение и перегрев;
- тележка, три типа дронов, охлаждение, ядро и лифт;
- шесть ambience по акустическим зонам.

## Воспроизводимость

`assets/audio/manifest.json` содержит для каждого набора официальный page URL, прямой URL архива, SHA-256 архива, автора, лицензию и SHA-256 license-файла. Для каждого звука указаны исходные имена и SHA-256, точная FFmpeg-цепочка, выходной SHA-256, формат, длительность и размер.

`scripts/build-audio.mjs` создаёт Ogg Vorbis, mono, 48 kHz. `scripts/verify-audio.mjs` проверяет manifest/runtime contract, все хэши, лицензии, FFmpeg-декодирование, codec/sample rate/channels, длительность, размер, тишину, integrated/mean loudness, peak/clipping, DC offset, спектр, начало/конец и loop seam.

Production не использует процедурные писки или случайно сгенерированный шум. Диагностический mock разрешён только с явным `AAJA_ALLOW_MOCK=1` и блокируется release verifier.
