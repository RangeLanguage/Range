import { homepageDescription, indexableSeoPages, repositoryUrl } from "$lib/seo";
import type { RequestHandler } from "./$types";

export const GET: RequestHandler = () => new Response(
  [
    "# Range",
    "",
    `> ${homepageDescription}`,
    "",
    "Range is under active development. The repository contains the source; benchmark results describe the recorded runs.",
    "",
    "## Website",
    "",
    ...indexableSeoPages.map((page) => `- [${page.title}](${page.canonicalUrl}): ${page.description}`),
    "",
    "## Source",
    "",
    `- [Range repository](${repositoryUrl}): Language and compiler source, examples, and development documentation.`,
    "",
  ].join("\n"),
  { headers: {
    "Content-Type": "text/plain; charset=utf-8",
    "Cache-Control": "public, max-age=3600",
  } },
);
