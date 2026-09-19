/**
 * DIPI application photo: preview + auto face-crop (35x45) + mobile camera.
 *
 * Ported from the schedule form's photo-preview.js, but much smaller: DIPI's
 * upload_photo is a plain Drupal 7 #type=file that uploads on normal form
 * submit (NOT an Ajax managed-file), so all the schedule code that holds back
 * the instant Ajax upload / presses a hidden upload button / swaps the Ajax
 * wrapper is unnecessary here.
 *
 * Flow: choose (or snap) a photo -> editor opens with Cropper locked to the
 * 35x45mm passport ratio, pico.js auto-frames the face -> operator adjusts ->
 * "Use this photo" writes the cropped JPEG back onto the file input via
 * DataTransfer -> the normal form submit POSTs the cropped file. The server
 * handler (file_save_upload('upload_photo') -> scale 800 -> S3) is unchanged.
 *
 * Everything is best-effort: if a library fails to load or the browser can't
 * edit, the plain file input is left exactly as it is today and the original
 * uploads. The photo must never be lost to a fancy editor.
 */
(function ($, Drupal) {
  'use strict';

  var RATIO_W = 35, RATIO_H = 45;
  var RATIO = RATIO_W / RATIO_H;
  // The server resizes to 800 anyway; a 600px-tall 35:45 crop is 467px wide.
  var MAX_W = 800, MAX_H = 600;

  function picoBase() {
    var s = (Drupal.settings && Drupal.settings.dhAppPhoto) || {};
    return s.picoPath || '/sites/all/modules/dh_manageapp/libraries/picojs/';
  }

  /* ---- pico.js face detection (loaded on demand, ~240KB) ---- */
  var picoPromise = null;
  function loadPico() {
    if (picoPromise) { return picoPromise; }
    var base = picoBase();
    picoPromise = new Promise(function (resolve, reject) {
      var script = document.createElement('script');
      script.src = base + 'pico.js';
      script.onerror = function () { reject(new Error('pico.js failed')); };
      script.onload = function () {
        if (!window.pico || typeof window.pico.unpack_cascade !== 'function') {
          reject(new Error('pico.js unusable')); return;
        }
        window.fetch(base + 'facefinder')
          .then(function (r) { if (!r.ok) { throw new Error('cascade ' + r.status); } return r.arrayBuffer(); })
          .then(function (buf) { resolve(window.pico.unpack_cascade(new Int8Array(buf))); })
          .catch(reject);
      };
      document.head.appendChild(script);
    });
    picoPromise.catch(function () { picoPromise = null; });
    return picoPromise;
  }

  // Strongest face in natural image coords, or null. Runs on a downscaled copy.
  function findFace(img, classify) {
    var DETECT_MAX_SIDE = 640, MIN_QUALITY = 50.0;
    try {
      var natW = img.naturalWidth, natH = img.naturalHeight;
      if (!natW || !natH) { return null; }
      var scale = Math.min(1, DETECT_MAX_SIDE / Math.max(natW, natH));
      var w = Math.max(1, Math.round(natW * scale)), h = Math.max(1, Math.round(natH * scale));
      var canvas = document.createElement('canvas');
      canvas.width = w; canvas.height = h;
      var ctx = canvas.getContext('2d'); if (!ctx) { return null; }
      ctx.drawImage(img, 0, 0, w, h);
      var rgba = ctx.getImageData(0, 0, w, h).data;
      var gray = new Uint8Array(w * h);
      for (var i = 0; i < w * h; i++) {
        gray[i] = (2 * rgba[4 * i] + 7 * rgba[4 * i + 1] + rgba[4 * i + 2]) / 10;
      }
      var found = window.pico.cluster_detections(
        window.pico.run_cascade(
          { pixels: gray, nrows: h, ncols: w, ldim: w }, classify,
          { shiftfactor: 0.1, minsize: Math.max(20, Math.round(Math.min(w, h) * 0.1)), maxsize: Math.min(w, h), scalefactor: 1.1 }
        ), 0.2);
      var best = null;
      for (var j = 0; j < found.length; j++) {
        if (found[j][3] < MIN_QUALITY) { continue; }
        if (!best || found[j][3] > best[3]) { best = found[j]; }
      }
      if (!best) { return null; }
      return { x: (best[1] - best[2] / 2) / scale, y: (best[0] - best[2] / 2) / scale, width: best[2] / scale, height: best[2] / scale };
    } catch (e) { return null; }
  }

  /* ---- capability guards ---- */
  function canEdit() {
    if (typeof window.Cropper === 'undefined') { return false; }
    if (!document.createElement('canvas').getContext) { return false; }
    try {
      var dt = new DataTransfer();
      dt.items.add(new File(['x'], 'x.jpg', { type: 'image/jpeg' }));
      return dt.files.length === 1;
    } catch (e) { return false; }
  }
  function cameraWorthOffering() {
    if (!window.DataTransfer || !window.matchMedia) { return false; }
    try { return new DataTransfer().items !== undefined && window.matchMedia('(pointer: coarse)').matches; }
    catch (e) { return false; }
  }

  /* ---- state (one photo field per form) ---- */
  var pending = null;    // File chosen but not yet confirmed
  var confirmer = null;  // function that crops + sets the current editor's file

  /* ---- crop-box geometry (from the schedule widget) ---- */
  function clampBox(b, imgW, imgH) {
    var r = { x: b.x, y: b.y, width: Math.min(b.width, imgW), height: Math.min(b.height, imgH) };
    if (r.width / r.height > RATIO) { r.width = r.height * RATIO; } else { r.height = r.width / RATIO; }
    r.x = Math.max(0, Math.min(r.x, imgW - r.width));
    r.y = Math.max(0, Math.min(r.y, imgH - r.height));
    return r;
  }
  function upperCentreBox(imgW, imgH) {
    var height = imgH * 0.9, width = height * RATIO;
    if (width > imgW) { width = imgW; height = width / RATIO; }
    return clampBox({ x: (imgW - width) / 2, y: imgH * 0.05, width: width, height: height }, imgW, imgH);
  }
  function boxAroundFace(face, imgW, imgH) {
    var FRAME_OF_FACE = 1.7;
    var height = face.height * FRAME_OF_FACE, width = height * RATIO;
    return clampBox({
      x: (face.x + face.width / 2) - width / 2,
      y: (face.y + face.height / 2) - height * 0.45,
      width: width, height: height
    }, imgW, imgH);
  }

  function icon(paths) {
    return '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.8" ' +
      'stroke-linecap="round" stroke-linejoin="round" aria-hidden="true">' + paths + '</svg>';
  }
  var ICONS = {
    left: '<path d="M9 4 4 9l5 5"/><path d="M4 9h8a6 6 0 1 1 0 12h-3"/>',
    right: '<path d="M15 4l5 5-5 5"/><path d="M20 9h-8a6 6 0 1 0 0 12h3"/>',
    zoomIn: '<circle cx="11" cy="11" r="7"/><path d="M20.5 20.5 16 16"/><path d="M11 8.2v5.6M8.2 11h5.6"/>',
    zoomOut: '<circle cx="11" cy="11" r="7"/><path d="M20.5 20.5 16 16"/><path d="M8.2 11h5.6"/>',
    reset: '<path d="M4 5v5h5"/><path d="M4.5 10a8 8 0 1 1 .9 6.4"/>'
  };
  function iconBtn(name, label) {
    var b = document.createElement('button');
    b.type = 'button'; b.className = 'dh-photo-icon'; b.title = label; b.setAttribute('aria-label', label);
    b.innerHTML = icon(ICONS[name]);
    return b;
  }
  function txtBtn(label, cls) {
    var b = document.createElement('button');
    b.type = 'button'; b.className = 'dh-photo-btn' + (cls ? ' ' + cls : ''); b.textContent = label;
    return b;
  }

  /* ---- the editor ---- */
  function buildEditor(input) {
    // Remove any previous editor on this form.
    var form = input.form || input.closest('form');
    (form || document).querySelectorAll('.dh-photo-editor').forEach(function (o) {
      if (o.parentNode) { o.parentNode.removeChild(o); }
    });
    var box = document.createElement('div');
    box.className = 'dh-photo-editor';
    var stage = document.createElement('div');
    stage.className = 'dh-photo-stage';
    var img = document.createElement('img');
    img.alt = Drupal.t('The photo you chose');
    stage.appendChild(img);
    var bar = document.createElement('div');
    bar.className = 'dh-photo-actions';
    box.appendChild(stage); box.appendChild(bar);
    // Place right after the file field's form-item so it is where the operator looks.
    var host = input.closest('.form-item') || input.parentNode;
    host.parentNode.insertBefore(box, host.nextSibling);
    return { box: box, img: img, bar: bar };
  }

  function setFileOnInput(input, blob, name) {
    try {
      var dt = new DataTransfer();
      dt.items.add(new File([blob], name, { type: 'image/jpeg', lastModified: Date.now() }));
      input.files = dt.files;
    } catch (e) { /* leave whatever is on the input */ }
  }

  function openEditor(input, file) {
    if (!/^image\//.test(file.type)) { return; }
    pending = file;
    var ui = buildEditor(input);
    var cropper = null;
    var url = URL.createObjectURL(file);
    var touched = false;

    function frameOn(target) {
      if (!cropper || !target.width || !target.height) { return; }
      var container = cropper.getContainerData(), canvas = cropper.getCanvasData();
      if (!container.width || !container.height) { return; }
      var MARGIN = 0.88, boxH = container.height * MARGIN, boxW = boxH * RATIO;
      if (boxW > container.width * MARGIN) { boxW = container.width * MARGIN; boxH = boxW / RATIO; }
      var boxLeft = (container.width - boxW) / 2, boxTop = (container.height - boxH) / 2;
      var scale = boxW / target.width;
      cropper.setCanvasData({ left: boxLeft - target.x * scale, top: boxTop - target.y * scale, width: canvas.naturalWidth * scale, height: canvas.naturalHeight * scale });
      cropper.setCropBoxData({ left: boxLeft, top: boxTop, width: boxW, height: boxH });
    }

    function autoFrame() {
      var imgW = ui.img.naturalWidth, imgH = ui.img.naturalHeight;
      if (!imgW || !imgH || !cropper) { return; }
      try { cropper.setData(upperCentreBox(imgW, imgH)); } catch (e) { return; }
      var lapsed = false;
      var budget = window.setTimeout(function () { lapsed = true; }, 4000);
      loadPico().then(function (classify) {
        window.clearTimeout(budget);
        if (lapsed || touched || !cropper) { return; }
        var face = findFace(ui.img, classify);
        if (!face) { return; }
        frameOn(boxAroundFace(face, imgW, imgH));
      }).catch(function () { window.clearTimeout(budget); });
    }

    function showConfirmed(blob) {
      var shown = URL.createObjectURL(blob);
      ui.box.textContent = '';
      ui.box.className = 'dh-photo-editor dh-photo-editor--done';
      var stage = document.createElement('div'); stage.className = 'dh-photo-stage';
      var done = document.createElement('img'); done.alt = Drupal.t('The photo to be saved'); done.src = shown;
      done.addEventListener('error', function () { URL.revokeObjectURL(shown); });
      stage.appendChild(done);
      var note = document.createElement('p'); note.className = 'dh-photo-hint dh-photo-hint--done';
      note.textContent = Drupal.t('This photo will be saved. To change it, choose or take another.');
      ui.box.appendChild(stage); ui.box.appendChild(note);
    }

    function send(blob, name, after) {
      setFileOnInput(input, blob, name);
      pending = null; confirmer = null;
      if (cropper) { cropper.destroy(); cropper = null; }
      URL.revokeObjectURL(url);
      showConfirmed(blob);
      if (typeof after === 'function') { after(); }
    }

    function confirm(after) {
      var name = (file.name || 'photo.jpg').replace(/\.[^.]+$/, '') + '.jpg';
      if (!cropper) { send(file, file.name || name, after); return; }
      var crop = cropper.getData(true);
      var options = { imageSmoothingQuality: 'high' };
      var shrink = Math.min(MAX_W / crop.width, MAX_H / crop.height);
      if (shrink < 1) { options.width = Math.round(crop.width * shrink); options.height = Math.round(crop.height * shrink); }
      var canvas = cropper.getCroppedCanvas(options);
      if (!canvas) { send(file, file.name || name, after); return; }
      canvas.toBlob(function (blob) {
        send(blob || file, blob ? name.replace(/\.jpg$/, '-crop.jpg') : (file.name || name), after);
      }, 'image/jpeg', 0.9);
    }

    function giveUpAndSend() { if (pending) { send(file, file.name || 'photo.jpg'); } }

    ui.img.onerror = giveUpAndSend;
    var safety = window.setTimeout(giveUpAndSend, 8000);
    ui.img.src = url;
    ui.img.onload = function () {
      window.clearTimeout(safety);
      try {
        ui.img.addEventListener('cropstart', function () { touched = true; });
        cropper = new window.Cropper(ui.img, {
          viewMode: 1, autoCropArea: 0.9, responsive: true, background: false,
          aspectRatio: RATIO_W / RATIO_H, zoomable: true, zoomOnWheel: false,
          dragMode: 'move', cropBoxMovable: false, cropBoxResizable: false,
          ready: function () { autoFrame(); }
        });

        var quarters = 0, tilt = 0;
        function applyAngle() { if (cropper) { cropper.rotateTo(quarters * 90 + tilt); } }
        var left = iconBtn('left', Drupal.t('Rotate left'));
        left.addEventListener('click', function () { quarters -= 1; applyAngle(); });
        var right = iconBtn('right', Drupal.t('Rotate right'));
        right.addEventListener('click', function () { quarters += 1; applyAngle(); });
        var zin = iconBtn('zoomIn', Drupal.t('Zoom in'));
        zin.addEventListener('click', function () { if (cropper) { cropper.zoom(0.1); } });
        var zout = iconBtn('zoomOut', Drupal.t('Zoom out'));
        zout.addEventListener('click', function () { if (cropper) { cropper.zoom(-0.1); } });
        var reset = iconBtn('reset', Drupal.t('Start again'));

        var tools = document.createElement('div'); tools.className = 'dh-photo-tools';
        [left, right, zin, zout, reset].forEach(function (b) { tools.appendChild(b); });
        ui.bar.appendChild(tools);

        var fine = document.createElement('div'); fine.className = 'dh-photo-fine';
        var flabel = document.createElement('label'); flabel.className = 'dh-photo-fine-label'; flabel.textContent = Drupal.t('Straighten');
        var slider = document.createElement('input'); slider.type = 'range'; slider.min = '-15'; slider.max = '15'; slider.step = '1'; slider.value = '0';
        var readout = document.createElement('span'); readout.className = 'dh-photo-degrees'; readout.textContent = '0°';
        slider.addEventListener('input', function () { tilt = parseInt(slider.value, 10) || 0; readout.textContent = tilt + '°'; applyAngle(); });
        fine.appendChild(flabel); fine.appendChild(slider); fine.appendChild(readout);
        ui.bar.appendChild(fine);

        reset.addEventListener('click', function () {
          quarters = 0; tilt = 0;
          if (cropper) { cropper.reset(); touched = false; autoFrame(); }
          slider.value = 0; readout.textContent = '0°';
        });

        var use = txtBtn(Drupal.t('Use this photo'), 'dh-photo-btn--primary');
        use.addEventListener('click', function () { confirm(); });
        ui.bar.appendChild(use);

        confirmer = confirm;

        var hint = document.createElement('p'); hint.className = 'dh-photo-hint';
        hint.textContent = Drupal.t('Move and zoom so the face fills the frame, then press "Use this photo".');
        ui.bar.appendChild(hint);
      } catch (e) { giveUpAndSend(); }
    };
  }

  function isCoarsePointer() {
    try { return window.matchMedia && window.matchMedia('(pointer: coarse)').matches; } catch (e) { return false; }
  }
  // Desktop/laptop/Mac live webcam. getUserMedia needs a secure context, so it is
  // present on https and on localhost but not on plain http to a hostname; when
  // it is absent we simply do not offer the button.
  function hasUserMedia() {
    return !!(navigator.mediaDevices && navigator.mediaDevices.getUserMedia);
  }

  /* ---- desktop webcam: live preview -> snapshot -> the same editor ---- */
  function openWebcam(input) {
    var form = input.form || input.closest('form');
    (form || document).querySelectorAll('.dh-photo-editor, .dh-webcam').forEach(function (o) {
      if (o.parentNode) { o.parentNode.removeChild(o); }
    });
    var box = document.createElement('div'); box.className = 'dh-photo-editor dh-webcam';
    var stage = document.createElement('div'); stage.className = 'dh-photo-stage';
    var video = document.createElement('video');
    video.autoplay = true; video.playsInline = true; video.muted = true;
    stage.appendChild(video);
    var bar = document.createElement('div'); bar.className = 'dh-photo-actions';
    var capture = txtBtn(Drupal.t('Capture'), 'dh-photo-btn--primary'); capture.disabled = true;
    var cancel = txtBtn(Drupal.t('Cancel'));
    var hint = document.createElement('p'); hint.className = 'dh-photo-hint';
    hint.textContent = Drupal.t('Starting the camera…');
    bar.appendChild(capture); bar.appendChild(cancel);
    box.appendChild(stage); box.appendChild(bar); box.appendChild(hint);
    var host = input.closest('.form-item') || input.parentNode;
    host.parentNode.insertBefore(box, host.nextSibling);

    var stream = null;
    function stop() { if (stream) { stream.getTracks().forEach(function (t) { t.stop(); }); stream = null; } }
    function close() { stop(); if (box.parentNode) { box.parentNode.removeChild(box); } }
    cancel.addEventListener('click', close);

    navigator.mediaDevices.getUserMedia({ video: { facingMode: 'user', width: { ideal: 1280 }, height: { ideal: 960 } }, audio: false })
      .then(function (s) { stream = s; video.srcObject = s; capture.disabled = false; hint.textContent = Drupal.t('Look at the camera, then press Capture.'); })
      .catch(function () { hint.textContent = Drupal.t('Could not open the camera. Press Cancel and choose a file instead.'); });

    capture.addEventListener('click', function () {
      if (!stream) { return; }
      var w = video.videoWidth || 1280, h = video.videoHeight || 960;
      var c = document.createElement('canvas'); c.width = w; c.height = h;
      var ctx = c.getContext('2d'); if (!ctx) { return; }
      ctx.drawImage(video, 0, 0, w, h);
      c.toBlob(function (blob) {
        if (!blob) { return; }
        var f = new File([blob], 'camera.jpg', { type: 'image/jpeg', lastModified: Date.now() });
        var dt = new DataTransfer(); dt.items.add(f); input.files = dt.files;
        close();
        input.dispatchEvent(new Event('change', { bubbles: true }));  // -> openEditor
      }, 'image/jpeg', 0.92);
    });
  }

  /* ---- "Take a photo" / "Choose a file" ---- */
  function addCameraButton(input) {
    var host = input.parentNode;
    if (!host || host.querySelector('.dh-photo-choose')) { return; }
    var coarse = isCoarsePointer();
    var nativeCam = null;
    if (coarse) {
      // Mobile: the native camera app, via a nameless capture input.
      nativeCam = document.createElement('input');
      nativeCam.type = 'file'; nativeCam.accept = 'image/*'; nativeCam.setAttribute('capture', 'environment');
      nativeCam.className = 'dh-photo-camera-input'; nativeCam.removeAttribute('name');
      nativeCam.addEventListener('change', function (e) {
        e.stopImmediatePropagation();
        var f = nativeCam.files && nativeCam.files[0]; if (!f) { return; }
        var dt = new DataTransfer(); dt.items.add(f); input.files = dt.files;
        nativeCam.value = '';
        input.dispatchEvent(new Event('change', { bubbles: true }));
      });
      host.appendChild(nativeCam);
    }
    var row = document.createElement('div'); row.className = 'dh-photo-choose';
    var shoot = txtBtn(Drupal.t('Take a photo'), 'dh-photo-camera');
    shoot.addEventListener('click', function () {
      if (coarse && nativeCam) { nativeCam.click(); }   // mobile: native camera
      else { openWebcam(input); }                        // desktop: live webcam
    });
    var pick = txtBtn(Drupal.t('Choose a file'), 'dh-photo-pick');
    pick.addEventListener('click', function () { input.click(); });
    var or = document.createElement('span'); or.className = 'dh-photo-or'; or.textContent = Drupal.t('or');
    row.appendChild(shoot); row.appendChild(or); row.appendChild(pick);
    host.insertBefore(row, input);
    input.classList.add('dh-photo-native-hidden');
  }

  Drupal.behaviors.dhAppPhoto = {
    attach: function (context) {
      var editable = canEdit();
      $(context).find('input[type="file"][id="edit-upload-photo"]').once('dh-app-photo').each(function () {
        var input = this;
        input.setAttribute('accept', 'image/*');   // lets mobile offer the camera on the native control too
        // Offer "Take a photo" on mobile (native camera) and on desktop/laptop/Mac
        // (live webcam via getUserMedia). Absent only where neither is available.
        if (cameraWorthOffering() || hasUserMedia()) { addCameraButton(input); }

        input.addEventListener('change', function () {
          var file = input.files && input.files[0];
          if (!file || !editable || !/^image\//.test(file.type)) { return; }
          openEditor(input, file);
        });

        // If a photo was chosen but "Use this photo" was never pressed, crop it on
        // submit (so a framed 35x45 is saved) then let the form submit for real.
        var form = input.form || input.closest('form');
        if (form) {
          form.addEventListener('submit', function (e) {
            if (!pending || !confirmer) { return; }
            e.preventDefault();
            confirmer(function () { form.submit(); });
          }, true);
        }
      });
    }
  };

})(jQuery, Drupal);
