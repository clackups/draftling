# Editor menu regression tests

Run the host-side feature matrix from the repository root:

```sh
cmake -S firmware/components/editor/tests -B /tmp/draftling-editor-menu-tests
cmake --build /tmp/draftling-editor-menu-tests
ctest --test-dir /tmp/draftling-editor-menu-tests --output-on-failure
```

The matrix builds the extracted menu module with USB MSC and Kermit disabled,
enabled individually, and enabled together. It checks row order and uniqueness,
ID/index round trips, unavailable and invalid lookups, and selection wrapping.
