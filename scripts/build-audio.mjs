import { spawnSync } from "node:child_process";
import { createHash } from "node:crypto";
import { chmodSync, copyFileSync, existsSync, mkdirSync, readFileSync, readdirSync, rmSync, writeFileSync } from "node:fs";
import { dirname, resolve } from "node:path";

const root = resolve(import.meta.dirname, "..");
const sourceRoot = resolve(root, "assets/audio/sources");
const downloadedRoot = resolve(root, ".audio-build/downloaded-sources");
const outDir = resolve(root, "public/assets/audio");
const manifestPath = resolve(root, "assets/audio/manifest.json");
const tempDir = resolve(root, ".audio-build");

function makeTreeWritable(path) {
  if (!existsSync(path)) return;
  chmodSync(path, 0o700);
  for (const entry of readdirSync(path, { withFileTypes: true })) {
    const child = resolve(path, entry.name);
    if (entry.isDirectory()) makeTreeWritable(child);
    else chmodSync(child, 0o600);
  }
}

function removeTree(path) {
  if (!existsSync(path)) return;
  makeTreeWritable(path);
  rmSync(path, { recursive: true, force: true, maxRetries: 3, retryDelay: 100 });
}

rmSync(outDir, { recursive: true, force: true });
mkdirSync(outDir, { recursive: true });
mkdirSync(dirname(manifestPath), { recursive: true });
removeTree(tempDir);
mkdirSync(tempDir, { recursive: true });

const packs = {
  "impact-sounds": { title: "Impact Sounds 1.0", author: "Kenney", page: "https://kenney.nl/assets/impact-sounds", archive: "https://kenney.nl/media/pages/assets/impact-sounds/87b4ddecda-1677589768/kenney_impact-sounds.zip", archiveSha256: "029d734af1582474edf3a694d1b0cebc97c1c152f2f39fa34d4c2bafc5de77f8" },
  "interface-sounds": { title: "Interface Sounds 1.0", author: "Kenney", page: "https://kenney.nl/assets/interface-sounds", archive: "https://kenney.nl/media/pages/assets/interface-sounds/fa43c1dd4d-1677589452/kenney_interface-sounds.zip", archiveSha256: "f2193d072726d6758a5f7871b2dcc54dcce0d5c35c6f0a62f92549b327c81232" },
  "rpg-audio": { title: "RPG Audio", author: "Kenney Vleugels", page: "https://kenney.nl/assets/rpg-audio", archive: "https://kenney.nl/media/pages/assets/rpg-audio/8e99002d76-1677590336/kenney_rpg-audio.zip", archiveSha256: "6dbeaf8544da958d8f2adcb4a4a4b76c1ade34a05f8ab9edccd327da7375f38b" },
  "sci-fi-sounds": { title: "Sci-Fi Sounds 1.0", author: "Kenney", page: "https://kenney.nl/assets/sci-fi-sounds", archive: "https://kenney.nl/media/pages/assets/sci-fi-sounds/6b296f9ecf-1677589334/kenney_sci-fi-sounds.zip", archiveSha256: "119340f351a5098ad814f78719438c0da355a9ce8a4c8a3af6a8d48aa3d49e04" },
};

const eventLoop = (id, pack, prefix, filter) => ({ id, pack, sources: Array.from({ length: 5 }, (_, index) => `${prefix}_${String(index).padStart(3, "0")}.ogg`), kind: "event-loop", filter });
const oneShot = (id, pack, source, filter = "volume=0.72,alimiter=limit=0.88:level=false") => ({ id, pack, sources: [source], kind: "one-shot", filter });
const loop = (id, pack, source, filter) => ({ id, pack, sources: [source], kind: "continuous-loop", filter });

