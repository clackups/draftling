/*
 * Draftling web client -- the page side.
 *
 * The program (draftling.js / draftling.wasm, built by client.mk) runs
 * on Web Workers; this script owns the page:
 *
 * - Start: cross-origin isolation (needed for the program's threads;
 *   sw.js adds the headers when the server does not), a single running
 *   copy per browser profile (Web Locks: two would write the same
 *   files), persistent storage (navigator.storage.persist(): without it
 *   the browser may delete the documents when the disk runs low, so
 *   Draftling does not start until it is granted), then the program.
 * - Screen: a canvas the size of the window in device pixels; the
 *   program draws changed rectangles (Module.webPresent) and lays its
 *   screens out again when the size changes (WEB_EV.RESIZE).
 * - Keyboard: KeyboardEvent.code (the physical key) -> Linux key code,
 *   as the Linux program gets them from Wayland / X11; the program
 *   applies its own layouts (Ctrl+L), dead keys, input methods and key
 *   repeat. The browser's repeats are dropped.
 * - Mouse / pen / touch on the canvas -> the program's pointer; the
 *   wheel moves the cursor.
 * - Phones and tablets: a canvas brings up no keyboard, a text field
 *   does, so a tap focuses a hidden textarea. A Bluetooth keyboard's
 *   keys then arrive as ordinary key events; what an on-screen
 *   keyboard types is read back from the field and sent as US-keyboard
 *   keys. A key bar adds the keys a phone keyboard lacks.
 * - Clipboard: Ctrl+C / Ctrl+X put the text on the system clipboard,
 *   Ctrl+V pastes what is there (the browser's paste event, or
 *   navigator.clipboard.readText()).
 * - Files panel (F9): download documents, a project or everything as
 *   a ZIP; upload documents into a project.
 * - Full screen (F11), with the keyboard locked where the browser
 *   offers it, so Esc and the browser's own shortcuts reach Draftling.
 * - Leaving or hiding the page saves the documents; leaving with
 *   unsaved changes asks first.
 */
"use strict";

