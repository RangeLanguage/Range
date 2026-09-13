import { afterAll, beforeAll, describe, expect, test } from "bun:test";
import { createHash } from "node:crypto";
import { readFile } from "node:fs/promises";
import { resolve } from "node:path";
import { allPosts } from "../src/lib/posts";
import {
  homepageDescription,
  homepageTitle,
  indexableSeoPages,
  repositoryUrl,
} from "../src/lib/seo";

const expectedPaths = [
  "/",
  "/benchmarks",
  "/benchmarks/constructs",
  "/benchmarks/constructs_deep_identity",
  "/benchmarks/constructs_shared_binding_mutation",
  "/benchmarks/constructs_state_replacement",
  "/posts/intro-to-range",
  "/features/macros/command-group-registration",
  "/features/macros/50-declarative-50-imperative",
  "/features/macros/somewhere-sometime-some-here",
  "/features/macros/codability-under-100",
];

const port = 45_000 + (process.pid % 1_000);
const origin = `http://127.0.0.1:${port}`;
let server: ReturnType<typeof Bun.spawn>;

beforeAll(async () => {
  server = Bun.spawn([process.execPath, "build/index.js"], {
    cwd: new URL("..", import.meta.url).pathname,
    env: { ...process.env, HOST: "127.0.0.1", PORT: String(port) },
    stdout: "ignore",
    stderr: "pipe",
  });

  for (let attempt = 0; attempt < 50; attempt += 1) {
    try {
      const response = await fetch(origin);
      if (response.ok) return;
    } catch {
      await Bun.sleep(40);
    }
  }

  throw new Error("SvelteKit SEO test server did not start in time");
});

afterAll(() => server?.kill());

function request(path: string) {
  return fetch(`${origin}${path}`, {
    headers: { accept: "text/html" },
    redirect: "manual",
  });
}