const specs = [
  eventLoop("footstep-wood", "impact-sounds", "footstep_wood", "highpass=f=70,lowpass=f=7200,volume=0.70,alimiter=limit=0.86:level=false"),
  eventLoop("footstep-concrete", "impact-sounds", "footstep_concrete", "highpass=f=90,lowpass=f=6800,volume=0.72,alimiter=limit=0.86:level=false"),
  eventLoop("footstep-rubber", "impact-sounds", "footstep_carpet", "highpass=f=55,lowpass=f=3200,volume=0.78,alimiter=limit=0.84:level=false"),
  eventLoop("footstep-gravel", "impact-sounds", "footstep_snow", "highpass=f=180,treble=g=4:f=3500,volume=0.72,alimiter=limit=0.85:level=false"),
  eventLoop("footstep-metal", "impact-sounds", "impactPlate_light", "highpass=f=220,lowpass=f=7800,volume=0.58,alimiter=limit=0.84:level=false"),
  eventLoop("footstep-grating", "impact-sounds", "impactTin_medium", "highpass=f=320,treble=g=5:f=4200,volume=0.55,alimiter=limit=0.83:level=false"),
  oneShot("door-open", "sci-fi-sounds", "doorOpen_000.ogg", "highpass=f=85,volume=0.76,alimiter=limit=0.88:level=false"),
  oneShot("door-close", "sci-fi-sounds", "doorClose_000.ogg", "highpass=f=75,volume=0.78,alimiter=limit=0.88:level=false"),
  oneShot("gate", "rpg-audio", "metalLatch.ogg", "asetrate=36000,aresample=48000,lowpass=f=5200,volume=0.88,alimiter=limit=0.88:level=false"),
  oneShot("switch", "interface-sounds", "switch_004.ogg", "highpass=f=250,volume=0.70,alimiter=limit=0.84:level=false"),
  oneShot("impact", "impact-sounds", "impactMetal_heavy_000.ogg", "lowpass=f=7200,volume=0.74,alimiter=limit=0.88:level=false"),
  oneShot("pickup", "interface-sounds", "confirmation_002.ogg", "highpass=f=350,volume=0.66,alimiter=limit=0.84:level=false"),
  oneShot("drop", "impact-sounds", "impactMetal_light_000.ogg", "lowpass=f=6000,volume=0.62,alimiter=limit=0.84:level=false"),
  oneShot("bolt", "impact-sounds", "impactTin_medium_004.ogg", "highpass=f=750,volume=0.68,alimiter=limit=0.86:level=false"),
  oneShot("alarm", "interface-sounds", "error_006.ogg", "asetrate=42000,aresample=48000,highpass=f=300,volume=0.82,alimiter=limit=0.88:level=false"),
  oneShot("success", "interface-sounds", "confirmation_004.ogg", "highpass=f=280,volume=0.78,alimiter=limit=0.86:level=false"),
  oneShot("failure", "interface-sounds", "error_004.ogg", "lowpass=f=5200,volume=0.80,alimiter=limit=0.87:level=false"),
  oneShot("core-overheat", "sci-fi-sounds", "explosionCrunch_001.ogg", "lowpass=f=6500,volume=0.72,alimiter=limit=0.88:level=false"),
  loop("trolley-loop", "sci-fi-sounds", "engineCircular_002.ogg", "lowpass=f=4100,volume=0.52,alimiter=limit=0.82:level=false"),
  loop("drone-sentinel-loop", "sci-fi-sounds", "spaceEngine_001.ogg", "highpass=f=180,lowpass=f=6500,volume=0.43,alimiter=limit=0.80:level=false"),
  loop("drone-listener-loop", "sci-fi-sounds", "spaceEngineSmall_002.ogg", "highpass=f=320,lowpass=f=7200,volume=0.40,alimiter=limit=0.79:level=false"),
  loop("drone-interceptor-loop", "sci-fi-sounds", "spaceEngineLarge_003.ogg", "highpass=f=90,lowpass=f=5200,volume=0.48,alimiter=limit=0.82:level=false"),
  loop("cooling-loop", "sci-fi-sounds", "spaceEngineLow_001.ogg", "highpass=f=70,lowpass=f=1900,volume=0.34,alimiter=limit=0.76:level=false"),
  loop("core-hum-loop", "sci-fi-sounds", "forceField_003.ogg", "highpass=f=110,lowpass=f=4600,volume=0.36,alimiter=limit=0.76:level=false"),
  loop("lift-loop", "sci-fi-sounds", "thrusterFire_002.ogg", "highpass=f=80,lowpass=f=3300,volume=0.35,alimiter=limit=0.78:level=false"),
  loop("yard-ambience", "sci-fi-sounds", "spaceEngineLow_003.ogg", "highpass=f=45,lowpass=f=1500,volume=0.20,alimiter=limit=0.72:level=false"),
  loop("hangar-ambience", "sci-fi-sounds", "engineCircular_004.ogg", "highpass=f=60,lowpass=f=2600,volume=0.21,alimiter=limit=0.72:level=false"),
  loop("corridor-ambience", "sci-fi-sounds", "computerNoise_000.ogg", "highpass=f=150,lowpass=f=4200,volume=0.19,alimiter=limit=0.70:level=false"),
  loop("cooling-ambience", "sci-fi-sounds", "forceField_001.ogg", "highpass=f=80,lowpass=f=2300,volume=0.18,alimiter=limit=0.70:level=false"),
  loop("shaft-ambience", "sci-fi-sounds", "spaceEngineLarge_001.ogg", "highpass=f=35,lowpass=f=1250,volume=0.18,alimiter=limit=0.70:level=false"),
  loop("machine-ambience", "sci-fi-sounds", "computerNoise_003.ogg", "highpass=f=70,lowpass=f=3600,volume=0.22,alimiter=limit=0.72:level=false"),
];

