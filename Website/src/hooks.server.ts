import { dev } from "$app/environment";
import { isProductionHiddenPath, isSearchPrivatePath } from "$lib/seo";
import type { Handle } from "@sveltejs/kit";

export const handle: Handle = async ({ event, resolve }) => {
  if (!dev && isProductionHiddenPath(event.url.pathname)) {
    return new Response("Not found", {
      status: 404,
      headers: {
        "Content-Type": "text/plain; charset=utf-8",
        "X-Robots-Tag": "noindex, nofollow",
        "Cache-Control": "no-store",
      },
    });
  }
  const response = await resolve(event);
  const headers = new Headers(response.headers);
  if (response.status >= 400 || isSearchPrivatePath(event.url.pathname)) {
    headers.set("X-Robots-Tag", "noindex, nofollow");
  }

  return new Response(response.body, {
    status: response.status,
    statusText: response.statusText,
    headers,
  });
};