describe("search discovery contract", () => {
  test("publishes exactly eleven canonical indexable pages", () => {
    expect(indexableSeoPages.map((page) => page.path)).toEqual(expectedPaths);
    expect(new Set(indexableSeoPages.map((page) => page.title)).size).toBe(11);
    expect(new Set(indexableSeoPages.map((page) => page.description)).size).toBe(11);
    for (const page of indexableSeoPages) {
      expect(page.indexable).toBe(true);
      expect(page.canonicalUrl).toBe(`https://rangelang.org${page.path}`);
      expect(page.canonicalUrl).not.toContain("?");
      expect(page.structuredData).toBeDefined();
    }
  });

  test("SSR-renders complete unique metadata and parseable structured data for every public page", async () => {
    for (const page of indexableSeoPages) {
      const response = await request(page.path);
      const html = await response.text();
      const jsonLd = html.match(
        /<script type="application\/ld\+json">(.*?)<\/script>/,
      )?.[1];

      expect(response.status).toBe(200);
      expect(html).toContain(`<title>${page.title}</title>`);
      expect(html).toContain(`name="description" content="${page.description}"`);
      expect(html).toContain(`rel="canonical" href="${page.canonicalUrl}"`);
      expect(html).toContain(`property="og:url" content="${page.canonicalUrl}"`);
      expect(html).toContain(`property="og:image" content="${page.image}"`);
      expect(html).toContain(`name="twitter:image" content="${page.image}"`);
      expect(jsonLd).toBeDefined();
      expect(() => JSON.parse(jsonLd!)).not.toThrow();
    }
  });

  test("renders homepage search metadata and machine-readable language identity", async () => {
    const response = await request("/");
    const html = await response.text();
    const jsonLd = html.match(
      /<script type="application\/ld\+json">(.*?)<\/script>/,
    )?.[1];

    expect(response.status).toBe(200);
    expect(html).toContain(`<title>${homepageTitle}</title>`);
    expect(html).toContain(`name="description" content="${homepageDescription}"`);
    expect(html).toContain('rel="canonical" href="https://rangelang.org/"');
    expect(html).toContain('property="og:url" content="https://rangelang.org/"');
    expect(html).toContain(repositoryUrl);
    expect(jsonLd).toBeDefined();
    const graph = JSON.parse(jsonLd!);
    expect(graph.map((entry: any) => entry["@type"])).toEqual([
      "WebSite",
      "ComputerLanguage",
    ]);
  });

  test("canonicalizes benchmark filters and renders unique route metadata", async () => {
    const [indexResponse, detailResponse, articleResponse] = await Promise.all([
      request("/benchmarks?category=constructs"),
      request("/benchmarks/constructs"),
      request("/posts/intro-to-range"),
    ]);
    const [indexHtml, detailHtml, articleHtml] = await Promise.all([
      indexResponse.text(),
      detailResponse.text(),
      articleResponse.text(),
    ]);

    expect(indexResponse.status).toBe(200);
    expect(indexHtml).toContain(
      'rel="canonical" href="https://rangelang.org/benchmarks"',
    );
    expect(indexHtml).not.toContain(
      'rel="canonical" href="https://rangelang.org/benchmarks?category=',
    );
    expect(detailHtml).toContain(
      "<title>Identity construct versus inline pair Benchmark — Range Programming Language</title>",
    );
    expect(detailHtml).toContain('"@type":"Dataset"');
    expect(articleHtml).toContain(
      "<title>Introduction to Range — Range Programming Language</title>",
    );
    expect(articleResponse.status).toBe(200);
    expect(articleResponse.headers.get("x-robots-tag")).toBeNull();
    expect(articleHtml).toContain('"@type":"BlogPosting"');
  });

  test("serves a discoverable robots file and an exact XML sitemap", async () => {
    const [robotsResponse, sitemapResponse] = await Promise.all([
      request("/robots.txt"),
      request("/sitemap.xml"),
    ]);
    const [robots, sitemap] = await Promise.all([
      robotsResponse.text(),
      sitemapResponse.text(),
    ]);
    const locations = [...sitemap.matchAll(/<loc>(.*?)<\/loc>/g)].map(
      (match) => match[1],
    );

    expect(robotsResponse.status).toBe(200);
    expect(robotsResponse.headers.get("content-type")).toContain("text/plain");
    expect(robots).toBe(
      "User-agent: *\nAllow: /\nSitemap: https://rangelang.org/sitemap.xml\n",
    );
    expect(sitemapResponse.status).toBe(200);
    expect(sitemapResponse.headers.get("content-type")).toContain(
      "application/xml",
    );
    expect(locations).toEqual(
      expectedPaths.map((path) => `https://rangelang.org${path}`),
    );
    expect(sitemap).not.toContain("__preview");
    expect(sitemap).not.toContain("?preview=");
  });

  test("blocks experimental routes in production, including preview and data URLs", async () => {
    const draftPaths = allPosts
      .filter((post) => post.draft)
      .map((post) => post.href);
    const hiddenPaths = [...draftPaths, "/design-knots", "/performance", "/api/performance/run", "/__preview/onboarding-sphere", "/__og-card/posts/intro-to-range"];
    for (const path of hiddenPaths.flatMap((path) => [path, `${path}/`, `${path}?preview=range-draft`, `${path}/__data.json`])) {
      let response = await request(path);
      // SvelteKit normalizes trailing slashes before calling the server hook.
      if (path.endsWith("/") && response.status === 308) {
        expect(response.headers.get("location")).toBe(path.slice(0, -1));
        response = await request(response.headers.get("location")!);
      }
      expect(response.status).toBe(404);
      expect(response.headers.get("location")).toBeNull();
      expect(response.headers.get("x-robots-tag")).toBe("noindex, nofollow");
      const html = await response.text();
      expect(html).toBe("Not found");
      expect(html).not.toContain('rel="canonical"');
    }
  });

  test("returns a real noindex error for an unknown route", async () => {
    const response = await request("/this-route-does-not-exist");
    expect(response.status).toBe(404);
    expect(response.headers.get("location")).toBeNull();
    expect(response.headers.get("x-robots-tag")).toBe("noindex, nofollow");
  });

  test("llms.txt lists exactly the same website pages as the sitemap", async () => {
    const response = await request("/llms.txt");
    const body = await response.text();
    expect(response.status).toBe(200);
    expect(response.headers.get("content-type")).toContain("text/plain");
    const links = [...body.matchAll(/\]\((https:\/\/rangelang\.org[^)]*)\)/g)].map((match) => match[1]);
    expect(links).toEqual(expectedPaths.map((path) => `https://rangelang.org${path}`));
    for (const post of allPosts.filter((post) => post.draft)) {
      expect(body).not.toContain(post.href);
    }
  });

  test("every article link on the homepage opens its own page", async () => {
    const html = await (await request("/")).text();
    expect(html).toContain('href="/posts/intro-to-range"');
    for (const post of allPosts.filter((post) => post.draft)) {
      expect(html).not.toContain(post.href);
    }
    const links = [...html.matchAll(/href="((?:\/posts\/|\/features\/)[^"]+)"/g)].map((match) => match[1]);
    expect(links.length).toBeGreaterThan(0);
    for (const link of links) {
      const response = await request(link);
      expect(response.status).toBe(200);
      expect(response.headers.get("location")).toBeNull();
      expect(await response.text()).toContain("<article");
    }
  });

  test("all internal navigation from public pages reaches accessible content", async () => {
    const links = new Set<string>();
    for (const page of indexableSeoPages) {
      const html = await (await request(page.path)).text();
      for (const match of html.matchAll(/<a\b[^>]*\bhref="([^"]+)"/g)) {
        const url = new URL(match[1].replaceAll("&amp;", "&"), `${origin}${page.path}`);
        if (url.origin === origin || url.origin === "https://rangelang.org") {
          links.add(`${url.pathname}${url.search}`);
        }
      }
    }
    expect(links.size).toBeGreaterThan(0);
    for (const link of links) {
      const response = await request(link);
      expect(response.status, link).toBe(200);
      expect(response.headers.get("location"), link).toBeNull();
    }
  });

  test("marks preview, social-card, and utility responses private", async () => {
    for (const path of [
      "/__preview/onboarding-sphere",
      "/__og-card/posts/intro-to-range",
      "/performance",
      "/design-knots",
      "/health",
    ]) {
      const response = await request(path);
      expect(response.headers.get("x-robots-tag")).toBe("noindex, nofollow");
    }
  });

  test("records immutable full-source snapshots with verified hashes", async () => {
    const snapshotRoot = resolve(
      new URL("../src/lib/content/source-snapshots", import.meta.url).pathname,
    );
    const manifest = JSON.parse(
      await readFile(resolve(snapshotRoot, "manifest.json"), "utf8"),
    );
    expect(manifest.sourceCommit).toMatch(/^[0-9a-f]{40}$/);
    expect(manifest.files).toHaveLength(5);
    for (const file of manifest.files) {
      const content = await readFile(resolve(snapshotRoot, file.snapshot));
      expect(createHash("sha256").update(content).digest("hex")).toBe(
        file.sha256,
      );
    }
  });
});