function run(command, args, options = {}) {
  const result = spawnSync(command, args, { encoding: "utf8", maxBuffer: 32 * 1024 * 1024, ...options });
  if (result.status !== 0) throw new Error(`${command} failed: ${result.stderr || result.stdout}`);
  return result;
}
const sha256 = (path) => createHash("sha256").update(readFileSync(path)).digest("hex");
const sourcePath = (pack, name) => {
  const committed = resolve(sourceRoot, pack, name);
  if (existsSync(committed)) return committed;
  return resolve(downloadedRoot, pack, "Audio", name);
};
function ensureSources() {
  const needed = new Set(specs.flatMap((spec) => spec.sources.map((name) => `${spec.pack}/${name}`)));
  const missingPacks = new Set([...needed].filter((entry) => {
    const [pack, ...rest] = entry.split("/");
    return !existsSync(sourcePath(pack, rest.join("/")));
  }).map((entry) => entry.split("/")[0]));
  if (missingPacks.size === 0) return;
  mkdirSync(downloadedRoot, { recursive: true });
  for (const packId of missingPacks) {
    const pack = packs[packId];
    const archive = resolve(tempDir, `${packId}.zip`);
    console.log(`Downloading official ${pack.title} archive...`);
    run("curl", ["--fail", "--location", "--retry", "3", pack.archive, "-o", archive]);
    if (sha256(archive) !== pack.archiveSha256) throw new Error(`Archive SHA-256 mismatch for ${packId}`);
    const target = resolve(downloadedRoot, packId);
    removeTree(target);
    mkdirSync(target, { recursive: true });
    run("unzip", ["-q", archive, "-d", target]);
  }
  for (const entry of needed) {
    const [pack, ...rest] = entry.split("/");
    if (!existsSync(sourcePath(pack, rest.join("/")))) throw new Error(`Downloaded pack is missing ${entry}`);
  }
}
function probe(path) {
  const meta = JSON.parse(run("ffprobe", ["-v", "error", "-show_entries", "format=duration,size:stream=codec_name,sample_rate,channels", "-of", "json", path]).stdout);
  return { codec: meta.streams?.[0]?.codec_name ?? "unknown", sampleRate: Number(meta.streams?.[0]?.sample_rate ?? 0), channels: Number(meta.streams?.[0]?.channels ?? 0), duration: Number(Number(meta.format?.duration ?? 0).toFixed(4)), size: Number(meta.format?.size ?? 0) };
}

