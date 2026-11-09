# Editor menu regression tests

Run the host-side feature matrix from the repository root:

```sh
cmake -S firmware/components/editor/tests -B /tmp/draftling-editor-menu-tests
cmake --build /tmp/draftling-editor-menu-tests
ctest --test-dir /tmp/draftling-editor-menu-tests --output-on-failure
```

The F1 matrix builds the extracted menu module with USB MSC and Kermit
disabled, enabled individually, and enabled together. The Settings matrix
varies backlight, color, rotation, and inversion support, including binary
backlight boards. Tests check both hierarchies and feature-gated row order,
stable ID/index lookups, unavailable and invalid lookups, descriptor
uniqueness and parent validity, and existing setting ID stability.
