# Agent guide

## Repository map

- `src/bootstrap.ts` — composition root;
- `src/scenes/` — Phaser runtime orchestration;
- `src/game/` — rules, commands, noise and A*;
- `src/world/` — map and doors;
- `src/entities/` — player, cores and drone AI;
- `src/input/` — keyboard, Pointer Events and VoiceOver adapters;
- `src/audio/` — Aaja adapter, acoustics, announcements and sync;
- `src/ui/` — semantic DOM and styles;
- `src/save/` — versioned local save;
- `src/testing/` — read-only test bridge;
- `assets/audio/` — source OGG, licenses and manifest;
- `public/assets/audio/` — committed production OGG;
- `vendor/aaja-package/` — generated exact Aaja package and WASM; never commit it;
- `tests/` — unit, contract and Playwright tests.

## Non-negotiable boundaries

Phaser owns timing, coordinates and collisions. Aaja owns the only production audio graph. Rules do not import Phaser/Web Audio. All input modes feed `InputHub`. Browser tests must not set coordinates, velocity, phase or victory. CI must never commit, push, listen to issues or run on a schedule. Do not reintroduce base64 bootstrap or runtime-generated production audio.

## Commands

- `npm run format:check`
- `npm run lint`
- `npm run typecheck`
- `npm run test:unit`
- `npm run audio:verify`
- `npm run repo:verify`
- `npm run build`
- `npm run test:e2e`

A change is complete only after affected playthroughs, restart/cleanup, source/license integrity and documentation have been verified.