function buildEventLoop(spec, output) {
  const inputs = spec.sources.flatMap((name) => ["-i", sourcePath(spec.pack, name)]);
  const filters = spec.sources.map((_, index) => `[${index}:a]aformat=sample_rates=48000:channel_layouts=mono,${spec.filter},apad=pad_dur=0.25,atrim=duration=0.40[s${index}]`).join(";");
  const labels = spec.sources.map((_, index) => `[s${index}]`).join("");
  const base = resolve(tempDir, `${spec.id}-base.wav`);
  run("ffmpeg", ["-y", "-hide_banner", "-loglevel", "error", ...inputs, "-filter_complex", `${filters};${labels}concat=n=${spec.sources.length}:v=0:a=1[out]`, "-map", "[out]", "-c:a", "pcm_s16le", base]);
  run("ffmpeg", ["-y", "-hide_banner", "-loglevel", "error", "-stream_loop", "2", "-i", base, "-t", "4.8", "-af", "adelay=80,afade=t=in:st=0:d=0.10,afade=t=out:st=4.68:d=0.12", "-ac", "1", "-ar", "48000", "-c:a", "libvorbis", "-q:a", "5", output]);
}
function buildAsset(spec, output) {
  if (spec.kind === "event-loop") return buildEventLoop(spec, output);
  const args = ["-y", "-hide_banner", "-loglevel", "error"];
  if (spec.kind === "continuous-loop") args.push("-stream_loop", "1");
  args.push("-i", sourcePath(spec.pack, spec.sources[0]));
  if (spec.kind === "continuous-loop") args.push("-t", "4.8", "-af", `${spec.filter},afade=t=in:st=0:d=0.03,afade=t=out:st=4.70:d=0.10`);
  else args.push("-af", spec.filter);
  args.push("-ac", "1", "-ar", "48000", "-c:a", "libvorbis", "-q:a", "5", output);
  run("ffmpeg", args);
}

ensureSources();

const sources = {};
for (const [id, pack] of Object.entries(packs)) {
  const license = sourcePath(id, "License.txt");
  sources[id] = { ...pack, license: "CC0-1.0", licenseFile: `assets/audio/sources/${id}/License.txt`, licenseSha256: sha256(license) };
}
const assets = [];
for (const spec of specs) {
  for (const source of spec.sources) if (!readFileSync(sourcePath(spec.pack, source))) throw new Error(`Missing source ${spec.pack}/${source}`);
  const output = resolve(outDir, `${spec.id}.ogg`);
  console.log(`Building ${spec.id}...`);
  buildAsset(spec, output);
  assets.push({ id: spec.id, file: `public/assets/audio/${spec.id}.ogg`, sourcePack: spec.pack, sourceFiles: spec.sources.map((name) => ({ name, sha256: sha256(sourcePath(spec.pack, name)) })), author: packs[spec.pack].author, source: packs[spec.pack].page, license: "CC0-1.0", attributionRequired: false, outputSha256: sha256(output), transformations: [spec.kind, spec.filter, "mono", "48000 Hz", "Ogg Vorbis quality 5"], analysis: probe(output) });
}
const manifest = { schemaVersion: 3, engineCommit: "4ad11f77b15163699d9213bc461d8aefb37c12f7", sources, assets };
writeFileSync(manifestPath, `${JSON.stringify(manifest, null, 2)}\n`);
copyFileSync(manifestPath, resolve(outDir, "manifest.json"));
removeTree(tempDir);
console.log(`Built ${assets.length} production audio assets from ${Object.keys(sources).length} official CC0 packs.`);
