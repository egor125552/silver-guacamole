import { spawnSync } from "node:child_process";
import { createHash } from "node:crypto";
import { existsSync, mkdirSync, readFileSync, rmSync, writeFileSync } from "node:fs";
import { resolve } from "node:path";

const root = resolve(import.meta.dirname, "..");
const manifestPath = resolve(root, "assets/audio/manifest.json");
const tempDir = resolve(root, ".audio-build/verify");
const requiredIds = new Set([
  "footstep-wood", "footstep-metal", "footstep-gravel", "footstep-concrete", "footstep-grating", "footstep-rubber",
  "door-open", "door-close", "gate", "switch", "impact", "pickup", "drop", "bolt", "alarm", "success", "failure", "core-overheat",
  "trolley-loop", "drone-sentinel-loop", "drone-listener-loop", "drone-interceptor-loop", "cooling-loop", "core-hum-loop", "lift-loop",
  "yard-ambience", "hangar-ambience", "corridor-ambience", "cooling-ambience", "shaft-ambience", "machine-ambience",
]);
const loopIds = new Set([...requiredIds].filter((id) => id.startsWith("footstep-") || id.endsWith("-loop") || id.endsWith("-ambience")));
if (!existsSync(manifestPath)) throw new Error("assets/audio/manifest.json is missing");
const manifest = JSON.parse(readFileSync(manifestPath, "utf8"));
if (manifest.schemaVersion !== 3 || manifest.engineCommit !== "4ad11f77b15163699d9213bc461d8aefb37c12f7") throw new Error("Unexpected manifest schema or Aaja commit");
if (!Array.isArray(manifest.assets) || manifest.assets.length !== requiredIds.size) throw new Error(`Audio manifest must contain exactly ${requiredIds.size} assets`);
if (new Set(manifest.assets.map((asset) => asset.id)).size !== requiredIds.size || manifest.assets.some((asset) => !requiredIds.has(asset.id))) throw new Error("Audio manifest IDs do not match runtime SoundName contract");
if (!manifest.sources || Object.keys(manifest.sources).length !== 4) throw new Error("Four licensed source packs are required");

const hash = (data) => createHash("sha256").update(data).digest("hex");
const run = (command, args) => {
  const result = spawnSync(command, args, { encoding: "utf8", maxBuffer: 64 * 1024 * 1024 });
  if (result.status !== 0) throw new Error(`${command} failed: ${result.stderr || result.stdout}`);
  return result.stdout;
};

for (const [sourceId, source] of Object.entries(manifest.sources)) {
  if (source.license !== "CC0-1.0" || source.author.length < 3 || !source.page.startsWith("https://kenney.nl/") || !/^[a-f0-9]{64}$/.test(source.archiveSha256)) throw new Error(`Incomplete source-pack metadata: ${sourceId}`);
  const licensePath = resolve(root, source.licenseFile);
  if (!existsSync(licensePath) || hash(readFileSync(licensePath)) !== source.licenseSha256) throw new Error(`License copy mismatch: ${sourceId}`);
}

rmSync(tempDir, { recursive: true, force: true });
mkdirSync(tempDir, { recursive: true });
const listPath = resolve(tempDir, "assets.tsv");
writeFileSync(listPath, manifest.assets.map((asset) => `${asset.id}\t${resolve(root, asset.file)}`).join("\n") + "\n");
const batchScript = String.raw`set -euo pipefail
while IFS=$'\t' read -r id file; do
  ffprobe -v error -show_entries stream=codec_name,sample_rate,channels:format=duration,size -of default=nw=1:nk=1 "$file" | paste -sd '|' - | sed "s#^#$id|#"
  ffmpeg -y -v error -i "$file" -ac 1 -ar 48000 -f s16le "$OUT/$id.s16le"
done < "$LIST"`;
const result = spawnSync("bash", ["-lc", batchScript], {
  encoding: "utf8",
  maxBuffer: 64 * 1024 * 1024,
  env: { ...process.env, LIST: listPath, OUT: tempDir },
});
if (result.status !== 0) throw new Error(`FFmpeg batch decode failed: ${result.stderr || result.stdout}`);
const metadata = new Map();
for (const line of result.stdout.trim().split("\n")) {
  const [id, codec, sampleRate, channels, duration, size] = line.split("|");
  metadata.set(id, { codec, sampleRate: Number(sampleRate), channels: Number(channels), duration: Number(duration), size: Number(size) });
}

