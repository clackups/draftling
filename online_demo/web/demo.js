/*
 * Draftling online demo -- the page side of the simulated device.
 *
 * - Variants: one firmware build per screen (board) and language pack,
 *   in fw/<board>/<pack>/ (variants.js lists the ones built). The page
 *   loads the one picked, remembered in the URL and in localStorage.
 * - Storage: the simulated SD card (/sdcard, shared by all variants)
 *   and the settings store (/persist: one NVS file per board, see
 *   src/nvs_demo.cpp) are Emscripten IDBFS mounts, loaded from
 *   IndexedDB before the firmware starts and written back every few
 *   seconds, when the firmware asks, and when the page is hidden.
 * - Screen: a color LCD gets changed RGB565 rectangles of the frame
 *   buffer (Module.demoPresent). An e-paper panel gets whole frames
 *   with the time its refresh starts and lasts (Module.demoEpdRefresh),
 *   drawn as the panel shows them: pixels that change go grey during a
 *   fast refresh, the whole screen flashes during a full one.
 * - Keyboard: KeyboardEvent.code (the physical key) -> HID usage code,
 *   plus the HID modifier byte, delivered like the BLE keyboard does.
 *   The firmware repeats held keys itself, so browser auto-repeat is
 *   ignored. Keyboard layouts are applied by the firmware (Ctrl+L).
 * - Touch: mouse / pen / finger on the canvas -> LVGL pointer.
 * - Buttons: the board's own buttons beside the screen -> demo_button().
 */
"use strict";

