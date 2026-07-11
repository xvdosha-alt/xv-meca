#pragma once

namespace MecchaCheatV::WebPanel
{
	inline constexpr const char* kIndexHtml = R"HTML(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>xv_meca</title>
<style>
:root {
  --bg: #0b0b10;
  --card: #14141c;
  --border: #2a2040;
  --text: #e8e8f0;
  --muted: #8b8ba3;
  --accent: #a855f7;
  --accent-dim: #7c3aed;
  --on: #22c55e;
  --off: #3f3f50;
}
* { box-sizing: border-box; }
body {
  margin: 0;
  font-family: "Segoe UI", system-ui, sans-serif;
  background: radial-gradient(1200px 600px at 80% -10%, #1a1030 0%, var(--bg) 55%);
  color: var(--text);
  min-height: 100vh;
}
.wrap { max-width: 720px; margin: 0 auto; padding: 28px 20px 48px; }
header { margin-bottom: 28px; }
h1 { margin: 0; font-size: 2rem; letter-spacing: 0.04em; }
h1 span { color: var(--accent); }
.sub { color: var(--muted); margin-top: 6px; font-size: 0.95rem; }
.status { margin-top: 14px; font-size: 0.85rem; color: var(--muted); }
.status.ok { color: #86efac; }
.grid { display: grid; gap: 14px; }
.card {
  background: var(--card);
  border: 1px solid var(--border);
  border-radius: 14px;
  padding: 16px 18px;
  box-shadow: 0 8px 24px rgba(0,0,0,.25);
}
.card-head { display: flex; align-items: center; justify-content: space-between; gap: 12px; }
.title { font-weight: 600; font-size: 1.05rem; }
.cat { font-size: 0.75rem; color: var(--accent); text-transform: uppercase; letter-spacing: 0.08em; }
.switch { position: relative; width: 48px; height: 26px; flex-shrink: 0; }
.switch input { opacity: 0; width: 0; height: 0; }
.slider {
  position: absolute; inset: 0; cursor: pointer;
  background: var(--off); border-radius: 999px; transition: .2s;
}
.slider:before {
  content: ""; position: absolute; height: 20px; width: 20px; left: 3px; bottom: 3px;
  background: white; border-radius: 50%; transition: .2s;
}
input:checked + .slider { background: var(--accent-dim); }
input:checked + .slider:before { transform: translateX(22px); }
.opts { margin-top: 12px; padding-top: 12px; border-top: 1px solid var(--border); display: grid; gap: 8px; }
.opt { display: flex; align-items: center; justify-content: space-between; font-size: 0.9rem; color: var(--muted); }
.opt label { cursor: pointer; }
.empty { text-align: center; color: var(--muted); padding: 40px 0; }
.danger-card { border-color: #5c2020; margin-bottom: 18px; }
.danger-text { margin: 8px 0 14px; color: var(--muted); font-size: 0.9rem; }
.btn-danger {
  width: 100%; padding: 12px 16px; border: 1px solid #991b1b; border-radius: 10px;
  background: linear-gradient(180deg, #b91c1c, #7f1d1d); color: #fff;
  font-size: 0.95rem; font-weight: 600; cursor: pointer;
}
.btn-danger:hover { filter: brightness(1.08); }
.field-row { display: grid; grid-template-columns: repeat(3, 1fr); gap: 8px; margin-bottom: 10px; }
.field, .select-field {
  width: 100%; padding: 8px 10px; border-radius: 8px; border: 1px solid var(--border);
  background: #0f0f16; color: var(--text); font-size: 0.9rem;
}
.btn {
  width: 100%; padding: 10px 14px; border-radius: 10px; border: 1px solid var(--border);
  background: #1a1430; color: var(--text); font-weight: 600; cursor: pointer; margin-top: 6px;
}
.btn:hover { border-color: var(--accent); }
.section-title { font-size: 0.8rem; color: var(--accent); margin: 12px 0 8px; text-transform: uppercase; letter-spacing: 0.06em; }
.panel { display: grid; gap: 8px; }
.panel.disabled { opacity: 0.45; pointer-events: none; }
.hint { font-size: 0.82rem; color: var(--muted); margin-top: 4px; }
</style>
</head>
<body>
<div class="wrap">
  <header>
    <h1><span>xv</span>_meca</h1>
    <div class="sub">Control panel · ESP overlay: <a href="/web" style="color:#a855f7">/web</a></div>
    <div class="status" id="status">Connecting...</div>
  </header>
  <div class="card danger-card">
    <div class="title">Emergency Exit</div>
    <div class="danger-text">Completely unload the cheat from the game.</div>
    <button type="button" class="btn-danger" id="unloadBtn">Unload xv_meca</button>
  </div>
  <div class="grid" id="features"></div>
</div>
<script>
const $features = document.getElementById('features');
const $status = document.getElementById('status');
const $unloadBtn = document.getElementById('unloadBtn');

$unloadBtn.addEventListener('click', async () => {
  if (!confirm('Unload xv_meca from the game?')) return;
  try {
    await api('/api/unload', {});
    $status.textContent = 'Unloading cheat...';
    $status.className = 'status';
    $unloadBtn.disabled = true;
  } catch (e) {
    $status.textContent = 'Unload failed';
    $status.className = 'status';
  }
});

async function api(path, body) {
  const res = await fetch(path, {
    method: body ? 'POST' : 'GET',
    headers: body ? {'Content-Type':'application/json'} : {},
    body: body ? JSON.stringify(body) : undefined
  });
  if (!res.ok) throw new Error('HTTP ' + res.status);
  return res.json();
}

function getOpt(f, key, fallback) {
  const o = (f.options || []).find(x => x.key === key);
  return o ? o.value : fallback;
}

function renderFloatOpt(f, o, container) {
  const row = document.createElement('div');
  row.className = 'opt';
  const lbl = document.createElement('label');
  lbl.textContent = o.key;
  const inp = document.createElement('input');
  inp.type = 'number';
  inp.step = '0.1';
  inp.className = 'field';
  inp.style.width = '120px';
  inp.value = o.value;
  let timer;
  inp.addEventListener('input', () => {
    clearTimeout(timer);
    timer = setTimeout(async () => {
      try {
        await api('/api/option', { id: f.id, key: o.key, value: parseFloat(inp.value) });
        $status.textContent = 'Saved';
        $status.className = 'status ok';
      } catch (e) {
        $status.textContent = 'Failed to save';
        $status.className = 'status';
      }
    }, 400);
  });
  row.append(lbl, inp);
  container.append(row);
}

function renderTeleportPanel(card, f) {
  const panel = document.createElement('div');
  panel.className = 'opts panel' + (f.enabled ? '' : ' disabled');

  const coordsTitle = document.createElement('div');
  coordsTitle.className = 'section-title';
  coordsTitle.textContent = 'Teleport to coords';
  panel.append(coordsTitle);

  const row = document.createElement('div');
  row.className = 'field-row';
  const x = document.createElement('input');
  x.type = 'number'; x.step = 'any'; x.className = 'field'; x.placeholder = 'X';
  x.value = getOpt(f, 'TeleportLocationX', 0);
  const y = document.createElement('input');
  y.type = 'number'; y.step = 'any'; y.className = 'field'; y.placeholder = 'Y';
  y.value = getOpt(f, 'TeleportLocationY', 0);
  const z = document.createElement('input');
  z.type = 'number'; z.step = 'any'; z.className = 'field'; z.placeholder = 'Z';
  z.value = getOpt(f, 'TeleportLocationZ', 0);
  row.append(x, y, z);
  panel.append(row);

  const tpCoords = document.createElement('button');
  tpCoords.type = 'button';
  tpCoords.className = 'btn';
  tpCoords.textContent = 'Teleport to coords';
  tpCoords.addEventListener('click', async () => {
    try {
      await api('/api/teleport/coords', { x: parseFloat(x.value), y: parseFloat(y.value), z: parseFloat(z.value) });
      $status.textContent = 'Teleport requested';
      $status.className = 'status ok';
    } catch (e) {
      $status.textContent = 'Teleport failed (enable feature?)';
      $status.className = 'status';
    }
  });
  panel.append(tpCoords);

  const playerTitle = document.createElement('div');
  playerTitle.className = 'section-title';
  playerTitle.textContent = 'Teleport to player';
  panel.append(playerTitle);

  const select = document.createElement('select');
  select.className = 'select-field';
  select.innerHTML = '<option value="">No players loaded</option>';

  const refreshBtn = document.createElement('button');
  refreshBtn.type = 'button';
  refreshBtn.className = 'btn';
  refreshBtn.textContent = 'Refresh players';
  refreshBtn.addEventListener('click', async () => {
    try {
      const data = await api('/api/teleport/players');
      select.innerHTML = '';
      if (!data.players || !data.players.length) {
        select.innerHTML = '<option value="">No players found</option>';
      } else {
        for (const p of data.players) {
          const opt = document.createElement('option');
          opt.value = String(p.index);
          opt.textContent = p.name + ' [' + p.index + ']';
          select.append(opt);
        }
      }
      $status.textContent = 'Players refreshed';
      $status.className = 'status ok';
    } catch (e) {
      $status.textContent = 'Failed to load players';
      $status.className = 'status';
    }
  });

  const tpPlayer = document.createElement('button');
  tpPlayer.type = 'button';
  tpPlayer.className = 'btn';
  tpPlayer.textContent = 'Teleport to player';
  tpPlayer.addEventListener('click', async () => {
    if (select.value === '') return;
    try {
      await api('/api/teleport/player', { index: parseInt(select.value, 10) });
      $status.textContent = 'Teleport to player requested';
      $status.className = 'status ok';
    } catch (e) {
      $status.textContent = 'Teleport failed (enable feature?)';
      $status.className = 'status';
    }
  });

  panel.append(refreshBtn, select, tpPlayer);
  card.append(panel);
}

function renderSetNamePanel(card, f) {
  const panel = document.createElement('div');
  panel.className = 'opts panel' + (f.enabled ? '' : ' disabled');

  const input = document.createElement('input');
  input.type = 'text';
  input.className = 'field';
  input.placeholder = 'New nickname';
  input.value = getOpt(f, 'PlayerName', '');
  panel.append(input);

  const applyBtn = document.createElement('button');
  applyBtn.type = 'button';
  applyBtn.className = 'btn';
  applyBtn.textContent = 'Apply name';
  applyBtn.addEventListener('click', async () => {
    try {
      await api('/api/setname/apply', { name: input.value });
      $status.textContent = 'Name applied';
      $status.className = 'status ok';
    } catch (e) {
      $status.textContent = 'Name apply failed (enable feature?)';
      $status.className = 'status';
    }
  });
  panel.append(applyBtn);
  card.append(panel);
}

function renderFeature(f) {
  const card = document.createElement('div');
  card.className = 'card';
  const head = document.createElement('div');
  head.className = 'card-head';
  head.innerHTML = `<div><div class="cat">${f.category}</div><div class="title">${f.title}</div></div>`;
  const sw = document.createElement('label');
  sw.className = 'switch';
  const inp = document.createElement('input');
  inp.type = 'checkbox';
  inp.checked = !!f.enabled;
  inp.addEventListener('change', async () => {
    try {
      await api('/api/feature', { id: f.id, enabled: inp.checked });
      f.enabled = inp.checked;
      const panels = card.querySelectorAll('.panel');
      panels.forEach(p => p.classList.toggle('disabled', !inp.checked));
      $status.textContent = 'Saved';
      $status.className = 'status ok';
    } catch (e) {
      inp.checked = !inp.checked;
      $status.textContent = 'Failed to save';
      $status.className = 'status';
    }
  });
  const slider = document.createElement('span');
  slider.className = 'slider';
  sw.append(inp, slider);
  head.append(sw);
  card.append(head);

  if (f.id === 'Teleport') {
    renderTeleportPanel(card, f);
    return card;
  }

  if (f.id === 'SetName') {
    renderSetNamePanel(card, f);
    return card;
  }

  const boolOpts = (f.options || []).filter(o => o.type === 'bool');
  const floatOpts = (f.options || []).filter(o => o.type === 'float');
  if (boolOpts.length || floatOpts.length) {
    const opts = document.createElement('div');
    opts.className = 'opts panel' + (f.enabled ? '' : ' disabled');
    for (const o of boolOpts) {
      const row = document.createElement('div');
      row.className = 'opt';
      const lbl = document.createElement('label');
      lbl.textContent = o.key;
      const mini = document.createElement('label');
      mini.className = 'switch';
      const cb = document.createElement('input');
      cb.type = 'checkbox';
      cb.checked = !!o.value;
      cb.addEventListener('change', async () => {
        try {
          await api('/api/option', { id: f.id, key: o.key, value: cb.checked });
        } catch (e) {
          cb.checked = !cb.checked;
        }
      });
      const sl = document.createElement('span');
      sl.className = 'slider';
      mini.append(cb, sl);
      row.append(lbl, mini);
      opts.append(row);
    }
    for (const o of floatOpts) renderFloatOpt(f, o, opts);
    card.append(opts);
  }
  return card;
}

async function refreshStatus() {
  try {
    const data = await api('/api/state');
    $status.textContent = 'Connected · ' + (data.features?.length || 0) + ' features';
    $status.className = 'status ok';
  } catch (e) {
    $status.textContent = 'Cannot reach cheat — is the game running?';
    $status.className = 'status';
  }
}

async function load() {
  try {
    const data = await api('/api/state');
    $features.innerHTML = '';
    if (!data.features || !data.features.length) {
      $features.innerHTML = '<div class="empty">No features</div>';
    } else {
      for (const f of data.features) $features.append(renderFeature(f));
    }
    $status.textContent = 'Connected · ' + (data.features?.length || 0) + ' features';
    $status.className = 'status ok';
  } catch (e) {
    $status.textContent = 'Cannot reach cheat — is the game running?';
    $status.className = 'status';
  }
}

load();
setInterval(refreshStatus, 3000);
</script>
</body>
</html>)HTML";
}
