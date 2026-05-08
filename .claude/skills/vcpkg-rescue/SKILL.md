---
name: vcpkg-rescue
description: Rescue a stalled vcpkg manifest install on Windows. Triggers when vcpkg fails with "curl operation failed with error code 35 (SSL connect error)", "Not a transient network error", or when the user says things like "vcpkg 又挂了 / 又是 SSL 35 / 帮我喂 tarball / vcpkg-rescue / port 编不下去". Reads the most recent vcpkg-manifest-install.log under build/, extracts every URL whose download failed, and re-fetches via system curl with --ssl-no-revoke through the local proxy (default 127.0.0.1:7897). Files land in $env:VCPKG_DOWNLOADS (or $LOCALAPPDATA/vcpkg/downloads) where vcpkg picks them up by SHA512 on next configure.
---

# vcpkg-rescue

## When to invoke

Triggers (any of):

- vcpkg-manifest-install.log contains lines like `error: curl operation failed with error code 35` or `Not a transient network error, won't retry download`.
- User says "vcpkg 挂了 / vcpkg 又挂 / SSL error 35 / 帮我喂 tarball / vcpkg-rescue".
- After clearing the registry cache and re-running cmake, vcpkg fails on the same port.

Do **not** invoke when:

- The error is `port not found` / `Duplicate preset` / `Invalid extra field` — those are config issues, not download issues; explain to the user instead.
- vcpkg succeeded but the project failed to link / compile — that's the project's bug, not vcpkg's.

## Procedure

1. **Confirm the proxy is up first** — saves time vs. hitting curl errors:
   ```bash
   curl.exe -x http://127.0.0.1:7897 -s -o /dev/null --max-time 10 \
       -w "github via proxy http=%{http_code} time=%{time_total}s\n" \
       https://github.com
   ```
   - Expected `http=200`. If `000` / timeout, **stop here** and tell the user the proxy is down. Don't run rescue blind.

2. **Run the rescue script with VCPKG_DOWNLOADS in env** so it writes to the right place:
   ```bash
   powershell.exe -NoProfile -ExecutionPolicy Bypass -Command \
     "$env:VCPKG_DOWNLOADS = 'E:/dev/vcpkg-downloads'; & ./tools/vcpkg-rescue.ps1"
   ```
   - The script auto-locates the most recent `build/*/vcpkg-manifest-install.log`, parses failed downloads, fetches each via `curl --ssl-no-revoke -x $proxy`.

3. **Read the script output**. Three possible outcomes per file:
   - `OK (X.X MB)` — fetched successfully, vcpkg will use it next run.
   - `SKIP (already cached)` — file already in the downloads dir; the previous failure may have been a transient SHA mismatch. **Verify**: re-run cmake; if still fails on this exact file, manually delete it and re-run rescue.
   - `FAILED (curl exit N)` — curl couldn't fetch even with proxy. Check exit code:
     - `35` — schannel TLS handshake failed even with `--ssl-no-revoke`. Try a different proxy node, or download the URL manually from a different machine and drop it in `$VCPKG_DOWNLOADS`.
     - `28` — timeout. Proxy is throttling. Wait a few minutes, retry.
     - `7` — connection refused. Proxy is down.

4. **If split downloads dir** (`$LOCALAPPDATA/vcpkg/downloads` vs `$VCPKG_DOWNLOADS`): mirror missing files. Common when `VCPKG_DOWNLOADS` was set partway through development:
   ```bash
   SRC="$LOCALAPPDATA/vcpkg/downloads"
   DST="$VCPKG_DOWNLOADS"
   for f in "$SRC"/*.tar.* "$SRC"/*.zip; do
       [ -f "$f" ] || continue
       name=$(basename "$f")
       [ -e "$DST/$name" ] || cp "$f" "$DST/$name"
   done
   ```

5. **Tell the user the next step**: `cmake --build --preset <preset>` (or VS reconfigure). vcpkg will validate SHA512 on the cached file and skip download.

## Anti-patterns

- ❌ Don't suggest "switch to a different vcpkg fork / mirror" — registry mirror doesn't fix tarball downloads (covered in [docs/devlog/2026-W19.md §1.3](../docs/devlog/2026-W19.md)).
- ❌ Don't suggest setting `X_VCPKG_ASSET_SOURCES = x-file,...` — that's the wrong syntax (asset cache only supports `x-azurl` / `x-script` / `x-block-origin` / `clear`).
- ❌ Don't run rescue automatically without confirming proxy is up; you'll just generate noise.

## Related files

- Tool: [tools/vcpkg-rescue.ps1](../../../tools/vcpkg-rescue.ps1)
- Background: [docs/devlog/2026-W19.md §1.3](../../../docs/devlog/2026-W19.md)