function spectralCentroid(pcm, sampleRate) {
  const total = pcm.length / 2;
  const n = Math.min(1024, total);
  if (n < 64) return 0;
  let start = 0;
  let bestEnergy = -1;
  const step = Math.max(1, Math.floor(n / 2));
  for (let candidate = 0; candidate + n <= total; candidate += step) {
    let candidateEnergy = 0;
    for (let index = 0; index < n; index += 1) {
      const sample = pcm.readInt16LE((candidate + index) * 2) / 32768;
      candidateEnergy += sample * sample;
    }
    if (candidateEnergy > bestEnergy) { bestEnergy = candidateEnergy; start = candidate; }
  }
  let weighted = 0; let energy = 0;
  for (let bin = 1; bin <= Math.floor(n / 2); bin += 1) {
    let real = 0; let imaginary = 0;
    for (let index = 0; index < n; index += 1) {
      const sample = pcm.readInt16LE((start + index) * 2) / 32768;
      const window = 0.5 - 0.5 * Math.cos((2 * Math.PI * index) / (n - 1));
      const phase = (2 * Math.PI * bin * index) / n;
      real += sample * window * Math.cos(phase);
      imaginary -= sample * window * Math.sin(phase);
    }
    const magnitude = real * real + imaginary * imaginary;
    const frequency = (bin * sampleRate) / n;
    weighted += frequency * magnitude;
    energy += magnitude;
  }
  return energy > 0 ? weighted / energy : 0;
}

for (const asset of manifest.assets) {
  const file = resolve(root, asset.file);
  if (!existsSync(file)) throw new Error(`Missing audio asset ${asset.file}`);
  const data = readFileSync(file);
  if (hash(data) !== asset.outputSha256) throw new Error(`Output SHA-256 mismatch for ${asset.id}`);
  if (data.length > 450_000) throw new Error(`Unexpectedly large compressed asset ${asset.id}`);
  if (asset.license !== "CC0-1.0" || asset.author.length < 3 || !Array.isArray(asset.transformations) || asset.transformations.length < 4) throw new Error(`Missing license or transformations for ${asset.id}`);
  if (!Array.isArray(asset.sourceFiles) || asset.sourceFiles.length === 0 || asset.sourceFiles.some((source) => !/^[a-f0-9]{64}$/.test(source.sha256))) throw new Error(`Missing source hashes for ${asset.id}`);

  const meta = metadata.get(asset.id);
  if (!meta || meta.codec !== "vorbis" || meta.sampleRate !== 48000 || meta.channels !== 1) throw new Error(`Unsupported browser asset format ${asset.id}`);
  if (!(meta.duration > 0.08 && meta.duration <= 6) || !Number.isFinite(meta.duration)) throw new Error(`Invalid duration ${asset.id}: ${meta.duration}`);
  if (Math.abs(meta.duration - asset.analysis.duration) > 0.01 || meta.size !== asset.analysis.size) throw new Error(`Manifest analysis drift for ${asset.id}`);

  const pcm = readFileSync(resolve(tempDir, `${asset.id}.s16le`));
  if (pcm.length < 8 || pcm.length % 2 !== 0) throw new Error(`Decoded PCM is invalid in ${asset.id}`);
  const sampleCount = pcm.length / 2;
  const edge = Math.min(960, Math.floor(sampleCount / 4));
  let sum = 0; let sumSquares = 0; let peak = 0; let clipping = 0; let edgePeak = 0; let seamSum = 0;
  for (let index = 0; index < sampleCount; index += 1) {
    const value = pcm.readInt16LE(index * 2) / 32768;
    const absolute = Math.abs(value);
    sum += value; sumSquares += value * value; peak = Math.max(peak, absolute);
    if (absolute >= 0.999) clipping += 1;
    if (index < edge) {
      const last = pcm.readInt16LE((sampleCount - edge + index) * 2) / 32768;
      edgePeak = Math.max(edgePeak, absolute, Math.abs(last));
      const diff = value - last; seamSum += diff * diff;
    }
  }
  const dc = Math.abs(sum / sampleCount);
  const rms = Math.sqrt(sumSquares / sampleCount);
  const meanDb = 20 * Math.log10(Math.max(rms, Number.EPSILON));
  const peakDb = 20 * Math.log10(Math.max(peak, Number.EPSILON));
  const centroid = spectralCentroid(pcm, 48000);
  if (peakDb > 0.1 || clipping > Math.max(2, sampleCount * 0.00005)) throw new Error(`Clipping risk in ${asset.id}: ${peakDb} dBFS, ${clipping} samples`);
  if (peakDb < -48 || meanDb < -72 || rms < 0.0004) throw new Error(`Suspiciously quiet asset ${asset.id}: peak ${peakDb}, mean ${meanDb}`);
  if (dc > 0.05) throw new Error(`Excessive DC offset in ${asset.id}: ${dc}`);
  if (!Number.isFinite(centroid) || centroid < 30) throw new Error(`Suspicious spectrum in ${asset.id}: centroid ${centroid}`);
  if (!Number.isFinite(edgePeak) || edgePeak > 1.001) throw new Error(`Invalid beginning/end samples in ${asset.id}`);
  if (loopIds.has(asset.id)) {
    const seamRms = Math.sqrt(seamSum / Math.max(1, edge));
    if (seamRms > 0.14) throw new Error(`Loop seam is too discontinuous in ${asset.id}: ${seamRms}`);
  }
}
console.log(`Verified ${manifest.assets.length} compressed audio assets: licenses, source/output hashes, FFmpeg decode, Vorbis format, duration, size, silence, loudness, peaks, clipping, DC, spectrum, boundaries and loop seams.`);
