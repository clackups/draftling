# Editor menu regression tests

Run the host-side feature matrix from the repository root:

```sh
cmake -S firmware/components/editor/tests -B /tmp/draftling-editor-menu-tests
cmake --build /tmp/draftling-editor-menu-tests
ctest --test-dir /tmp/draftling-editor-menu-tests --output-on-failure
```

The matrix builds the extracted menu module with USB MSC and Kermit disabled,
enabled individually, and enabled together. It varies display-setting feature
flags across those builds to verify setting IDs do not shift. Tests check menu
row order and uniqueness, menu ID/index round trips, unavailable and invalid
lookups, setting ID uniqueness/stability, and selection wrapping.
