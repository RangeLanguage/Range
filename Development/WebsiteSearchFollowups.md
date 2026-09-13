# Website search discoverability

Preserved from development's TODO.md during the experimental compiler reset merge.
These are historical release notes; live status has not been rechecked here.

- [x] Make the Website self-contained and publish one 11-route SEO contract
  with canonical metadata, structured data, `robots.txt`, `sitemap.xml`, and
  production draft/private-route indexing guards.
- [x] Release the verified Website snapshot to `production` and deploy it at
  `https://rangelang.org` with the pinned Sveltely submodule and a rollback
  image retained.
- [x] Point `www.rangelang.org` at the production server and verify its
  permanent path-and-query-preserving redirect to the apex origin.
- [x] Verify the domain in Google Search Console, submit the sitemap, and add
  the four priority URLs to Google's crawl queue.
- [x] Import the verified property into Bing Webmaster Tools and submit its
  sitemap and priority URLs.
- [ ] Inspect Google and Bing sitemap and URL processing after 48 hours and
  seven days.
  - One-time Codex follow-ups were scheduled for 2026-08-28 and 2026-09-02 at
    17:30 Asia/Tbilisi, pending acceptance of the automation cards.