(function () {
  /* Emscripten's draftling.js adopts this object as its Module. */
  const Module = window.Module = window.Module || {};

  const BOARDS = {
    freenove_fnk0104s: {
      name: "Freenove FNK0104S",
      label: "Freenove FNK0104S \u00b7 ST7796 4.0\" color LCD \u00b7 480\u00d7320",
      option: "Freenove FNK0104S -- 4.0\" color LCD",
      style: "lcd",
      buttons: [
        { id: 0, label: "BOOT", title: "BOOT button. Hold 2 s: forget the paired keyboards" },
      ],
    },
    seeed_reterminal_sticky: {
      name: "Seeed reTerminal Sticky",
      label: "Seeed reTerminal Sticky \u00b7 SSD1677 3.97\" e-paper \u00b7 800\u00d7480",
      option: "Seeed reTerminal Sticky -- 3.97\" e-paper",
      style: "epaper",
      buttons: [
        { id: 0, label: "Power", title: "Power button. Press: sleep. Hold 2 s: forget the paired keyboards" },
        { id: 1, label: "Up", title: "Page Up" },
        { id: 2, label: "Down", title: "Page Down" },
      ],
    },
  };

  const PACKS = {
    default: {
      option: "Default -- European layouts",
      tip: "Default pack: US, Ukrainian, German, French, Hebrew and 20 more European layouts. " +
           "Turn on the ones you want in F1 \u2192 Settings \u2192 Keyboard \u2192 Active layouts, " +
           "then switch with Ctrl+L.",
    },
    vietnamese: {
      option: "Vietnamese",
      tip: "Vietnamese pack: switch to VI with Ctrl+L and type Telex (aa = \u00e2, aw = \u0103, " +
           "dd = \u0111, s f r x j = tones).",
    },
    korean: {
      option: "Korean",
      tip: "Korean pack: switch to KO with Ctrl+L; the keys type Dubeolsik Hangul and compose syllables.",
    },
    japanese: {
      option: "Japanese",
      tip: "Japanese pack: switch to JA with Ctrl+L, type romaji; Space converts to kanji, " +
           "Enter accepts.",
    },
    chinese: {
      option: "Chinese",
      tip: "Chinese pack: switch to ZH with Ctrl+L, type pinyin and pick a candidate with " +
           "Space or a digit.",
    },
    indic: {
      option: "Indic -- Hindi, Marathi, Nepali, Bengali, Tamil",
      tip: "Indic pack: Hindi, Marathi, Nepali, Bengali and Tamil layouts, shaped on the device " +
           "with HarfBuzz. Tamil 99 and Hindi InScript are on: switch with Ctrl+L. Turn on the " +
           "others in F1 \u2192 Settings \u2192 Keyboard \u2192 Active layouts.",
    },
  };

  const DEFAULT_BOARD = "freenove_fnk0104s";
  const DEFAULT_PACK = "default";
  const CHOICE_KEY = "draftling-demo-variant";
  const RESET_FLAG = "draftling-demo-reset";
  const IDB_NAMES = ["/sdcard", "/persist"];

  const canvas = document.getElementById("screen");
  const ctx = canvas.getContext("2d", { alpha: false });
  const device = document.getElementById("device");
  const overlay = document.getElementById("overlay");
  const overlayText = document.getElementById("overlay-text");
  const statusEl = document.getElementById("status");
  const focusHint = document.getElementById("focus-hint");
  const boardSel = document.getElementById("board");
  const packSel = document.getElementById("pack");
  const busyEl = document.getElementById("busy");

  let screenW = 480, screenH = 320, screenScale = 1, upsideDown = false, epaper = false;
  let running = false;
  let halted = false;
  let storageOk = true;

  function setStatus(text) {
    if (statusEl) statusEl.textContent = text;
  }

  function showOverlay(text, onClick) {
    overlayText.textContent = text;
    overlay.hidden = false;
    overlay.onclick = onClick || null;
  }

  function lsGet(key) {
    try { return localStorage.getItem(key); } catch (e) { return null; }
  }
  function lsSet(key, value) {
    try {
      if (value === null) localStorage.removeItem(key);
      else localStorage.setItem(key, value);
    } catch (e) { /* ignore */ }
  }

  /* ---- variant ------------------------------------------------------ */

  const built = new Set(window.DEMO_VARIANTS || []);
  function isBuilt(board, pack) { return built.has(board + "/" + pack); }

  function pickVariant() {
    const params = new URLSearchParams(location.search);
    const saved = (lsGet(CHOICE_KEY) || "").split("/");
    let board = params.get("board") || saved[0] || DEFAULT_BOARD;
    let pack = params.get("pack") || saved[1] || DEFAULT_PACK;
    if (!BOARDS[board]) board = DEFAULT_BOARD;
    if (!PACKS[pack]) pack = DEFAULT_PACK;
    if (!isBuilt(board, pack)) {
      const any = [...built].find((v) => v.startsWith(board + "/")) || [...built][0];
      if (any) [board, pack] = any.split("/");
    }
    return { board, pack };
  }

  const variant = pickVariant();
  const board = BOARDS[variant.board];

  function fillPicker() {
    for (const [id, b] of Object.entries(BOARDS)) {
      const o = new Option(b.option, id, false, id === variant.board);
      o.disabled = ![...built].some((v) => v.startsWith(id + "/"));
      boardSel.add(o);
    }
    for (const [id, p] of Object.entries(PACKS)) {
      const o = new Option(p.option, id, false, id === variant.pack);
      o.disabled = !isBuilt(variant.board, id);
      packSel.add(o);
    }
    const change = () => {
      let b = boardSel.value, p = packSel.value;
      if (!isBuilt(b, p)) p = isBuilt(b, DEFAULT_PACK) ? DEFAULT_PACK
        : ([...built].find((v) => v.startsWith(b + "/")) || "/").split("/")[1];
      switchTo(b, p);
    };
    boardSel.addEventListener("change", change);
    packSel.addEventListener("change", change);
    document.getElementById("pack-tip").textContent = PACKS[variant.pack].tip;
    document.getElementById("board-label").textContent = board.label;
    device.classList.add(board.style);
  }

  function switchTo(b, p) {
    halted = true;
    showOverlay("Loading " + BOARDS[b].name + "\u2026");
    lsSet(CHOICE_KEY, b + "/" + p);
    syncStorage(() => {
      const params = new URLSearchParams(location.search);
      params.set("board", b);
      params.set("pack", p);
      location.search = params.toString();
    });
  }

  /* ---- storage ------------------------------------------------------ */

  let syncing = false, syncAgain = false, syncTimer = 0;

  function syncStorage(done) {
    if (!running || !storageOk) { if (done) done(); return; }
    if (syncing) { syncAgain = true; if (done) setTimeout(() => syncStorage(done), 100); return; }
    syncing = true;
    Module.FS.syncfs(false, (err) => {
      syncing = false;
      if (err) console.warn("Saving to browser storage failed:", err);
      if (syncAgain) { syncAgain = false; syncStorage(); }
      if (done) done();
    });
  }

  function deleteDatabases() {
    return Promise.all(IDB_NAMES.map((name) => new Promise((resolve) => {
      const req = indexedDB.deleteDatabase(name);
      req.onsuccess = req.onerror = req.onblocked = () => resolve();
    })));
  }

  function mountStorage() {
    const FS = Module.FS;
    for (const dir of IDB_NAMES) {
      FS.mkdir(dir);
      FS.mount(Module.IDBFS, {}, dir);
    }
    Module.addRunDependency("idbfs");
    const finish = () => {
      FS.syncfs(true, (err) => {
        if (err) {
          storageOk = false;
          console.warn("Browser storage is not available:", err);
        }
        Module.removeRunDependency("idbfs");
      });
    };
    if (lsGet(RESET_FLAG) === "1") {
      lsSet(RESET_FLAG, null);
      deleteDatabases().then(finish);
    } else {
      finish();
    }
  }

  /* ---- screen ------------------------------------------------------- */

  /* E-paper colours: ink on paper, and the grey of a pixel in mid-waveform. */
  const PAPER = [226, 224, 216], INK = [38, 38, 42], MID = [140, 139, 134];

  let image = null;
  let panelW = 480, panelH = 320;   /* canvas pixels */

  function fitScreen() {
    /* As large as the window allows, up to 3x. */
    const physW = screenW * screenScale, physH = screenH * screenScale;
    let chrome = 0;   /* horizontal padding of the board, the bezel and the side buttons */
    for (const el of [device, device.querySelector(".bezel")]) {
      const cs = getComputedStyle(el);
      chrome += parseFloat(cs.paddingLeft) + parseFloat(cs.paddingRight);
    }
    const side = document.getElementById("side-buttons");
    if (side.offsetWidth) chrome += side.offsetWidth + 12;
    const avail = device.parentElement.clientWidth - chrome;
    const maxH = Math.max(window.innerHeight * 0.72, 240);
    /* Work in device pixels: with 125 % / 150 % display scaling a CSS
     * pixel is not a screen pixel. Each panel pixel gets a whole number
     * of screen pixels, so none is doubled or dropped (a fractional
     * nearest-neighbour zoom made the text grainy and lost strokes).
     * When rounding down would cost more than a quarter of the size
     * (1.9x -> 1x) the fractional zoom stays: the LCD's pixel fonts
     * take that fine, while the e-paper's fine hi-res text is scaled
     * smoothly below 2x. A window too small for 1:1 shrinks the
     * panel, smoothly. */
    const dpr = window.devicePixelRatio || 1;
    let dz = Math.min(avail / physW, maxH / physH, 3) * dpr;
    if (dz >= 1 && Math.floor(dz) / dz >= 0.75) dz = Math.floor(dz);
    const crisp = Number.isInteger(dz) || (dz >= 1 && (!epaper || dz >= 2));
    canvas.style.imageRendering = crisp ? "pixelated" : "auto";
    canvas.style.width = (physW * dz / dpr) + "px";
    canvas.style.height = (physH * dz / dpr) + "px";
  }

  Module.demoSetScreen = function (w, h, scale, flip, epd) {
    screenW = w; screenH = h; screenScale = scale; upsideDown = !!flip; epaper = !!epd;
    /* An LCD canvas is the LVGL frame (CSS scales it); an e-paper one is
     * the panel, which gets the frame scaled up already. */
    panelW = epaper ? w * scale : w;
    panelH = epaper ? h * scale : h;
    canvas.width = panelW;
    canvas.height = panelH;
    canvas.style.transform = upsideDown ? "rotate(180deg)" : "";
    image = ctx.createImageData(panelW, panelH);
    if (epaper) {
      shown = new Uint8Array(panelW * panelH).fill(1);
      paint(shown, null, null);
    }
    fitScreen();
  };

  Module.demoPresent = function (fbPtr, fbW, x, y, w, h) {
    if (!image) return;
    const src = Module.HEAPU16;
    const dst = image.data;
    const base = fbPtr >> 1;
    for (let row = y; row < y + h; row++) {
      let s = base + row * fbW + x;
      let d = (row * panelW + x) * 4;
      for (let col = 0; col < w; col++, s++, d += 4) {
        const p = src[s];
        const r = p >> 11, g = (p >> 5) & 0x3f, b = p & 0x1f;
        dst[d] = (r << 3) | (r >> 2);
        dst[d + 1] = (g << 2) | (g >> 4);
        dst[d + 2] = (b << 3) | (b >> 2);
        dst[d + 3] = 255;
      }
    }
    ctx.putImageData(image, 0, 0, x, y, w, h);
  };

  /* ---- e-paper ------------------------------------------------------ */

  let shown = null;       /* what the glass shows: 1 byte per pixel, 1 = white */
  let busyCount = 0;

  /* Draw frame px; where mask is set, draw maskColor instead. */
  function paint(px, mask, maskColor) {
    const d = image.data;
    for (let i = 0, j = 0; i < px.length; i++, j += 4) {
      const c = mask && mask[i] ? maskColor : (px[i] ? PAPER : INK);
      d[j] = c[0]; d[j + 1] = c[1]; d[j + 2] = c[2]; d[j + 3] = 255;
    }
    ctx.putImageData(image, 0, 0);
  }

  function fill(color) {
    const d = image.data;
    for (let j = 0; j < d.length; j += 4) {
      d[j] = color[0]; d[j + 1] = color[1]; d[j + 2] = color[2]; d[j + 3] = 255;
    }
    ctx.putImageData(image, 0, 0);
  }

  function setBusy(delta) {
    busyCount += delta;
    busyEl.hidden = busyCount <= 0;
  }

  Module.demoEpdRefresh = function (ptr, w, h, full, delayMs, durMs) {
    if (!image || w !== panelW || h !== panelH) return;
    /* Copy now: the firmware reuses the buffer for the next frame. */
    const next = Module.HEAPU8.slice(ptr, ptr + w * h);
    setBusy(1);
    setTimeout(() => {
      const prev = shown;
      if (full) {
        /* Full refresh: every pixel is re-driven -- the screen flashes
         * inverted, black, white, then shows the new frame. */
        const inv = prev.map((v) => v ^ 1);
        paint(inv, null, null);
        setTimeout(() => fill(INK), durMs * 0.25);
        setTimeout(() => fill(PAPER), durMs * 0.5);
        setTimeout(() => paint(next, null, null), durMs * 0.75);
      } else {
        /* Fast (differential) refresh: only the pixels that change are
         * driven; they pass through grey before settling. */
        const changed = new Uint8Array(next.length);
        let any = false;
        for (let i = 0; i < next.length; i++) {
          if (next[i] !== prev[i]) { changed[i] = 1; any = true; }
        }
        if (any) {
          paint(prev, changed, MID);
          setTimeout(() => paint(next, null, null), durMs * 0.6);
        }
      }
      shown = next;
      setTimeout(() => setBusy(-1), durMs);
    }, Math.max(0, delayMs));
  };

  Module.demoSetBacklight = function (percent) {
    if (epaper) return;
    /* A dimmed LCD, not a black one: keep some light at 0 %. */
    const level = Math.max(0, Math.min(100, percent));
    canvas.style.filter = level >= 100 ? "" : "brightness(" + (0.25 + 0.75 * level / 100).toFixed(3) + ")";
  };

  Module.demoStorageDirty = function () {
    clearTimeout(syncTimer);
    syncTimer = setTimeout(syncStorage, 300);
  };

  Module.demoRestart = function () {
    halted = true;
    showOverlay("Restarting\u2026");
    syncStorage(() => location.reload());
  };

  Module.demoSleep = function () {
    halted = true;
    /* An LCD goes dark; e-paper keeps its last picture. */
    if (!epaper) canvas.style.filter = "brightness(0)";
    syncStorage(() => {
      showOverlay("Draftling is asleep.\nPress any key or click to wake it up.", () => location.reload());
    });
  };

  /* ---- keyboard ----------------------------------------------------- */

  const HID = {
    Enter: 0x28, Escape: 0x29, Backspace: 0x2a, Tab: 0x2b, Space: 0x2c,
    Minus: 0x2d, Equal: 0x2e, BracketLeft: 0x2f, BracketRight: 0x30,
    Backslash: 0x31, IntlHash: 0x32, Semicolon: 0x33, Quote: 0x34,
    Backquote: 0x35, Comma: 0x36, Period: 0x37, Slash: 0x38,
    CapsLock: 0x39,
    PrintScreen: 0x46, ScrollLock: 0x47, Pause: 0x48, Insert: 0x49,
    Home: 0x4a, PageUp: 0x4b, Delete: 0x4c, End: 0x4d, PageDown: 0x4e,
    ArrowRight: 0x4f, ArrowLeft: 0x50, ArrowDown: 0x51, ArrowUp: 0x52,
    NumLock: 0x53, NumpadDivide: 0x54, NumpadMultiply: 0x55,
    NumpadSubtract: 0x56, NumpadAdd: 0x57, NumpadEnter: 0x58,
    NumpadDecimal: 0x63, IntlBackslash: 0x64, ContextMenu: 0x65,
    NumpadEqual: 0x67, IntlRo: 0x87, IntlYen: 0x89,
  };
  for (let i = 0; i < 26; i++) HID["Key" + String.fromCharCode(65 + i)] = 0x04 + i;
  for (let i = 1; i <= 9; i++) HID["Digit" + i] = 0x1d + i;
  HID.Digit0 = 0x27;
  for (let i = 1; i <= 12; i++) HID["F" + i] = 0x39 + i;
  for (let i = 1; i <= 9; i++) HID["Numpad" + i] = 0x58 + i;
  HID.Numpad0 = 0x62;

  const MOD = {
    ControlLeft: 0x01, ShiftLeft: 0x02, AltLeft: 0x04, MetaLeft: 0x08,
    ControlRight: 0x10, ShiftRight: 0x20, AltRight: 0x40, MetaRight: 0x80,
  };

  /* Keys the page leaves to the browser. */
  function browserKey(e) {
    if (e.code === "F5" || e.code === "F11" || e.code === "F12") return true;
    if (e.ctrlKey && e.shiftKey && (e.code === "KeyI" || e.code === "KeyJ")) return true;
    return false;
  }

  let mods = 0;
  const held = new Set();
  let active = false;

  function modifierByte(e) {
    let m = mods;
    /* On-screen keyboards report Shift etc. without a key event of its
     * own. */
    if (e.shiftKey && !(m & (MOD.ShiftLeft | MOD.ShiftRight))) m |= MOD.ShiftLeft;
    if (e.ctrlKey && !(m & (MOD.ControlLeft | MOD.ControlRight))) m |= MOD.ControlLeft;
    if (e.altKey && !(m & (MOD.AltLeft | MOD.AltRight))) m |= MOD.AltLeft;
    if (e.metaKey && !(m & (MOD.MetaLeft | MOD.MetaRight))) m |= MOD.MetaLeft;
    /* Keep the byte honest when a modifier changed outside the page. */
    if (!e.shiftKey) m &= ~(MOD.ShiftLeft | MOD.ShiftRight);
    if (!e.ctrlKey) m &= ~(MOD.ControlLeft | MOD.ControlRight);
    if (!e.altKey) m &= ~(MOD.AltLeft | MOD.AltRight);
    if (!e.metaKey) m &= ~(MOD.MetaLeft | MOD.MetaRight);
    /* Windows sends AltGr as Ctrl + Right Alt; the firmware wants only
     * Right Alt (AltGr) there. */
    if (e.getModifierState && e.getModifierState("AltGraph")) {
      m &= ~MOD.ControlLeft;
      m |= MOD.AltRight;
    }
    return m;
  }

  function sendKey(code, modifier, pressed) {
    if (!running || halted) return;
    Module._demo_key(code, modifier, pressed ? 1 : 0);
  }

  function releaseAll() {
    for (const code of held) sendKey(code, 0, false);
    held.clear();
    mods = 0;
  }

  function setActive(on) {
    active = on;
    device.classList.toggle("focused", on);
    focusHint.classList.toggle("hidden", on);
    if (!on) releaseAll();
  }

  document.addEventListener("keydown", (e) => {
    if (halted && overlay.onclick) { overlay.onclick(); return; }
    if (!active || browserKey(e)) return;
    if (e.code in MOD) {
      mods |= MOD[e.code];
      e.preventDefault();
      return;
    }
    const code = HID[e.code];
    if (code === undefined) return;
    e.preventDefault();
    if (e.repeat) return;
    held.add(code);
    sendKey(code, modifierByte(e), true);
  });

  document.addEventListener("keyup", (e) => {
    if (e.code in MOD) {
      mods &= ~MOD[e.code];
      if (active) e.preventDefault();
      return;
    }
    const code = HID[e.code];
    if (code === undefined || !held.has(code)) return;
    held.delete(code);
    if (active) e.preventDefault();
    sendKey(code, modifierByte(e), false);
  });

  canvas.addEventListener("focus", () => setActive(true));
  canvas.addEventListener("blur", () => setActive(false));
  window.addEventListener("blur", releaseAll);

  /* ---- on-screen keyboard (phones and tablets) ---------------------- */

  /* A canvas never brings up the on-screen keyboard, a text field does:
   * on touch devices a tap on the screen focuses an invisible textarea.
   * On-screen keyboards mostly send no usable key events (key
   * "Unidentified"), and their predictive text rewrites words, so what
   * they type is read back from the field: the difference to the last
   * value becomes Backspaces and the US-keyboard keys of the new
   * characters. A hardware keyboard still goes through keydown above,
   * which keeps the text out of the field. */
  const proxy = document.getElementById("kbd-proxy");
  let touchMode = window.matchMedia && matchMedia("(pointer: coarse)").matches;
  /* Spaces before the caret, so Backspace on an empty field still
   * deletes (and predictive text sees a word boundary). */
  const PAD = " ".repeat(16);
  let proxyLast = PAD;
  let composing = false;

  function setTouchMode(on) {
    touchMode = on;
    document.body.classList.toggle("touch", on);
    focusHint.textContent = on ? "Tap the screen to bring up the keyboard."
                               : "Click the screen, then type.";
  }

  function proxyReset() {
    proxy.value = PAD;
    proxyLast = PAD;
    proxy.setSelectionRange(PAD.length, PAD.length);
  }

  /* The screen gets the keys: the canvas, or on touch devices the
   * textarea (re-focused, so a dismissed keyboard comes back). */
  function focusScreen() {
    if (!touchMode) { canvas.focus(); return; }
    if (document.activeElement === proxy) proxy.blur();
    proxyReset();
    proxy.focus({ preventScroll: true });
  }

  /* US keyboard: character -> [HID usage, shift]. */
  const US_KEYS = {};
  (function () {
    for (let i = 0; i < 26; i++) {
      US_KEYS[String.fromCharCode(97 + i)] = [0x04 + i, false];
      US_KEYS[String.fromCharCode(65 + i)] = [0x04 + i, true];
    }
    const rows = [
      ["1234567890", "!@#$%^&*()", 0x1e],
    ];
    for (const [plain, shifted, first] of rows) {
      for (let i = 0; i < plain.length; i++) {
        US_KEYS[plain[i]] = [first + i, false];
        US_KEYS[shifted[i]] = [first + i, true];
      }
    }
    const pairs = [
      ["-", "_", 0x2d], ["=", "+", 0x2e], ["[", "{", 0x2f], ["]", "}", 0x30],
      ["\\", "|", 0x31], [";", ":", 0x33], ["'", "\"", 0x34], ["`", "~", 0x35],
      [",", "<", 0x36], [".", ">", 0x37], ["/", "?", 0x38],
    ];
    for (const [plain, shifted, code] of pairs) {
      US_KEYS[plain] = [code, false];
      US_KEYS[shifted] = [code, true];
    }
    US_KEYS[" "] = [0x2c, false];
    US_KEYS["\n"] = [0x28, false];
    US_KEYS["\t"] = [0x2b, false];
    /* Typographic characters phone keyboards put in by themselves. */
    const alias = {
      "\u2018": "'", "\u2019": "'", "\u201c": "\"", "\u201d": "\"",
      "\u2013": "-", "\u2014": "-", "\u00a0": " ",
    };
    for (const [from, to] of Object.entries(alias)) US_KEYS[from] = US_KEYS[to];
  })();

  let warnedNonAscii = false;
  function tapKey(code, modifier) {
    sendKey(code, modifier, true);
    sendKey(code, modifier, false);
  }
  function typeText(text) {
    for (const ch of text.replace(/\u2026/g, "...")) {
      const k = US_KEYS[ch];
      if (k) { tapKey(k[0], k[1] ? MOD.ShiftLeft : 0); continue; }
      if (!warnedNonAscii) {
        warnedNonAscii = true;
        setStatus("The phone keyboard types as a US keyboard. For other scripts, " +
                  "switch the firmware's layout with Ctrl+L and type the Latin keys.");
      }
    }
  }

  function proxyInput() {
    const now = proxy.value;
    let i = 0;
    while (i < proxyLast.length && i < now.length && proxyLast[i] === now[i]) i++;
    for (let n = proxyLast.length - i; n > 0; n--) tapKey(HID.Backspace, 0);
    typeText(now.slice(i));
    proxyLast = now;
    /* Keep the field short and the pad intact, but never in the middle
     * of a word the keyboard is still composing. */
    if (!composing) proxyReset();
  }

  proxy.addEventListener("input", proxyInput);
  proxy.addEventListener("compositionstart", () => { composing = true; });
  proxy.addEventListener("compositionend", () => {
    composing = false;
    /* Some browsers fire input after compositionend, some before. */
    setTimeout(() => { if (!composing && proxy.value !== PAD) proxyInput(); }, 0);
  });
  proxy.addEventListener("focus", () => setActive(true));
  proxy.addEventListener("blur", () => setActive(false));
  setTouchMode(touchMode);

  /* On-page buttons for the shortcuts browsers keep for themselves (and
   * the keys a phone keyboard lacks). They leave the focus where it is,
   * so a phone keyboard stays up. */
  for (const btn of document.querySelectorAll(".keys button")) {
    btn.addEventListener("mousedown", (e) => e.preventDefault());
    btn.addEventListener("pointerdown", (e) => { if (e.pointerType !== "mouse") e.preventDefault(); });
    btn.addEventListener("click", () => {
      const parts = btn.dataset.key.split("+");
      const code = HID[parts.pop()];
      let m = 0;
      for (const p of parts) if (p === "Ctrl") m |= MOD.ControlLeft;
      tapKey(code, m);
      if (!active) focusScreen();
    });
  }

  /* ---- the board's buttons ------------------------------------------- */

  function addBoardButtons() {
    const box = document.getElementById("side-buttons");
    for (const b of board.buttons) {
      const btn = document.createElement("button");
      btn.type = "button";
      btn.className = "hw-button";
      btn.textContent = b.label;
      btn.title = b.title;
      const press = (down) => {
        if (running && !halted) Module._demo_button(b.id, down ? 1 : 0);
      };
      btn.addEventListener("pointerdown", (e) => {
        e.preventDefault();
        btn.setPointerCapture(e.pointerId);
        btn.classList.add("down");
        press(true);
      });
      const up = () => {
        if (!btn.classList.contains("down")) return;
        btn.classList.remove("down");
        press(false);
        if (!active) focusScreen();
      };
      btn.addEventListener("pointerup", up);
      btn.addEventListener("pointercancel", up);
      box.appendChild(btn);
    }
  }

  /* ---- touch -------------------------------------------------------- */

  function pointerPos(e) {
    const r = canvas.getBoundingClientRect();
    let x = Math.floor((e.clientX - r.left) / r.width * screenW);
    let y = Math.floor((e.clientY - r.top) / r.height * screenH);
    if (upsideDown) { x = screenW - 1 - x; y = screenH - 1 - y; }
    return [x, y];
  }

  let pointerDown = false;
  canvas.addEventListener("pointerdown", (e) => {
    if (!running || halted || e.button > 0) return;
    if (e.pointerType === "mouse") canvas.focus();
    else if (!touchMode) setTouchMode(true);
    pointerDown = true;
    canvas.setPointerCapture(e.pointerId);
    const [x, y] = pointerPos(e);
    Module._demo_pointer(x, y, 1);
    e.preventDefault();
  });
  canvas.addEventListener("pointermove", (e) => {
    if (!pointerDown) return;
    const [x, y] = pointerPos(e);
    Module._demo_pointer(x, y, 1);
  });
  const pointerUp = (e) => {
    if (!pointerDown) return;
    pointerDown = false;
    /* A finger or pen: bring up the on-screen keyboard. Done on release,
     * which iOS counts as a user gesture that may open it. */
    if (e.pointerType !== "mouse" && e.type === "pointerup") focusScreen();
    const [x, y] = pointerPos(e);
    Module._demo_pointer(x, y, 0);
  };
  canvas.addEventListener("pointerup", pointerUp);
  canvas.addEventListener("pointercancel", pointerUp);
  canvas.addEventListener("contextmenu", (e) => e.preventDefault());

  /* ---- page --------------------------------------------------------- */

  /* Start a fresh demo: every document and setting of every screen and
   * language pack, and the remembered choice, go. The databases are
   * deleted at the next start, before anything opens them. */
  document.getElementById("reset").addEventListener("click", () => {
    if (!confirm("Delete all of the demo's documents and settings in this browser " +
                 "and start a fresh demo?")) return;
    halted = true;
    running = false;   /* no more saving */
    lsSet(RESET_FLAG, "1");
    lsSet(CHOICE_KEY, null);
    showOverlay("Resetting\u2026");
    location.href = location.pathname;
  });

  window.addEventListener("resize", fitScreen);
  setInterval(() => { if (!halted) syncStorage(); }, 3000);
  document.addEventListener("visibilitychange", () => {
    if (document.visibilityState === "hidden") syncStorage();
  });
  window.addEventListener("pagehide", () => syncStorage());

  /* ---- start ----------------------------------------------------------- */

  if (built.size === 0) {
    setStatus("No firmware has been built (run make in online_demo/).");
    return;
  }

  fillPicker();
  addBoardButtons();
  fitScreen();

  const base = "fw/" + variant.board + "/" + variant.pack + "/";
  const build = (document.querySelector('script[src^="demo.js"]').src.split("?v=")[1]) || "";

  Object.assign(Module, {
    preRun: [mountStorage],
    locateFile: (path) => base + path + "?v=" + encodeURIComponent(build),
    onRuntimeInitialized: () => {
      running = true;
      lsSet(CHOICE_KEY, variant.board + "/" + variant.pack);
      setStatus(storageOk
        ? "Running " + board.name + " with the " + variant.pack +
          " language pack. Your documents are kept in this browser."
        : "Running, but this browser does not allow storage: changes are lost on reload.");
      if (!touchMode) canvas.focus();
    },
    print: (text) => console.log(text),
    printErr: (text) => console.warn(text),
    onAbort: (what) => {
      halted = true;
      showOverlay("The firmware stopped unexpectedly (" + what + "). Reload the page to restart it.",
                  () => location.reload());
    },
  });

  const script = document.createElement("script");
  script.src = base + "draftling.js?v=" + encodeURIComponent(build);
  script.onerror = () => setStatus("Could not load " + script.src);
  document.body.appendChild(script);
})();
