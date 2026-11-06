# Pyramid lighting editor

A single-page web tool for designing the pyramid's LED pattern and sending it
to the toy over USB.

## Use it

1. Open **https://geoffreygao1.github.io/SSWIII-Pyramid/** in **Chrome** or
   **Edge** on a computer (or double-click `index.html` for an offline copy
   without firmware updates). Other browsers can't talk to USB serial; they
   can still design, preview and export.
2. Plug the pyramid in with USB and switch it **on** (it can't be reached
   while asleep).
3. Click **Connect** and choose the pyramid (`USB JTAG/serial debug unit` /
   `COM…`).
4. Edit the **Top set** (LEDs 5–8) and **Bottom set** (LEDs 1–4). With
   **Live update** on, the pyramid follows your edits straight away.
5. Click **Save to pyramid** to keep the pattern after it sleeps.

## How patterns work

Each set plays its own loop of keyframes. A keyframe fades in from the
previous keyframe's colour over **Fade** seconds (smooth or linear), then stays
for **Hold** seconds. The first keyframe fades in from the last, so the loop is
seamless. One keyframe gives a solid colour; no keyframes turns the set off.
Up to 16 keyframes per set. **Brightness** scales the whole pattern.

**Export JSON / Import JSON** save and share patterns as files.
**Built-in pattern** restores the factory look (green breathing bottom, dim
red top).

## Firmware updates

The hosted page also flashes the latest firmware (via
[ESP Web Tools](https://esphome.github.io/esp-web-tools/)): switch the
pyramid on, click **Update firmware** and pick it. The saved pattern is kept.

Every push to `main` runs [`.github/workflows/pages.yml`](../../.github/workflows/pages.yml),
which builds the firmware with PlatformIO and publishes this page, the
firmware images and `manifest.json`. The version shown comes from
`FIRMWARE_VERSION` in [`include/config.h`](../../include/config.h); bump it
for each release so the page can tell when a pyramid is out of date.

## Protocol

The page uses the line-based serial protocol in
[`src/console.h`](../../src/console.h) (`HELLO`, `GET`, `SET <json>`, `SAVE`,
`REVERT`, `DEFAULT`). The pattern JSON is:

```json
{
  "brightness": 255,
  "bottom": [{ "c": "#00c200", "fade": 5750, "hold": 0, "ease": "smooth" }],
  "top":    [{ "c": "#880000", "fade": 0,    "hold": 0, "ease": "linear" }]
}
```

Times are in milliseconds.
