# SSWIII Pyramid

Firmware and web tools for the SSWIII pyramid toy (Seeed XIAO ESP32S3,
Main Board v2).

## Lighting editor and firmware updates

**https://geoffreygao1.github.io/SSWIII-Pyramid/** (Chrome or Edge on a
computer)

- Design the top/bottom LED keyframe pattern, preview it, and send it to a
  pyramid over USB.
- Update a pyramid's firmware from the browser: plug it in, switch it on,
  click **Update firmware**. The saved pattern is kept.

The site rebuilds automatically on every push to `main`
([workflow](.github/workflows/pages.yml)): it compiles the firmware with
PlatformIO and publishes the editor, the firmware images and the
ESP Web Tools manifest. Bump `FIRMWARE_VERSION` in
[`include/config.h`](include/config.h) for each release so the page can
flag out-of-date pyramids. See [tools/lighting-editor](tools/lighting-editor)
for details and the serial protocol.

## Building locally

```bash
pio run -t upload
```

Hardware, pin map and tuning constants are in
[`include/config.h`](include/config.h).
