#pragma once

namespace MecchaCheatV::WebPanel
{
	inline constexpr const char* kWebViewHtml = R"HTML(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>xv_meca · ESP</title>
<style>
* { box-sizing: border-box; margin: 0; padding: 0; }
html, body { width: 100%; height: 100%; overflow: hidden; background: #000; }
canvas { display: block; width: 100vw; height: 100vh; }
#hint {
  position: fixed; left: 50%; top: 50%; transform: translate(-50%, -50%);
  color: #555; font: 14px "Segoe UI", system-ui, sans-serif; text-align: center;
  pointer-events: none; line-height: 1.6;
}
</style>
</head>
<body>
<canvas id="c"></canvas>
<div id="hint">Enable <b>Web Only</b> in the control panel<br>http://127.0.0.1:17777</div>
<script>
const canvas = document.getElementById('c');
const ctx = canvas.getContext('2d');
const hint = document.getElementById('hint');

function imColor(c) {
  const r = c & 255;
  const g = (c >> 8) & 255;
  const b = (c >> 16) & 255;
  const a = ((c >> 24) & 255) / 255;
  return 'rgba(' + r + ',' + g + ',' + b + ',' + a + ')';
}

function resize() {
  const dpr = window.devicePixelRatio || 1;
  canvas.width = Math.floor(window.innerWidth * dpr);
  canvas.height = Math.floor(window.innerHeight * dpr);
  ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
}
window.addEventListener('resize', resize);
resize();

function drawFrame(frame) {
  const vw = window.innerWidth;
  const vh = window.innerHeight;
  ctx.fillStyle = '#000';
  ctx.fillRect(0, 0, vw, vh);

  if (!frame.webOnly) {
    hint.innerHTML = 'Enable <b>Web Only</b> in the control panel<br>http://127.0.0.1:17777';
    hint.style.display = 'block';
    return;
  }

  if (!frame.active || !frame.w || !frame.h) {
    hint.innerHTML = '<b>Web Only</b> is on — waiting for game render...';
    hint.style.display = 'block';
    return;
  }

  hint.style.display = 'none';

  const sx = vw / frame.w;
  const sy = vh / frame.h;

  for (const line of frame.lines || []) {
    ctx.strokeStyle = imColor(line.c);
    ctx.lineWidth = Math.max(1, line.t * ((sx + sy) * 0.5));
    ctx.beginPath();
    ctx.moveTo(line.x1 * sx, line.y1 * sy);
    ctx.lineTo(line.x2 * sx, line.y2 * sy);
    ctx.stroke();
  }

  for (const rect of frame.rects || []) {
    ctx.strokeStyle = imColor(rect.c);
    ctx.lineWidth = Math.max(1, rect.t * ((sx + sy) * 0.5));
    const w = (rect.x2 - rect.x1) * sx;
    const h = (rect.y2 - rect.y1) * sy;
    const r = (rect.r || 0) * ((sx + sy) * 0.5);
    const x = rect.x1 * sx;
    const y = rect.y1 * sy;
    if (r > 0) {
      ctx.beginPath();
      ctx.roundRect(x, y, w, h, r);
      ctx.stroke();
    } else {
      ctx.strokeRect(x, y, w, h);
    }
  }

  ctx.textBaseline = 'top';
  for (const text of frame.texts || []) {
    ctx.fillStyle = imColor(text.c);
    const size = Math.max(8, text.s * sy);
    ctx.font = size + 'px "Segoe UI", system-ui, sans-serif';
    ctx.fillText(text.v, text.x * sx, text.y * sy);
  }
}

let busy = false;
async function tick() {
  if (!busy) {
    busy = true;
    try {
      const res = await fetch('/api/web/frame');
      if (res.ok) drawFrame(await res.json());
    } catch (_) {}
    busy = false;
  }
  requestAnimationFrame(tick);
}
tick();
</script>
</body>
</html>)HTML";
}
