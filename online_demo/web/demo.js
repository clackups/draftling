/*
 * Draftling online demo -- the page side of the simulated device.
 *
 * - Storage: the simulated SD card (/sdcard) and the settings store
 *   (/persist, holding NVS) are Emscripten IDBFS mounts, loaded from
 *   IndexedDB before the firmware starts and written back every few
 *   seconds, when the firmware asks (src/demo_host.c), and when the
 *   page is hidden.
 * - Screen: the firmware hands over changed RGB565 rectangles of its
 *   frame buffer (Module.demoPresent); they are drawn 1:1 into the
 *   canvas, which CSS scales up with crisp pixels.
 * - Keyboard: KeyboardEvent.code (the physical key) -> HID usage code,
 *   plus the HID modifier byte, delivered like the BLE keyboard does.
 *   The firmware repeats held keys itself, so browser auto-repeat is
 *   ignored. Keyboard layouts are applied by the firmware (Ctrl+L).
 * - Touch: mouse / pen / finger on the canvas -> LVGL pointer.
 */
"use strict";

(function () {
  /* Emscripten's draftling.js adopts this object as its Module. */
  const Module = window.Module = window.Module || {};

  const RESET_FLAG = "draftling-demo-reset";
  const IDB_NAMES = ["/sdcard", "/persist"];

  const canvas = document.getElementById("screen");
  const ctx = canvas.getContext("2d", { alpha: false });
  const device = document.getElementById("device");
  const overlay = document.getElementById("overlay");
  const overlayText = document.getElementById("overlay-text");
  const statusEl = document.getElementById("status");
  const focusHint = document.getElementById("focus-hint");

  let screenW = 480, screenH = 320, screenScale = 1, upsideDown = false;
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

  /* ---- storage ---------------------------------------------------- */

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
    const IDBFS = Module.IDBFS;
    for (const dir of IDB_NAMES) {
      FS.mkdir(dir);
      FS.mount(IDBFS, {}, dir);
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
    let reset = false;
    try { reset = localStorage.getItem(RESET_FLAG) === "1"; } catch (e) { /* ignore */ }
    if (reset) {
      try { localStorage.removeItem(RESET_FLAG); } catch (e) { /* ignore */ }
      deleteDatabases().then(finish);
    } else {
      finish();
    }
  }

  /* ---- screen ----------------------------------------------------- */

  let image = null;

  function fitScreen() {
    /* As large as the window allows, up to 3x. */
    const physW = screenW * screenScale, physH = screenH * screenScale;
    let chrome = 0;   /* horizontal padding of the board and the bezel */
    for (const el of [device, device.querySelector(".bezel")]) {
      const cs = getComputedStyle(el);
      chrome += parseFloat(cs.paddingLeft) + parseFloat(cs.paddingRight);
    }
    const avail = device.parentElement.clientWidth - chrome;
    const maxH = Math.max(window.innerHeight * 0.75, 240);
    let zoom = Math.min(avail / physW, maxH / physH, 3);
    /* Whole multiples keep every panel pixel the same size; below 2x
     * a fractional zoom that fills the space reads better. */
    if (zoom >= 2) zoom = Math.floor(zoom);
    canvas.style.width = Math.round(physW * zoom) + "px";
    canvas.style.height = Math.round(physH * zoom) + "px";
  }

  Module.demoSetScreen = function (w, h, scale, flip) {
    screenW = w; screenH = h; screenScale = scale; upsideDown = !!flip;
    canvas.width = w;
    canvas.height = h;
    canvas.style.transform = upsideDown ? "rotate(180deg)" : "";
    image = ctx.createImageData(w, h);
    fitScreen();
  };

  Module.demoPresent = function (fbPtr, fbW, x, y, w, h) {
    if (!image) return;
    const src = Module.HEAPU16;
    const dst = image.data;
    const base = fbPtr >> 1;
    for (let row = y; row < y + h; row++) {
      let s = base + row * fbW + x;
      let d = (row * screenW + x) * 4;
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

  Module.demoSetBacklight = function (percent) {
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
    canvas.style.filter = "brightness(0)";
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

  /* On-page buttons for the shortcuts browsers keep for themselves. */
  for (const btn of document.querySelectorAll(".keys button")) {
    btn.addEventListener("mousedown", (e) => e.preventDefault());   /* keep focus on the screen */
    btn.addEventListener("click", () => {
      const parts = btn.dataset.key.split("+");
      const code = HID[parts.pop()];
      let m = 0;
      for (const p of parts) if (p === "Ctrl") m |= MOD.ControlLeft;
      sendKey(code, m, true);
      sendKey(code, m, false);
      canvas.focus();
    });
  }

  /* ---- touch ---------------------------------------------------------- */

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
    canvas.focus();
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
    const [x, y] = pointerPos(e);
    Module._demo_pointer(x, y, 0);
  };
  canvas.addEventListener("pointerup", pointerUp);
  canvas.addEventListener("pointercancel", pointerUp);
  canvas.addEventListener("contextmenu", (e) => e.preventDefault());

  /* ---- page ----------------------------------------------------------- */

  document.getElementById("reset").addEventListener("click", () => {
    if (!confirm("Delete the demo's documents and settings in this browser and start over?")) return;
    halted = true;
    try { localStorage.setItem(RESET_FLAG, "1"); } catch (e) { /* ignore */ }
    showOverlay("Resetting\u2026");
    location.reload();
  });

  window.addEventListener("resize", fitScreen);
  setInterval(() => { if (!halted) syncStorage(); }, 3000);
  document.addEventListener("visibilitychange", () => {
    if (document.visibilityState === "hidden") syncStorage();
  });
  window.addEventListener("pagehide", () => syncStorage());

  fitScreen();

  /* ---- Emscripten module ---------------------------------------------- */

  Object.assign(Module, {
    preRun: [mountStorage],
    locateFile: (path) => path + "?v=" + encodeURIComponent(
      (document.querySelector('script[src^="draftling.js"]').src.split("?v=")[1]) || ""),
    onRuntimeInitialized: () => {
      running = true;
      setStatus(storageOk
        ? "Running. Your documents are kept in this browser."
        : "Running, but this browser does not allow storage: changes are lost on reload.");
      canvas.focus();
    },
    print: (text) => console.log(text),
    printErr: (text) => console.warn(text),
    onAbort: (what) => {
      halted = true;
      showOverlay("The firmware stopped unexpectedly (" + what + "). Reload the page to restart it.",
                  () => location.reload());
    },
  });
})();