(function () {
  const BUILD = window.DRAFTLING_BUILD || "dev";

  /* Keep in step with web_client/src/web_host.h. */
  const WEB_EV = {
    KEY: 1, POINTER: 2, WHEEL: 3, RESIZE: 4, RELEASE_ALL: 5,
    PASTE: 6, SAVE: 7, DAMAGE: 8,
  };
  /* Keep in step with files_web.c. */
  const FILES_OP = { LIST: 1, READ: 2, ZIP: 3, WRITE: 4, MKPROJECT: 5 };
  /* LINUX_MOD_* in linux/src/linux_host.h. */
  const MOD = { SHIFT: 1, CTRL: 2, ALT: 4, ALTGR: 8, SUPER: 16 };

  const ua = navigator.userAgent;
  const IS_MAC = /Mac|iPhone|iPad/.test(navigator.platform || "") || /Mac OS X/.test(ua);
  const IS_SAFARI = /^((?!chrome|chromium|android|crios|fxios|edg).)*safari/i.test(ua);
  const IS_FIREFOX = /firefox/i.test(ua);

  const canvas = document.getElementById("canvas");
  const screenEl = document.getElementById("screen");
  const ctx = canvas.getContext("2d", { alpha: false });
  const messageEl = document.getElementById("message");
  const messageTitle = document.getElementById("message-title");
  const messageBody = document.getElementById("message-body");
  const messageActions = document.getElementById("message-actions");
  const toolbar = document.getElementById("toolbar");

  let ready = false;          /* the program is running */
  let failed = false;
  let screenW = 0, screenH = 0;   /* window size in device pixels */

  /* ---- messages (start-up and errors) --------------------------------- */

  function showMessage(title, html, actions) {
    messageTitle.textContent = title;
    messageBody.innerHTML = html;
    messageActions.textContent = "";
    for (const a of actions || []) {
      const b = document.createElement("button");
      b.type = "button";
      b.textContent = a.label;
      if (a.primary) b.className = "primary";
      b.addEventListener("click", a.onClick);
      messageActions.appendChild(b);
    }
    messageEl.hidden = false;
    const first = messageActions.querySelector("button");
    if (first) first.focus();
  }

  function fail(title, html) {
    failed = true;
    showMessage(title, html, [{ label: "Reload", onClick: () => location.reload() }]);
  }

  /* ---- start ------------------------------------------------------------ */

  async function isolate() {
    if (window.crossOriginIsolated) return true;
    if (!("serviceWorker" in navigator)) return false;
    /* The service worker adds the isolation headers. A page it did not
     * load yet is reloaded once, through it. */
    try {
      await navigator.serviceWorker.register("sw.js");
      await navigator.serviceWorker.ready;
    } catch (e) {
      console.warn("Draftling: service worker:", e);
      return false;
    }
    let tried = null;
    try { tried = sessionStorage.getItem("draftling-isolate"); } catch (e) { /* ignore */ }
    if (tried === BUILD) return false;
    try { sessionStorage.setItem("draftling-isolate", BUILD); } catch (e) { /* ignore */ }
    location.reload();
    return new Promise(() => {});   /* the reload takes over */
  }

  function missingFeatures() {
    const missing = [];
    if (typeof SharedArrayBuffer === "undefined" || !window.crossOriginIsolated)
      missing.push("cross-origin isolation (SharedArrayBuffer)");
    if (!navigator.storage || !navigator.storage.getDirectory)
      missing.push("the origin-private file system");
    if (!navigator.locks) missing.push("Web Locks");
    if (typeof WebAssembly !== "object") missing.push("WebAssembly");
    return missing;
  }

  function persistHelp() {
    if (IS_FIREFOX) {
      return "<p>Firefox asks whether this site may store data persistently: allow it, " +
        "then press <b>Try again</b>. If it did not ask, open the site information " +
        "(the icon left of the address) and allow <i>Store data in persistent storage</i>.</p>";
    }
    if (IS_SAFARI) {
      return "<p>Safari grants it to a site added to the Dock (File &rarr; Add to Dock) " +
        "or to the Home Screen. Open Draftling from there, then press <b>Try again</b>.</p>";
    }
    return "<p>Chrome and Edge grant it to a site installed as an app, or one you use " +
      "often. Install Draftling (the install icon in the address bar, or the browser " +
      "menu &rarr; <i>Install Draftling</i>) and open it from there, or bookmark this page; " +
      "then press <b>Try again</b>.</p>";
  }

  async function ensurePersistent() {
    for (;;) {
      let ok = false;
      try {
        ok = await navigator.storage.persisted() || await navigator.storage.persist();
      } catch (e) {
        console.warn("Draftling: persist():", e);
      }
      if (ok) return;
      await new Promise((resolve) => {
        showMessage("Draftling needs persistent storage",
          "<p>Draftling keeps your documents in this browser. Unless the browser marks " +
          "that storage as persistent, it may delete it, documents included, when the " +
          "disk runs low or the site has not been used for a while.</p>" + persistHelp() +
          "<p>Synchronize with a Git repository or download your documents regularly " +
          "(F9) to keep copies elsewhere as well.</p>",
          [{ label: "Try again", primary: true, onClick: resolve }]);
      });
      showMessage("Draftling", "<p>Checking storage...</p>", []);
    }
  }

  async function storageEstimate() {
    try {
      const e = await navigator.storage.estimate();
      return [e.quota || 0, e.usage || 0];
    } catch (e) {
      return [0, 0];
    }
  }

  async function start() {
    if (!await isolate() || missingFeatures().length) {
      const m = missingFeatures();
      fail("This browser cannot run Draftling",
        "<p>Draftling needs " + (m.length ? m.join(", ") : "cross-origin isolation") +
        ". Use a current Chrome, Edge, Firefox or Safari, and not a private window.</p>");
      return;
    }
    /* One Draftling per browser profile: hold the lock while it runs. */
    const got = await new Promise((resolve) => {
      navigator.locks.request("draftling-client", { ifAvailable: true }, (lock) => {
        resolve(!!lock);
        return lock ? new Promise(() => {}) : undefined;
      });
    });
    if (!got) {
      fail("Draftling is already open",
        "<p>Draftling is open in another tab or window of this browser. Use that one: " +
        "two would overwrite each other's documents.</p>");
      return;
    }
    await ensurePersistent();
    showMessage("Draftling", "<p>Loading...</p>", []);
    const est = await storageEstimate();
    measureScreen();
    loadProgram(est);
  }

  /* ---- the program --------------------------------------------------- */

  const Module = window.Module = {
    locateFile: (path) => path + "?v=" + BUILD,
    mainScriptUrlOrBlob: "draftling.js?v=" + BUILD,
    webLogLevel: ({ error: 1, warn: 2, info: 3, debug: 4 })[
      new URLSearchParams(location.search).get("log")] || 0,
    print: (s) => console.log(s),
    printErr: (s) => console.warn(s),
    webSize: () => [screenW, screenH],
    webPresent: present,
    webClipSet: clipSet,
    webClipRequest: clipRequest,
    webFilesReply: filesReply,
    webReady: () => {
      ready = true;
      messageEl.hidden = true;
      focusScreen();
      if (touchMode) toast("Tap the screen to type.");
      else showToolbar();
    },
    webFatal: (msg) => fail("Draftling stopped", "<p>" + escapeHtml(msg) + "</p>"),
    webRestart: () => location.reload(),
    onAbort: (what) => fail("Draftling stopped",
      "<p>The program ended unexpectedly" + (what ? ": " + escapeHtml(String(what)) : "") +
      ". Your saved documents are kept.</p>"),
  };

  function loadProgram(est) {
    Module.onRuntimeInitialized = () => {
      Module._web_storage(est[0], est[1]);
      setInterval(async () => {
        const e = await storageEstimate();
        if (Module._web_storage) Module._web_storage(e[0], e[1]);
      }, 30000);
    };
    const s = document.createElement("script");
    s.src = "draftling.js?v=" + BUILD;
    s.onerror = () => fail("Draftling could not be loaded",
      "<p>The program's files did not load. Check the connection and reload.</p>");
    document.body.appendChild(s);
  }

  function send(type, a, b, c) {
    if (ready && Module._web_input) Module._web_input(type, a | 0, b | 0, c | 0);
  }

  function escapeHtml(s) {
    return s.replace(/[&<>"']/g, (c) => ({ "&": "&amp;", "<": "&lt;", ">": "&gt;",
      "\"": "&quot;", "'": "&#39;" })[c]);
  }

  /* ---- screen --------------------------------------------------------- */

  let frameW = 0, frameH = 0;
  let resizeTimer = 0;

  function setScreen(w, h) {
    w = Math.max(1, Math.round(w));
    h = Math.max(1, Math.round(h));
    if (w === screenW && h === screenH) return;
    screenW = w;
    screenH = h;
    sizeCanvasCss();
    clearTimeout(resizeTimer);
    resizeTimer = setTimeout(() => send(WEB_EV.RESIZE, screenW, screenH), 30);
  }

  function measureScreen() {
    const r = screenEl.getBoundingClientRect();
    const dpr = window.devicePixelRatio || 1;
    setScreen(r.width * dpr, r.height * dpr);
  }

  /* The window's size in device pixels: exact where the browser reports
   * it, so every canvas pixel is one screen pixel. */
  try {
    new ResizeObserver((entries) => {
      for (const e of entries) {
        if (e.devicePixelContentBoxSize && e.devicePixelContentBoxSize[0]) {
          setScreen(e.devicePixelContentBoxSize[0].inlineSize,
            e.devicePixelContentBoxSize[0].blockSize);
        } else {
          measureScreen();
        }
      }
    }).observe(screenEl, { box: "device-pixel-content-box" });
  } catch (e) {
    new ResizeObserver(measureScreen).observe(screenEl);
  }
  window.addEventListener("resize", measureScreen);

  /* One frame pixel on the page: the ratio of the screen area's CSS size
   * to the device-pixel size reported for it, so the canvas fills it
   * exactly even where the observer's pixel ratio differs from
   * window.devicePixelRatio (browser zoom, emulators). */
  function sizeCanvasCss() {
    if (!frameW) return;
    const r = screenEl.getBoundingClientRect();
    let kx = screenW && r.width ? r.width / screenW : 0;
    let ky = screenH && r.height ? r.height / screenH : 0;
    if (!kx || !ky) kx = ky = 1 / (window.devicePixelRatio || 1);
    canvas.style.width = (frameW * kx) + "px";
    canvas.style.height = (frameH * ky) + "px";
  }

  function present(fw, fh, x, y, w, h, bytes) {
    if (fw !== frameW || fh !== frameH || canvas.width !== fw || canvas.height !== fh) {
      frameW = fw;
      frameH = fh;
      canvas.width = fw;
      canvas.height = fh;
      sizeCanvasCss();
      if (x !== 0 || y !== 0 || w !== fw || h !== fh) {
        /* A new canvas is blank: have the whole frame sent again. */
        send(WEB_EV.DAMAGE, 0, 0, 0);
      }
    }
    if (!w || !h) return;
    ctx.putImageData(new ImageData(new Uint8ClampedArray(bytes.buffer, 0, w * h * 4), w, h), x, y);
  }

  /* ---- keyboard ---------------------------------------------------------- */

  /* KeyboardEvent.code -> Linux input event code (linux/input-event-codes.h). */
  const KEYS = {
    Escape: 1, Digit1: 2, Digit2: 3, Digit3: 4, Digit4: 5, Digit5: 6, Digit6: 7,
    Digit7: 8, Digit8: 9, Digit9: 10, Digit0: 11, Minus: 12, Equal: 13, Backspace: 14,
    Tab: 15, KeyQ: 16, KeyW: 17, KeyE: 18, KeyR: 19, KeyT: 20, KeyY: 21, KeyU: 22,
    KeyI: 23, KeyO: 24, KeyP: 25, BracketLeft: 26, BracketRight: 27, Enter: 28,
    ControlLeft: 29, KeyA: 30, KeyS: 31, KeyD: 32, KeyF: 33, KeyG: 34, KeyH: 35,
    KeyJ: 36, KeyK: 37, KeyL: 38, Semicolon: 39, Quote: 40, Backquote: 41,
    ShiftLeft: 42, Backslash: 43, IntlHash: 43, KeyZ: 44, KeyX: 45, KeyC: 46, KeyV: 47,
    KeyB: 48, KeyN: 49, KeyM: 50, Comma: 51, Period: 52, Slash: 53, ShiftRight: 54,
    NumpadMultiply: 55, AltLeft: 56, Space: 57, CapsLock: 58,
    F1: 59, F2: 60, F3: 61, F4: 62, F5: 63, F6: 64, F7: 65, F8: 66, F9: 67, F10: 68,
    NumLock: 69, ScrollLock: 70, Numpad7: 71, Numpad8: 72, Numpad9: 73,
    NumpadSubtract: 74, Numpad4: 75, Numpad5: 76, Numpad6: 77, NumpadAdd: 78,
    Numpad1: 79, Numpad2: 80, Numpad3: 81, Numpad0: 82, NumpadDecimal: 83,
    IntlBackslash: 86, F11: 87, F12: 88, IntlRo: 89, NumpadEnter: 96,
    ControlRight: 97, NumpadDivide: 98, PrintScreen: 99, AltRight: 100,
    Home: 102, ArrowUp: 103, PageUp: 104, ArrowLeft: 105, ArrowRight: 106, End: 107,
    ArrowDown: 108, PageDown: 109, Insert: 110, Delete: 111, NumpadEqual: 117,
    Pause: 119, IntlYen: 124, MetaLeft: 125, MetaRight: 126, ContextMenu: 127,
  };
  /* On a Mac, Command is Draftling's Ctrl (Cmd+C, Cmd+V, Cmd+S...). */
  if (IS_MAC) {
    KEYS.MetaLeft = 29;
    KEYS.MetaRight = 97;
  }

  const MODIFIER_CODES = new Set([29, 42, 54, 56, 97, 100, 125, 126]);
  const down = new Set();        /* codes Draftling has been sent as down */
  /* Modifier keys whose own keydown the page saw, until their keyup (or
   * the page loses the focus). They count as held whatever later events
   * say: Android does not always set altKey / ctrlKey on the next key's
   * event for a Bluetooth keyboard, and Alt+I then arrived as a plain
   * I. */
  const realDown = new Set();
  let ctrlLeftAt = -1;           /* Windows AltGr: the ControlLeft it fakes */
  let imeKeyAt = -1e9;           /* last keydown the input method took */
  let fakeCtrl = false;

  /* The modifiers held according to the page's own record. */
  function heldMods() {
    let m = 0;
    if (realDown.has(42) || realDown.has(54)) m |= MOD.SHIFT;
    if ((realDown.has(29) && !fakeCtrl) || realDown.has(97)) m |= MOD.CTRL;
    if (realDown.has(56) || realDown.has(100)) m |= MOD.ALT;
    if (realDown.has(125) || realDown.has(126)) m |= MOD.SUPER;
    return m;
  }

  function mods(e) {
    let m = heldMods();
    if (!e) return m;
    if (e.shiftKey) m |= MOD.SHIFT;
    if ((e.ctrlKey && !fakeCtrl) || (IS_MAC && e.metaKey)) m |= MOD.CTRL;
    if (e.altKey) m |= MOD.ALT;
    if (e.getModifierState && e.getModifierState("AltGraph")) m |= MOD.ALTGR;
    if (e.metaKey && !IS_MAC) m |= MOD.SUPER;
    return m;
  }

  function sendKey(code, pressed, e) {
    if (pressed) down.add(code);
    else down.delete(code);
    send(WEB_EV.KEY, code, pressed ? 1 : 0, mods(e));
  }

  /* A modifier the event says is held, but whose key went down before
   * the page had the focus (or in another window): press it now, so
   * Ctrl+S is Ctrl+S. Releases need no help -- the program drops a
   * modifier the event no longer reports (WEB_EV.KEY's modifier mask). */
  function catchUpModifiers(e) {
    const m = mods(e);
    const want = [
      [MOD.SHIFT, 42, 54],
      [MOD.CTRL, 29, 97],
      [MOD.ALT, 56, 100],
      [MOD.SUPER, 125, 126],
    ];
    for (const [bit, left, right] of want) {
      if (m & bit) {
        if (!down.has(left) && !down.has(right)) sendKey(left, true, e);
      } else if (!(bit === MOD.ALT && (m & MOD.ALTGR))) {
        /* Released where the page did not see it: the program drops it
         * (the modifier mask), and so does this record. */
        if (!realDown.has(left)) down.delete(left);
        if (!realDown.has(right)) down.delete(right);
      }
    }
  }

  function releaseAll() {
    down.clear();
    realDown.clear();
    fakeCtrl = false;
    send(WEB_EV.RELEASE_ALL, 0, 0, 0);
  }

  function isCtrlCombo(e) {
    return (e.ctrlKey && !fakeCtrl) || (IS_MAC && e.metaKey);
  }

  function onKeyDown(e) {
    if (!ready || !filesEl.hidden || !messageEl.hidden) return;
    hideToolbar();
    if (e.code === "F11") {
      e.preventDefault();
      if (!e.repeat) toggleFullscreen();
      return;
    }
    if (e.code === "F9") {
      e.preventDefault();
      if (!e.repeat) openFiles();
      return;
    }
    const code = KEYS[e.code];
    if (!code) {
      /* A key the input method takes (Android: dead keys such as
       * AltGr+I, keyCode 229): its keyup still carries the code, see
       * onKeyUp(). */
      if (e.keyCode === 229 || e.key === "Unidentified" || e.key === "Process")
        imeKeyAt = performance.now();
      return;
    }
    /* Down already: Android repeats a held key without setting
     * e.repeat. Draftling repeats keys itself. */
    if (down.has(code)) {
      e.preventDefault();
      return;
    }
    /* Ctrl+V: the browser's paste event brings the clipboard's text. */
    const paste = isCtrlCombo(e) && e.code === "KeyV";
    if (!paste) e.preventDefault();
    if (e.repeat) return;   /* Draftling repeats keys itself */
    if (paste) pasteKeyAt = performance.now();
    if (isCtrlCombo(e) && (e.code === "KeyC" || e.code === "KeyX")) prepareCopy();

    /* Windows sends AltGr as ControlLeft + AltRight: take the
     * ControlLeft back. */
    if (e.code === "ControlLeft") ctrlLeftAt = e.timeStamp;
    if (e.code === "AltRight" && down.has(29) && ctrlLeftAt >= 0 &&
        e.timeStamp - ctrlLeftAt < 50) {
      fakeCtrl = true;
      sendKey(29, false, e);
    }
    if (MODIFIER_CODES.has(code)) realDown.add(code);
    else catchUpModifiers(e);
    noteHardwareKey(e);
    sendKey(code, true, e);
  }

  function onKeyUp(e) {
    if (!ready) return;
    const code = KEYS[e.code];
    if (!code) return;
    e.preventDefault();
    /* The keyup of a physical key whose keydown the input method took
     * (Android hands AltGr+I to it as a dead key, keyCode 229): send
     * the key now, with the modifiers held, and drop what the input
     * method put into the field. */
    if (!down.has(code) && !MODIFIER_CODES.has(code) &&
        performance.now() - imeKeyAt < 3000) {
      imeKeyAt = -1e9;
      dropImeText();
      setHwKeyboard(true);
      const m = mods(e);
      send(WEB_EV.KEY, code, 1, m);
      send(WEB_EV.KEY, code, 0, m);
      return;
    }
    realDown.delete(code);
    if (e.code === "ControlLeft" && fakeCtrl) {
      fakeCtrl = false;
      if (!down.has(29)) return;
    }
    if (!down.has(code)) return;
    sendKey(code, false, e);
  }

  window.addEventListener("keydown", onKeyDown, true);
  window.addEventListener("keyup", onKeyUp, true);
  window.addEventListener("blur", () => { if (ready) releaseAll(); });

  /* ---- pointer -------------------------------------------------------- */

  function canvasPoint(e) {
    const r = canvas.getBoundingClientRect();
    const sx = r.width ? canvas.width / r.width : 1;
    const sy = r.height ? canvas.height / r.height : 1;
    return [Math.floor((e.clientX - r.left) * sx), Math.floor((e.clientY - r.top) * sy)];
  }

  let pointerDown = false;

  canvas.addEventListener("pointerdown", (e) => {
    if (!ready || e.button !== 0) return;
    e.preventDefault();
    if (e.pointerType === "mouse") canvas.focus();
    else if (!touchMode) setTouchMode(true);
    canvas.setPointerCapture(e.pointerId);
    pointerDown = true;
    const [x, y] = canvasPoint(e);
    send(WEB_EV.POINTER, x, y, 1);
  });
  canvas.addEventListener("pointermove", (e) => {
    if (!ready) return;
    const [x, y] = canvasPoint(e);
    send(WEB_EV.POINTER, x, y, pointerDown ? 1 : 0);
  });
  const pointerUp = (e) => {
    if (!ready || !pointerDown) return;
    pointerDown = false;
    const [x, y] = canvasPoint(e);
    send(WEB_EV.POINTER, x, y, 0);
    /* The end of a tap is a user gesture: focusing the text field now
     * brings up the on-screen keyboard. */
    if (touchMode && e.type === "pointerup" && document.activeElement !== proxy) focusScreen();
  };
  canvas.addEventListener("pointerup", pointerUp);
  canvas.addEventListener("pointercancel", pointerUp);
  canvas.addEventListener("contextmenu", (e) => e.preventDefault());

  let wheelAcc = 0;
  canvas.addEventListener("wheel", (e) => {
    if (!ready) return;
    e.preventDefault();
    /* One step = one mouse-wheel notch (three lines in Draftling). */
    const unit = e.deltaMode === 1 ? 3 : e.deltaMode === 2 ? 1 : 100;
    wheelAcc += e.deltaY / unit;
    const steps = wheelAcc > 0 ? Math.floor(wheelAcc) : Math.ceil(wheelAcc);
    if (steps) {
      wheelAcc -= steps;
      send(WEB_EV.WHEEL, steps, 0, 0);
    }
  }, { passive: false });

  /* ---- phones and tablets ------------------------------------------------ */

  /* A canvas never brings up the on-screen keyboard, a text field does
   * -- and Android also hands a Bluetooth keyboard's keys only to a
   * focused text field. In touch mode a tap on the screen focuses an
   * invisible textarea. Keys with a code (a hardware keyboard) still go
   * through onKeyDown(), which keeps them out of the field; on-screen
   * keyboards send no usable key events ("Unidentified", and their
   * predictive text rewrites whole words), so what they type is read
   * back from the field: the difference to the last value becomes
   * Backspaces and the US-keyboard keys of the new characters. */
  const proxy = document.getElementById("kbd-proxy");
  const keybar = document.getElementById("keybar");
  const appEl = document.getElementById("app");
  const toastEl = document.getElementById("toast");
  let touchMode = !!(window.matchMedia && matchMedia("(pointer: coarse)").matches);
  /* Spaces before the caret, so Backspace in an empty field still
   * deletes (and predictive text sees a word boundary). */
  const PAD = " ".repeat(16);
  let proxyLast = PAD;
  let composing = false;
  let stickyCtrl = false;
  let warnedNonUs = false;
  /* A physical keyboard is in use: its keys arrive with a code. The key
   * bar is then hidden, until an on-screen keyboard types again. */
  let hwKeyboard = false;
  /* Field text that may belong to a physical key the input method took
   * (see onKeyUp()): typed only if no such keyup follows in time. */
  const IME_WAIT_MS = 400;
  let imeTimer = 0;
  let suppressUntil = 0;
  let toastTimer = 0;

  function toast(msg, ms) {
    toastEl.textContent = msg;
    toastEl.hidden = false;
    clearTimeout(toastTimer);
    toastTimer = setTimeout(() => { toastEl.hidden = true; }, ms || 3500);
  }

  function setTouchMode(on) {
    touchMode = on;
    document.body.classList.toggle("touch", on);
    keybar.hidden = !on || hwKeyboard;
    if (on) toolbar.classList.remove("shown");
  }

  function setHwKeyboard(on) {
    if (on === hwKeyboard) return;
    hwKeyboard = on;
    keybar.hidden = !touchMode || hwKeyboard;
  }

  /* A key with a code that only a physical keyboard sends (on-screen
   * keyboards may send Enter / Backspace / Space with a code, but not
   * letters, digits, modifiers or arrows). */
  const SOFT_CODES = new Set(["Enter", "Backspace", "Space", "Tab", "NumpadEnter"]);
  function noteHardwareKey(e) {
    if (touchMode && !e.isComposing && e.keyCode !== 229 && !SOFT_CODES.has(e.code))
      setHwKeyboard(true);
  }

  function proxyReset() {
    proxy.value = PAD;
    proxyLast = PAD;
    try { proxy.setSelectionRange(PAD.length, PAD.length); } catch (e) { /* ignore */ }
  }

  /* The keys go to the canvas, or on touch devices to the text field
   * (focused again, so a dismissed keyboard comes back). */
  function focusScreen(reshow) {
    if (!touchMode) {
      canvas.focus();
      return;
    }
    if (reshow && document.activeElement === proxy) proxy.blur();
    proxyReset();
    proxy.focus({ preventScroll: true });
  }

  /* The app follows the visual viewport, which the on-screen keyboard
   * shortens (Safari does not resize the page for it). */
  function fitViewport() {
    const vv = window.visualViewport;
    if (!vv) return;
    appEl.style.top = vv.offsetTop + "px";
    appEl.style.height = vv.height + "px";
  }
  if (window.visualViewport) {
    visualViewport.addEventListener("resize", fitViewport);
    visualViewport.addEventListener("scroll", fitViewport);
  }

  /* A US keyboard: character -> [key code, Shift]. */
  const US_KEYS = {};
  (function () {
    for (let i = 0; i < 26; i++) {
      const c = KEYS["Key" + String.fromCharCode(65 + i)];
      US_KEYS[String.fromCharCode(97 + i)] = [c, false];
      US_KEYS[String.fromCharCode(65 + i)] = [c, true];
    }
    const digits = "1234567890", shifted = "!@#$%^&*()";
    for (let i = 0; i < 10; i++) {
      const c = KEYS["Digit" + digits[i]];
      US_KEYS[digits[i]] = [c, false];
      US_KEYS[shifted[i]] = [c, true];
    }
    const pairs = [
      ["-", "_", "Minus"], ["=", "+", "Equal"], ["[", "{", "BracketLeft"],
      ["]", "}", "BracketRight"], ["\\", "|", "Backslash"], [";", ":", "Semicolon"],
      ["'", "\"", "Quote"], ["`", "~", "Backquote"], [",", "<", "Comma"],
      [".", ">", "Period"], ["/", "?", "Slash"],
    ];
    for (const [plain, shift, name] of pairs) {
      US_KEYS[plain] = [KEYS[name], false];
      US_KEYS[shift] = [KEYS[name], true];
    }
    US_KEYS[" "] = [KEYS.Space, false];
    US_KEYS["\n"] = [KEYS.Enter, false];
    US_KEYS["\t"] = [KEYS.Tab, false];
    /* Typographic characters phone keyboards put in by themselves. */
    const alias = {
      "\u2018": "'", "\u2019": "'", "\u201c": "\"", "\u201d": "\"",
      "\u2013": "-", "\u2014": "-", "\u00a0": " ",
    };
    for (const [from, to] of Object.entries(alias)) US_KEYS[from] = US_KEYS[to];
  })();

  function setStickyCtrl(on) {
    stickyCtrl = on;
    document.getElementById("kb-ctrl").setAttribute("aria-pressed", on ? "true" : "false");
  }

  /* Press and release one key, with Shift and the key bar's Ctrl as
   * asked; the modifiers go down and up around it. */
  function tapKey(code, shift) {
    const ctrl = stickyCtrl;
    /* Modifiers of a physical keyboard held meanwhile (Android may turn
     * Alt + a letter into text in the field instead of a key event). */
    const m = (shift ? MOD.SHIFT : 0) | (ctrl ? MOD.CTRL : 0) | heldMods();
    if (ctrl) send(WEB_EV.KEY, 29, 1, m);
    if (shift) send(WEB_EV.KEY, 42, 1, m);
    send(WEB_EV.KEY, code, 1, m);
    send(WEB_EV.KEY, code, 0, m);
    if (shift) send(WEB_EV.KEY, 42, 0, m & ~MOD.SHIFT);
    if (ctrl) {
      send(WEB_EV.KEY, 29, 0, 0);
      setStickyCtrl(false);
      if (code === KEYS.KeyC || code === KEYS.KeyX) prepareCopy();
    }
  }

  function typeText(text) {
    for (const ch of text.replace(/\u2026/g, "...")) {
      const k = US_KEYS[ch];
      if (k) {
        tapKey(k[0], k[1]);
      } else if (!warnedNonUs) {
        warnedNonUs = true;
        toast("The on-screen keyboard types as a US keyboard. For other scripts, " +
              "turn on a layout in F1 -> Settings -> Keyboard and switch with Ctrl+L.", 7000);
      }
    }
  }

  /* Throw away what the input method put into the field for a physical
   * key, and end its composition (an Android dead key would otherwise
   * turn the next letter into an accented one). */
  function dropImeText() {
    clearTimeout(imeTimer);
    imeTimer = 0;
    suppressUntil = performance.now() + 150;
    composing = false;
    proxy.blur();
    proxyReset();
    proxy.focus({ preventScroll: true });
  }

  function proxyInput() {
    if (!ready) return;
    if (performance.now() < suppressUntil) {
      if (!composing) proxyReset();
      return;
    }
    /* With a physical keyboard in use, text right after a key the input
     * method took may be that key's: wait for its keyup first. */
    if (hwKeyboard && performance.now() - imeKeyAt < IME_WAIT_MS) {
      clearTimeout(imeTimer);
      imeTimer = setTimeout(() => {
        imeTimer = 0;
        imeKeyAt = -1e9;
        proxyInput();
      }, IME_WAIT_MS);
      return;
    }
    const now = proxy.value;
    /* Text without key events while no physical modifier is held: an
     * on-screen keyboard typed it, so it is in use again. */
    if (now !== proxyLast && !heldMods()) setHwKeyboard(false);
    let i = 0;
    while (i < proxyLast.length && i < now.length && proxyLast[i] === now[i]) i++;
    for (let n = proxyLast.length - i; n > 0; n--) tapKey(KEYS.Backspace, false);
    typeText(now.slice(i));
    proxyLast = now;
    /* Keep the field short and the pad whole, but never in the middle
     * of a word the keyboard is still composing. */
    if (!composing) proxyReset();
  }

  proxy.addEventListener("input", proxyInput);
  proxy.addEventListener("compositionstart", () => { composing = true; });
  proxy.addEventListener("compositionend", () => {
    composing = false;
    /* Some browsers fire input after compositionend, some before. */
    setTimeout(() => { if (!composing && proxy.value !== proxyLast) proxyInput(); }, 0);
  });

  /* The key bar's buttons leave the focus where it is, so the phone
   * keyboard stays up. */
  for (const btn of keybar.querySelectorAll("button")) {
    btn.addEventListener("pointerdown", (e) => e.preventDefault());
    btn.addEventListener("mousedown", (e) => e.preventDefault());
  }
  for (const btn of keybar.querySelectorAll("button[data-key]")) {
    btn.addEventListener("click", () => {
      if (!ready) return;
      const code = KEYS[btn.dataset.key];
      if (code) tapKey(code, false);
    });
  }
  document.getElementById("kb-ctrl").addEventListener("click", () => setStickyCtrl(!stickyCtrl));
  document.getElementById("kb-files").addEventListener("click", () => openFiles());
  document.getElementById("kb-full").addEventListener("click", () => toggleFullscreen());
  document.getElementById("kb-show").addEventListener("click", () => focusScreen(true));

  setTouchMode(touchMode);

  /* ---- key log (?keylog) ------------------------------------------------- */

  /* For finding out what a browser sends for a key (an Android Bluetooth
   * keyboard, say): with ?keylog in the address, the last events are
   * listed over the screen. */
  if (new URLSearchParams(location.search).has("keylog")) {
    const box = document.createElement("pre");
    box.id = "keylog";
    document.body.appendChild(box);
    const lines = [];
    const add = (text) => {
      lines.push(text);
      while (lines.length > 12) lines.shift();
      box.textContent = lines.join("\n");
    };
    const flags = (e) => (e.shiftKey ? "S" : "-") + (e.ctrlKey ? "C" : "-") +
      (e.altKey ? "A" : "-") + (e.metaKey ? "M" : "-") +
      (e.getModifierState && e.getModifierState("AltGraph") ? "G" : "-");
    for (const type of ["keydown", "keyup"]) {
      window.addEventListener(type, (e) => add(type + " code=" + JSON.stringify(e.code) +
        " key=" + JSON.stringify(e.key) + " keyCode=" + e.keyCode + " " + flags(e) +
        (e.isComposing ? " composing" : "") + (e.repeat ? " repeat" : "")), true);
    }
    for (const type of ["beforeinput", "input", "compositionstart", "compositionend"]) {
      proxy.addEventListener(type, (e) => add(type +
        (e.inputType ? " " + e.inputType : "") + " data=" + JSON.stringify(e.data) +
        " held=" + heldMods()), true);
    }
  }

  /* ---- clipboard ---------------------------------------------------------- */

  let pasteKeyAt = -1e9;
  let lastPaste = null;        /* { text, at } from the last paste event */
  let pasteWanted = false;
  let pasteTimer = 0;
  let copyResolve = null;      /* Safari: the pending ClipboardItem's text */

  /* Safari writes the clipboard only while handling the key: hand it a
   * promise of the text now, and fulfil it when Draftling copies. */
  function prepareCopy() {
    if (!IS_SAFARI || !window.ClipboardItem || !navigator.clipboard ||
        !navigator.clipboard.write) return;
    const p = new Promise((resolve, reject) => {
      copyResolve = resolve;
      setTimeout(() => { if (copyResolve === resolve) { copyResolve = null; reject(new Error("nothing copied")); } }, 2000);
    });
    navigator.clipboard.write([new ClipboardItem({ "text/plain": p })]).catch(() => {});
  }

  function clipSet(text) {
    if (copyResolve) {
      const r = copyResolve;
      copyResolve = null;
      r(new Blob([text], { type: "text/plain" }));
      return;
    }
    if (navigator.clipboard && navigator.clipboard.writeText) {
      navigator.clipboard.writeText(text).catch((e) =>
        console.warn("Draftling: the clipboard refused the copy:", e));
    }
  }

  function deliverPaste(text) {
    pasteWanted = false;
    clearTimeout(pasteTimer);
    if (text === null || text === undefined) {
      send(WEB_EV.PASTE, 0, 0, 0);
      return;
    }
    const bytes = new TextEncoder().encode(text);
    const p = Module.webCopyIn(bytes);
    send(WEB_EV.PASTE, p, bytes.length, 0);
  }

  function clipRequest() {
    const now = performance.now();
    if (lastPaste && now - lastPaste.at < 1500) {
      const t = lastPaste.text;
      lastPaste = null;
      deliverPaste(t);
      return;
    }
    pasteWanted = true;
    /* No paste event (Ctrl+V on a Mac, a browser that sends none):
     * ask the clipboard directly. */
    pasteTimer = setTimeout(() => {
      if (!pasteWanted) return;
      if (navigator.clipboard && navigator.clipboard.readText) {
        navigator.clipboard.readText().then((t) => deliverPaste(t), (e) => {
          console.warn("Draftling: the clipboard cannot be read:", e);
          deliverPaste(null);
        });
      } else {
        deliverPaste(null);
      }
    }, 250);
  }

  document.addEventListener("paste", (e) => {
    if (!ready || !filesEl.hidden) return;
    e.preventDefault();
    const text = e.clipboardData ? e.clipboardData.getData("text/plain") : "";
    if (pasteWanted) deliverPaste(text);
    else lastPaste = { text, at: performance.now() };
  });

  /* ---- leaving -------------------------------------------------------- */

  window.addEventListener("beforeunload", (e) => {
    if (!ready || failed || !Module._web_unsaved) return;
    send(WEB_EV.SAVE, 0, 0, 0);
    if (Module._web_unsaved()) {
      e.preventDefault();
      e.returnValue = "";
    }
  });
  document.addEventListener("visibilitychange", () => {
    if (document.visibilityState === "hidden" && ready) {
      releaseAll();
      send(WEB_EV.SAVE, 0, 0, 0);
    }
  });
  window.addEventListener("pagehide", () => { if (ready) send(WEB_EV.SAVE, 0, 0, 0); });

  /* ---- full screen and toolbar --------------------------------------- */

  const btnFull = document.getElementById("btn-fullscreen");

  async function toggleFullscreen() {
    try {
      if (document.fullscreenElement) {
        await document.exitFullscreen();
        return;
      }
      await document.documentElement.requestFullscreen({ navigationUI: "hide" });
      /* Chrome / Edge: Esc and the browser's shortcuts (Ctrl+W, Ctrl+N,
       * Ctrl+T, Ctrl+Tab) go to Draftling; holding Esc leaves. */
      if (navigator.keyboard && navigator.keyboard.lock) {
        await navigator.keyboard.lock().catch(() => {});
      }
    } catch (e) {
      console.warn("Draftling: full screen:", e);
    }
    focusScreen();
  }

  document.addEventListener("fullscreenchange", () => {
    btnFull.textContent = document.fullscreenElement ? "Leave full screen" : "Full screen";
    if (!document.fullscreenElement && navigator.keyboard && navigator.keyboard.unlock) {
      navigator.keyboard.unlock();
    }
  });
  btnFull.addEventListener("click", toggleFullscreen);
  document.getElementById("btn-files").addEventListener("click", openFiles);

  let toolbarTimer = 0;
  function showToolbar() {
    toolbar.classList.add("shown");
    clearTimeout(toolbarTimer);
    toolbarTimer = setTimeout(hideToolbar, 2500);
  }
  function hideToolbar() {
    if (toolbar.matches(":hover")) return;
    toolbar.classList.remove("shown");
  }
  window.addEventListener("mousemove", (e) => {
    if (ready && !touchMode && e.clientY < 80) showToolbar();
  });
  toolbar.addEventListener("mouseleave", () => {
    clearTimeout(toolbarTimer);
    toolbarTimer = setTimeout(hideToolbar, 1200);
  });

  /* ---- Files panel ------------------------------------------------------- */

  const filesEl = document.getElementById("files");
  const filesList = document.getElementById("files-list");
  const filesStatus = document.getElementById("files-status");
  const uploadInput = document.getElementById("upload-input");
  const pending = new Map();
  let nextId = 1;
  let listing = null;          /* the last LIST answer */
  let uploadProject = null;

  function filesReply(id, ok, text, bytes) {
    const p = pending.get(id);
    if (!p) return;
    pending.delete(id);
    if (ok) p.resolve({ text, bytes });
    else p.reject(new Error(text || "failed"));
  }

  function newString(s) {
    if (s === null || s === undefined) return 0;
    const b = new TextEncoder().encode(s + "\0");
    return Module.webCopyIn(b);
  }

  function filesRequest(op, a, b, data) {
    return new Promise((resolve, reject) => {
      if (!ready) {
        reject(new Error("Draftling is not running."));
        return;
      }
      const id = nextId++;
      pending.set(id, { resolve, reject });
      const pd = data ? Module.webCopyIn(data) : 0;
      Module._web_files_request(op, id, newString(a), newString(b), pd, data ? data.length : 0);
    });
  }

  function status(msg, error) {
    filesStatus.textContent = msg || "";
    filesStatus.classList.toggle("error", !!error);
  }

  function sizeText(n) {
    if (n < 1024) return n + " B";
    if (n < 1024 * 1024) return (n / 1024).toFixed(n < 10240 ? 1 : 0) + " KB";
    return (n / 1048576).toFixed(1) + " MB";
  }

  function saveBlob(bytes, name, type) {
    const url = URL.createObjectURL(new Blob([bytes], { type }));
    const a = document.createElement("a");
    a.href = url;
    a.download = name;
    document.body.appendChild(a);
    a.click();
    a.remove();
    setTimeout(() => URL.revokeObjectURL(url), 10000);
  }

  function stamp() {
    const d = new Date();
    const p = (n) => String(n).padStart(2, "0");
    return d.getFullYear() + "-" + p(d.getMonth() + 1) + "-" + p(d.getDate());
  }

  function mimeOf(name) {
    return /\.md$/i.test(name) ? "text/markdown" : "text/plain";
  }

  async function openFiles() {
    if (!ready) return;
    releaseAll();
    filesEl.hidden = false;
    status("Reading...");
    document.getElementById("files-close").focus();
    await refreshFiles();
  }

  function closeFiles() {
    filesEl.hidden = true;
    focusScreen();
  }

  async function refreshFiles() {
    try {
      const r = await filesRequest(FILES_OP.LIST);
      listing = JSON.parse(r.text);
      renderFiles();
      status("");
    } catch (e) {
      status(e.message, true);
    }
  }

  function el(tag, cls, text) {
    const n = document.createElement(tag);
    if (cls) n.className = cls;
    if (text !== undefined) n.textContent = text;
    return n;
  }

  function button(label, title, onClick) {
    const b = el("button", null, label);
    b.type = "button";
    if (title) b.title = title;
    b.addEventListener("click", onClick);
    return b;
  }

  function renderFiles() {
    filesList.textContent = "";
    const projects = listing.projects || [];
    document.getElementById("files-zip-all").disabled = !projects.some((p) => p.files.length);
    if (!projects.length) {
      filesList.appendChild(el("p", "hint",
        "No projects yet. Create one here or in Draftling, then upload documents into it."));
    }
    for (const p of projects) {
      const box = el("section", "project");
      const head = el("div", "project-head");
      head.appendChild(el("h2", null, p.name));
      const zip = button("Download ZIP", "Every document of " + p.name, () => downloadZip(p.name));
      zip.disabled = !p.files.length;
      head.appendChild(zip);
      head.appendChild(button("Upload...", "Upload documents into " + p.name, () => {
        uploadProject = p.name;
        uploadInput.value = "";
        uploadInput.click();
      }));
      box.appendChild(head);
      if (!p.files.length) box.appendChild(el("div", "empty", "No documents."));
      for (const f of p.files) {
        const row = el("div", "file");
        row.appendChild(el("span", "name", f.name));
        if (f.modified) row.appendChild(el("span", "badge", "open, unsaved"));
        else if (f.open) row.appendChild(el("span", "badge", "open"));
        row.appendChild(el("span", "size", sizeText(f.size)));
        row.appendChild(button("Download", null, () => downloadFile(p.name, f.name)));
        box.appendChild(row);
      }
      box.addEventListener("dragover", (e) => {
        e.preventDefault();
        box.classList.add("drop");
      });
      box.addEventListener("dragleave", () => box.classList.remove("drop"));
      box.addEventListener("drop", (e) => {
        e.preventDefault();
        box.classList.remove("drop");
        upload(p.name, e.dataTransfer.files);
      });
      filesList.appendChild(box);
    }
  }

  async function downloadFile(project, name) {
    status("Downloading " + name + "...");
    try {
      const r = await filesRequest(FILES_OP.READ, project, name);
      saveBlob(r.bytes, name, mimeOf(name));
      status("");
    } catch (e) {
      status(e.message, true);
    }
  }

  async function downloadZip(project) {
    status("Packing " + (project || "every project") + "...");
    try {
      const r = await filesRequest(FILES_OP.ZIP, project || "");
      saveBlob(r.bytes, (project || "Draftling") + "-" + stamp() + ".zip", "application/zip");
      status("");
    } catch (e) {
      status(e.message, true);
    }
  }

  async function upload(project, fileList) {
    const files = Array.from(fileList || []);
    if (!files.length) return;
    const proj = (listing.projects || []).find((p) => p.name === project);
    const existing = new Set(proj ? proj.files.map((f) => f.name) : []);
    const max = listing.maxSize || Infinity;
    let done = 0;
    const errors = [];
    for (const f of files) {
      if (!/\.(md|fountain|txt)$/i.test(f.name)) {
        errors.push(f.name + ": only .md, .fountain and .txt files");
        continue;
      }
      if (f.size > max) {
        errors.push(f.name + ": larger than Draftling can open");
        continue;
      }
      if (existing.has(f.name) &&
          !confirm(project + " has a document " + f.name + " already. Replace it?")) continue;
      status("Uploading " + f.name + "...");
      try {
        const data = new Uint8Array(await f.arrayBuffer());
        await filesRequest(FILES_OP.WRITE, project, f.name, data);
        done++;
      } catch (e) {
        errors.push(f.name + ": " + e.message);
      }
    }
    await refreshFiles();
    if (errors.length) status(errors.join(" "), true);
    else status(done ? (done === 1 ? "1 document uploaded." : done + " documents uploaded.") : "");
  }

  uploadInput.addEventListener("change", () => {
    if (uploadProject) upload(uploadProject, uploadInput.files);
  });

  document.getElementById("files-zip-all").addEventListener("click", () => downloadZip(""));
  document.getElementById("files-new-project").addEventListener("click", async () => {
    const name = prompt("Name of the new project (letters, digits, space, - and _):", "");
    if (!name) return;
    try {
      await filesRequest(FILES_OP.MKPROJECT, name.trim());
      await refreshFiles();
      status("Project " + name.trim() + " created.");
    } catch (e) {
      status(e.message, true);
    }
  });
  document.getElementById("files-close").addEventListener("click", closeFiles);
  filesEl.addEventListener("keydown", (e) => {
    if (e.key === "Escape" || e.code === "F9") {
      e.preventDefault();
      e.stopPropagation();
      closeFiles();
    }
  });
  filesEl.addEventListener("mousedown", (e) => { if (e.target === filesEl) closeFiles(); });
  /* Files dropped anywhere else do not open in the tab. */
  window.addEventListener("dragover", (e) => e.preventDefault());
  window.addEventListener("drop", (e) => e.preventDefault());

  start().catch((e) => {
    console.error(e);
    fail("Draftling could not start", "<p>" + escapeHtml(String(e && e.message || e)) + "</p>");
  });
})();
