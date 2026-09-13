import { createHash } from "node:crypto";
import { mkdir, readFile, writeFile } from "node:fs/promises";
import { basename, resolve } from "node:path";

const websiteRoot = resolve(import.meta.dir, "..");
const repositoryRoot = resolve(websiteRoot, "..");
const snapshotRoot = resolve(
  websiteRoot,
  "src/lib/content/source-snapshots",
);
const manifestPath = resolve(snapshotRoot, "manifest.json");

const manifest = JSON.parse(await readFile(manifestPath, "utf8")) as {
  sourceCommit: string;
  files: { source: string }[];
};
const sourceCommit = manifest.sourceCommit;

await mkdir(snapshotRoot, { recursive: true });

const files = [];
for (const { source } of manifest.files) {
  // The compiler reset retired these paths; retain their recorded source revision.
  const result = Bun.spawnSync({
    cmd: ["git", "show", `${sourceCommit}:${source}`],
    cwd: repositoryRoot,
  });
  if (result.exitCode !== 0) throw new Error(result.stderr.toString().trim());
  const content = result.stdout;
  const snapshot = basename(source);
  await writeFile(resolve(snapshotRoot, snapshot), content);
  files.push({
    source,
    snapshot,
    sha256: createHash("sha256").update(content).digest("hex"),
  });
}

await writeFile(
  manifestPath,
  `${JSON.stringify(
    {
      sourceCommit,
      files,
    },
    null,
    2,
  )}\n`,
);

console.log(`Wrote ${files.length} source snapshots from ${sourceCommit}.`);
