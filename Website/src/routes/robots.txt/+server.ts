import { siteOrigin } from "$lib/posts";
import type { RequestHandler } from "./$types";

export const GET: RequestHandler = () =>
  new Response(
    [
      "User-agent: *",
      "Allow: /",
      "# Unlisted pages send noindex headers; allow crawling so bots can read them.",
      `Sitemap: ${siteOrigin}/sitemap.xml`,
      "",
    ].join("\n"),
    {
      headers: {
        "Cache-Control": "public, max-age=3600",
        "Content-Type": "text/plain; charset=utf-8",
      },
    },
  );
