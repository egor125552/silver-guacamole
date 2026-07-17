import { readFileSync, statSync } from "node:fs";
import { execFileSync } from "node:child_process";
const files = execFileSync("find", ["src", "scripts", "tests", "docs", ".github", "-type", "f"], { encoding: "utf8" }).trim().split("\n").filter(Boolean);
const errors = [];
for (const file of files) {
  if (statSync(file).size > 500_000) continue;
  const text = readFileSync(file, "utf8");
  if (/\r/.test(text)) errors.push(`${file}: CRLF`);
  if (/[ \t]+$/m.test(text)) errors.push(`${file}: trailing whitespace`);
  if (!text.endsWith("\n")) errors.push(`${file}: missing final newline`);
}
if (errors.length) throw new Error(errors.join("\n"));
console.log(`Formatting contract checked for ${files.length} files.`);
