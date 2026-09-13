import assert from "node:assert/strict";
import { readFile } from "node:fs/promises";
import test from "node:test";

const conversationPagePath = new URL(
  "../direct-renderer/app/webview/assets/local-conversation-page-BFpUCld2.js",
  import.meta.url,
);
const fileTreeSidePanePath = new URL(
  "../direct-renderer/app/webview/assets/review-file-tree-side-pane-CsHY_Cjz.js",
  import.meta.url,
);
const appInitialPath = new URL(
  "../direct-renderer/app/webview/assets/app-initial-DJrCTPoN.js",
  import.meta.url,
);
const homeModeTogglePath = new URL(
  "../direct-renderer/app/webview/assets/home-composer-mode-toggle-Bf2rMA7W.js",
  import.meta.url,
);

test("Build opens the native icon file browser as a content column beside Chat", async () => {
  const source = await readFile(conversationPagePath, "utf8");
  const conversation = source.slice(source.indexOf("function yo(e)"), source.indexOf("function bo(e)"));
  const switcher = source.slice(source.indexOf("function So(e)"), source.indexOf("function Co(e)"));

  assert.match(source, /b as WorkspaceDocumentBrowser/);
  assert.match(source, /n as initWorkspaceDocumentBrowser/);
  assert.match(source, /initWorkspaceDocumentBrowser\(\)/);
  assert.match(source, /defaultRangeBuildDocumentRoots = \[`\/Users\/george`\]/);
  assert.match(source, /roots\.length === 1 && roots\[0\] === `\/Users\/george` \? `Home` : `Locations`/);
  assert.match(source, /rangeBuildDocumentRootsKey = `range\.build\.documentRoots`/);
  assert.match(source, /Document folders \(one absolute path per line\)/);
  assert.match(source, /"data-range-build-document-column": !0/);
  assert.match(source, /children: \(0, Q\.jsx\)\(WorkspaceDocumentBrowser/);
  assert.match(
    conversation,
    /buildBrowserOpen \? \(0, Q\.jsx\)\(RangeBuildDocumentColumn,[\s\S]*?"data-range-pane": `chat`/,
  );
  assert.match(switcher, /"aria-pressed": buildBrowserOpen/);
  assert.match(switcher, /onClick: selectBuild/);
  assert.match(switcher, /children: buildRunLocation === `cloud` \? `Cloud` : `Local`/);
});

test("the existing workspace browser is exported for reuse", async () => {
  const source = await readFile(fileTreeSidePanePath, "utf8");
  assert.match(source, /function er\(e\)/);
  assert.match(source, /er as b/);
  assert.match(source, /includeHidden: !0/);
});

test("Home Build exposes a Finder-style icon browser", async () => {
  const source = await readFile(fileTreeSidePanePath, "utf8");

  assert.match(source, /function RangeIconDocumentBrowser\(e\)/);
  assert.match(source, /RangeIconDocumentBrowser as d/);
  assert.match(source, /"data-range-icon-document-browser": !0/);
  assert.match(source, /display: `grid`/);
  assert.match(source, /gridTemplateColumns: `repeat\(auto-fill, minmax\(88px, 1fr\)\)`/);
  assert.match(source, /includeHidden: !1/);
  assert.match(source, /Number\(b\.isDirectory\) - Number\(a\.isDirectory\)/);
  assert.match(source, /setDirectoryPath\(entry\.path\.slice\(0, -1\)\)/);
  assert.match(source, /scope\.get\(l\)\.mutate\(\{ cwd, path: entry\.path \}\)/);
});

test("Home Build replaces Choose project with the document-browser column", async () => {
  const source = await readFile(appInitialPath, "utf8");
  const home = source.slice(source.indexOf("function ioc(e)"), source.indexOf("function aoc()"));

  assert.match(source, /RangeHomeWorkspaceDocumentBrowser = a6\.lazy/);
  assert.match(source, /module\.n\(\)/);
  assert.match(source, /return \{ default: module\.d \}/);
  assert.match(source, /"data-range-home-build-document-column": !0/);
  assert.match(source, /"data-range-home-build-layout": !0/);
  assert.match(source, /if \(a === `work`\)[\s\S]*?RangeHomeBuildDocumentColumn/);
  assert.match(home, /children: \[null, en, tn\]/);
  assert.doesNotMatch(home, /children: \[\$t, en, tn\]/);
  assert.match(
    home,
    /RangeHomeBuildDocumentColumn, \{[\s\S]*?hostId: Lt,[\s\S]*?runLocation: an,[\s\S]*?runLocationControl: cn/,
  );
  assert.match(source, /function RangeHomeRunLocationPicker/);
  assert.match(source, /"aria-label": `Build location`/);
  assert.match(source, /role: `radiogroup`/);
  assert.match(source, /"aria-checked": selected/);
  assert.match(source, /role: `radio`/);
  assert.match(source, /children: value === `local` \? `Local` : `Cloud`/);
  assert.match(home, /\(RangeHomeRunLocationPicker, \{/);
  assert.doesNotMatch(home, /\(Wao, \{/);
  assert.doesNotMatch(home, /cloudUsesLocalExecutor:/);
  assert.match(source, /runLocation === `cloud`/);
  assert.match(source, /"data-range-home-cloud-files": !0/);
  assert.match(source, /children: `Upload files`/);
  assert.match(source, /children: `Create file`/);
  assert.match(source, /rangeHomeCloudAttachmentInput\(\)\?\.click\(\)/);
  assert.match(source, /transfer\.items\.add\(new File/);
  assert.match(source, /input\.dispatchEvent\(new Event\(`change`, \{ bubbles: !0 \}\)\)/);
  assert.match(source, /"data-range-home-chat-column": !0/);
  assert.match(source, /pointer-events-none absolute inset-x-0 top-0/);
  assert.match(source, /children: a === `work` \? null : o/);
  assert.match(source, /hideRunLocationDropdownOverride: a === `work` \|\| !ge/);
  assert.match(source, /rangeHomeDocumentRootsKey = `range\.build\.documentRoots`/);
  assert.match(source, /defaultRangeHomeDocumentRoots = \[`\/Users\/george`\]/);
});

test("Home Build and Chat use a compact borderless toolbar segment", async () => {
  const source = await readFile(homeModeTogglePath, "utf8");

  assert.match(source, /inline-flex h-9 min-w-0 items-center rounded-xl/);
  assert.match(source, /inline-flex h-8 min-w-\[72px\] items-center justify-center rounded-lg/);
  assert.doesNotMatch(source, /border-heavy/);
  assert.doesNotMatch(source, /border-light/);
  assert.doesNotMatch(source, /inline-grid h-32/);
});
