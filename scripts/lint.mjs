import { readFileSync } from "node:fs";
import { execFileSync } from "node:child_process";
const files = execFileSync("find", ["src", "-name", "*.ts"], { encoding: "utf8" }).trim().split("\n").filter(Boolean);
const problems = [];
for (const file of files) {
  const text = readFileSync(file, "utf8");
  if (/console\.log\(/.test(text)) problems.push(`${file}: console.log is not allowed in production source`);
  if (/catch\s*\{\s*\}/s.test(text)) problems.push(`${file}: empty catch block`);
  if (/new AudioContext\(/.test(text)) problems.push(`${file}: only Aaja may create AudioContext`);
  if (text.split("\n").length > 550) problems.push(`${file}: file exceeds 550 lines`);
}
if (problems.length) throw new Error(problems.join("\n"));
console.log(`Static lint contract checked for ${files.length} TypeScript files.`);
