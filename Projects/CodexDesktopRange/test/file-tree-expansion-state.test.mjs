import assert from "node:assert/strict";
import { readFileSync } from "node:fs";
import test from "node:test";

const source = readFileSync(
  new URL(
    "../direct-renderer/app/webview/assets/review-file-tree-pane-Dg3636o1.js",
    import.meta.url,
  ),
  "utf8",
);

test("collapsing a parent preserves expanded descendant state", () => {
  assert.match(source, /selfPublishedExpandedPaths = \(0, X\.useRef\)\(null\)/);
  assert.match(source, /selfPublishedExpandedPaths\.current = Array\.from\(e\)/);
  assert.match(source, /Je\(selfPublishedExpandedPaths\.current, e\)/);
  assert.match(source, /t\?\.model === W/);
  assert.match(source, /t\.resetKey === S/);
  assert.match(source, /Je\(t\.treePaths, I\)/);

  const selfEchoGuard = source.slice(
    source.indexOf("selfPublishedExpandedPaths.current != null"),
    source.indexOf("selfPublishedExpandedPaths.current = null;", source.indexOf("selfPublishedExpandedPaths.current != null")) +
      "selfPublishedExpandedPaths.current = null;".length,
  );
  assert.doesNotMatch(selfEchoGuard, /W\.resetPaths/);
});

test("external tree restores still reset paths", () => {
  assert.match(source, /W\.resetPaths\(I, \{ initialExpandedPaths: u \}\)/);
});
