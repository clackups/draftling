# Git sync host test

`run.sh` compiles the device's Git client (`git_sync.cpp`, `git_*.c`,
`draftbox.c`) for the host, with small stand-ins for the ESP-IDF APIs
in `stub/` and `support.cpp`:

- the SD card is a folder on disk,
- `esp_http_client` runs `git http-backend` as a CGI, so every request
  reaches a real bare repository,
- the sync task runs synchronously.

`test.cpp` includes `git_sync.cpp` and drives whole syncs, checking the
files on the "card" and in the repository afterwards:

1. first sync of a project that already has documents, against a
   repository with documents, an image, a sub-folder and `.gitignore`
   (nothing lost on either side; files on both sides are merged),
2. later edits keep the repository's non-document files,
3. server-side changes are integrated and checked out,
4. a `git.cfg` committed to the repository does not reach the card,
5. first sync of an empty project, 6. into an empty repository,
7. 200 files, 8. more than 32 refs on the server,
9. invalid branch names are refused,
10. a rebase of 70 unpushed local commits.

Run it after changing anything under `components/git_sync/`:

```bash
./run.sh
```

It needs gcc / g++, OpenSSL's libcrypto, git and (on the first run)
curl to fetch miniz 1.14, the zlib implementation in the ESP32 ROM.
The build uses AddressSanitizer and UBSan.
