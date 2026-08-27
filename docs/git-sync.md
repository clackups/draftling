# Git synchronisation

Draftling ships a small but real Git client. It keeps a genuine commit
history on the SD card and exchanges objects with a remote server over
the **smart HTTP protocol**
([gitprotocol-http](https://www.kernel.org/pub/software/scm/git/docs/gitprotocol-http.html)):
`GET .../info/refs?service=git-upload-pack`, `POST .../git-upload-pack`
and `POST .../git-receive-pack`, with pkt-line framing and packfile v2
transfer. There is no dependency on any host-specific REST API, so any
standard Git HTTP host works (GitHub, Gitea, Forgejo, GitLab, cgit with
smart HTTP, a bare repo behind `git http-backend`, ...).

## What lives on the SD card

```
/sdcard/
  note1.md            working tree (the files you edit)
  note2.md
  .git/               real Git repository
    HEAD
    refs/heads/<branch>
    refs/remotes/origin/<branch>
    objects/xx/…       loose, zlib-compressed objects
    tmp/               transient fetch.pack / push.pack
```

The object store uses standard loose objects (`<type> <size>\0<payload>`
deflated with zlib), so the `.git` directory can be copied onto a
computer and inspected with the real `git` if needed. zlib is provided
by the miniz implementation in the ESP32 ROM; SHA-1 by Mbed TLS / PSA
Crypto; TLS + HTTP by `esp_http_client` / `esp-tls`.

## What one sync does

Triggered by `Ctrl+G`, by the F1 → Git Sync menu entry, or automatically
before deep sleep. The sync task runs these steps in order:

1. **Advertise** – contact the remote and read its branch tip.
2. **Clone** – on the very first sync, if the remote branch exists its
   full history is fetched and checked out. (History is fetched in
   full; there is no shallow clone.)
3. **Commit** – the current `*.md` files in the working directory are
   written as a tree and committed on top of the local branch tip. If
   nothing changed, no commit is made.
4. **Fetch** – if the remote tip is not already in local history, the
   missing commits are fetched (sending `have` lines for the local
   commits so the server can send a minimal packfile).
5. **Integrate**
   - *remote unchanged* → nothing to do;
   - *local unchanged* → **fast-forward** to the remote tip;
   - *both advanced* → **rebase**: each local commit is replayed on top
     of the remote tip through a three-way merge.
6. **Merge & conflicts** – the three-way merge is done per file:
   - a file changed on only one side is taken from that side;
   - a file changed identically on both sides is taken as-is;
   - a file changed on both sides gets a **diff3 line merge**. Cleanly
     separable edits are merged silently. Overlapping edits are written
     into the file with conflict markers

     ```
     <<<<<<< ours (remote)
     …the remote version…
     =======
     …your local version…
     >>>>>>> theirs (local)
     ```

     and the commit is still created – conflicts are **committed as-is**,
     never lost. The commit message gets a `[draftling] N conflict(s)`
     note. Resolve them by editing the file and syncing again.
   - a delete/modify clash keeps the surviving content and marks a
     conflict.
7. **Checkout** – the merged tree is written back to the SD card.
   The editor reloads the open buffer and the file browser refreshes so
   you immediately see the merged result.
8. **Push** – the new commits are packed and pushed to the remote with
   `git-receive-pack`. The push is a plain fast-forward update of
   `refs/heads/<branch>`; if the remote moved again while the sync was
   running, the push is refused and you are asked to sync once more.

## Configuration (`/sdcard/git.cfg`)

```
repo_url=https://github.com/user/repo
branch=main
token=ghp_xxxxxxxxxxxx
path=notes/
username=x-access-token
author_name=Jane Doe
author_email=jane@example.com
```

| Key | Required | Meaning |
|-----|----------|---------|
| `repo_url` | yes | Repository URL, `.git` suffix optional. |
| `token` | yes | HTTP Basic password / access token. |
| `branch` | no | Branch to sync (default `main`). |
| `path` | no | Map the working tree onto this sub-directory of the repo. Intermediate trees are created/preserved; the rest of the repo is left untouched. |
| `username` | no | HTTP Basic username (default `x-access-token`). |
| `author_name`, `author_email` | no | Identity written into commits (default `Draftling <draftling@localhost>`). |

Wall-clock time for commit timestamps is taken from a best-effort SNTP
query on the first sync after boot; if that fails, timestamps are
floored at 2025-01-01. Git orders history by the commit graph, not by
timestamp, so an inaccurate clock does not corrupt anything.

## Scope and limitations

This is deliberately a minimal client:

- **One branch**, no tags, no submodules, no signed commits.
- The working tree is the **flat set of `*.md` files** in the sync
  directory (non-recursive). Sub-directories in the repository under
  `path` are not checked out. Dotfiles (`.git`, `*.meta`, `wifi.cfg`,
  `git.cfg`) are never committed.
- Deleting every local `*.md` file will commit the deletion of the whole
  synced sub-tree on the next sync – that is correct Git behaviour, but
  worth knowing.
- The initial sync fetches full history; for a large repository that can
  be slow and use significant SD space.
- Line-level merges above ~1400 lines on any side fall back to a
  whole-file conflict block.
- Authentication is HTTP Basic only (token in `git.cfg`); SSH is not
  supported.
