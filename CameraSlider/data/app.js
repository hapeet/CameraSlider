'use strict';
const $ = id => document.getElementById(id);
const qa = s => document.querySelectorAll(s);

let ws, S = {}, maxPos = 600, homed = false, jogTimer = null, saveTimer = null;

// ---------------- WebSocket ----------------
function connect() {
  ws = new WebSocket(`ws://${location.host}/ws`);
  ws.onopen = () => $('conn').classList.add('on');
  ws.onclose = () => { $('conn').classList.remove('on'); holdEnd(); setTimeout(connect, 1500); };
  ws.onmessage = e => {
    let m; try { m = JSON.parse(e.data); } catch (_) { return; }
    if (m.type === 'status') onStatus(m);
    else if (m.type === 'settings') onSettings(m.s);
    else if (m.type === 'msg') toast(m.text, m.err);
    else if (m.type === 'scan') onScan(m.list);
  };
}
function send(o) {
  if (ws && ws.readyState === 1) ws.send(JSON.stringify(o));
  else toast('Not connected to the slider', true);
}
function toast(t, err) {
  const d = document.createElement('div');
  d.className = 'toast' + (err ? ' err' : '');
  d.textContent = t;
  $('toasts').appendChild(d);
  setTimeout(() => d.remove(), err ? 6000 : 3000);
}

const f1 = v => (Math.round(v * 10) / 10).toFixed(1);
const pct = v => Math.max(0, Math.min(100, v / maxPos * 100)) + '%';

// ---------------- stav ----------------
const ST = { Idle: 'Ready', Homing: 'Homing…', Moving: 'Moving', Jogging: 'Jogging', Program: 'Program running' };
const PH = { ToStart: 'moving to From', ToB: 'moving → To', WaitB: 'waiting at To', ToA: 'moving → From', WaitA: 'waiting at From' };

function onStatus(x) {
  const m = x.m, w = x.w;
  homed = m.homed;
  $('app').classList.toggle('locked', !homed);

  $('pos').textContent = homed ? f1(m.pos) : '?';
  const st = $('state');
  st.textContent = ST[m.state] || m.state;
  st.className = 'badge' + (m.state === 'Idle' ? '' : ' warn');
  $('phase').textContent = m.state === 'Program' ? (m.mode === 'cycle' ? 'Cycling: ' : 'Single cycle: ') + (PH[m.phase] || '') : '';

  const h = $('hdrHome');
  h.textContent = homed ? 'Homed' : (m.state === 'Homing' ? 'Homing…' : 'Not homed');
  h.className = 'badge ' + (homed ? 'ok' : 'err');

  $('spd').textContent = f1(Math.abs(m.speed || 0)) + ' mm/s';
  $('tgt').textContent = homed ? f1(m.target) + ' mm' : '–';
  $('cyc').textContent = m.state === 'Program' ? `${m.done} / ${m.total || '∞'}` : '–';
  $('wait').textContent = m.waitLeft > 0 ? (m.shotWait ? (m.shotWait === 'pre' ? 'before shot ' : 'after shot ') : '') + f1(m.waitLeft) + ' s' : '–';
  $('shots').textContent = m.shots !== undefined ? m.shots + (m.shotsMissed ? ` (${m.shotsMissed} skipped)` : '') : '–';
  onCamera(x.cam || {});
  $('hall').innerHTML = m.hall ? '<span class="badge warn">ACTIVE</span>' : 'free';
  $('drv').innerHTML = (m.driver ? '<span class="badge ok">OK</span>' : '<span class="badge err">UART error</span>') +
    (m.drvFails ? ` <span class="hint" title="UART read failures since boot">${m.drvFails}×</span>` : '');

  const car = $('vCar');
  car.style.left = pct(homed ? m.pos : 0);
  car.style.opacity = homed ? 1 : .35;
  const tg = $('vTgt');
  tg.style.left = pct(m.target);
  tg.style.display = homed && m.state !== 'Idle' && m.state !== 'Homing' ? 'block' : 'none';

  $('jmpWarn').hidden = x.jmp !== false;

  let wi = `Access point: <b>CameraSlider</b> – http://${w.apIp} (clients: ${w.apCl})<br>`;
  if (w.sta) wi += `Network: <b>${esc(w.ssid)}</b> – http://${w.ip} (${w.rssi} dBm)`;
  else if (w.ssid) wi += `Network: <b>${esc(w.ssid)}</b> – not connected`;
  else wi += 'Network: not configured';
  $('wifiInfo').innerHTML = wi;
}
const CAM = {
  none: ['Not paired', 'err'], idle: ['Not connected', 'warn'], pairing: ['Pairing…', 'warn'],
  connecting: ['Connecting…', 'warn'], connected: ['Connected', 'ok'],
};
function onCamera(c) {
  const [t, cls] = CAM[c.st] || ['–', ''];
  $('camSt').textContent = t;
  $('camSt').className = 'badge ' + cls;
  $('camStat').innerHTML = `<span class="badge ${cls}">${t}</span>`;
  $('camName').textContent = c.name ? c.name + (c.shots ? ` · ${c.shots} shots since boot` : '') : '';
  $('bCamShot').disabled = c.st !== 'connected';
  $('bCamForget').disabled = c.st === 'none';
  $('bCamPair').textContent = c.st === 'pairing' ? 'Searching…' : (c.st === 'none' ? 'Pair camera' : 'Pair again');
}

const esc = s => String(s).replace(/[&<>"]/g, c => ({ '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;' }[c]));

// ---------------- nastavení ----------------
function onSettings(s) {
  S = s;
  maxPos = s.maxPos || 600;
  qa('[data-k]').forEach(el => {
    const k = el.dataset.k;
    if (!(k in s) || el === document.activeElement) return;
    if (el.type === 'checkbox') el.checked = !!s[k];
    else if (el.type === 'radio') el.checked = (el.value === '1') === !!s[k];
    else el.value = s[k];
  });
  if (document.activeElement !== $('wSsid')) $('wSsid').value = s.ssid || '';
  $('vMax').textContent = maxPos + ' mm';
  $('gotoPos').max = maxPos;
  ['rangeFrom', 'rangeTo'].forEach(k => document.querySelector(`[data-k="${k}"]`).max = maxPos);
  drawRange();
  updateSpm();
  updateCountVis();
  updatePhoto();
}

function collect(group) {
  const o = {};
  qa(`[data-g="${group}"][data-k]`).forEach(el => {
    const k = el.dataset.k;
    if (el.type === 'radio') { if (el.checked) o[k] = el.value === '1'; }
    else if (el.type === 'checkbox') o[k] = el.checked;
    else { const v = parseFloat(el.value); if (!isNaN(v)) o[k] = v; }
  });
  return o;
}

function saveMotion() {
  clearTimeout(saveTimer);
  send({ cmd: 'saveMotion', s: collect('motion') });
  $('saved').textContent = '· saved ✓';
  setTimeout(() => $('saved').textContent = '', 1500);
}

function drawRange() {
  const a = parseFloat(document.querySelector('[data-k="rangeFrom"]').value) || 0;
  const b = parseFloat(document.querySelector('[data-k="rangeTo"]').value) || 0;
  const lo = Math.min(a, b), hi = Math.max(a, b);
  const r = $('vRng');
  r.style.left = pct(lo);
  r.style.width = Math.max(0, Math.min(hi, maxPos) - lo) / maxPos * 100 + '%';
  $('vRngLbl').textContent = `From ${f1(a)} → To ${f1(b)}`;
}

function updateSpm() {
  const c = collect('config');
  if (!c.steps100 || !c.microsteps) return;
  const spm = c.steps100 * c.microsteps / 100;
  $('spmInfo').textContent = `${spm.toFixed(2)} microsteps/mm · resolution ${(1000 / spm).toFixed(1)} µm`;
}

function updateCountVis() {
  $('cntWrap').style.display = $('ci1').checked ? 'none' : '';
}

function updatePhoto() {
  const m = collect('motion');
  const slow = $('ps1').checked;
  $('photoBox').classList.toggle('dim', !m.photoOn);
  $('slowWrap').style.display = slow ? '' : 'none';
  $('preLbl').firstChild.textContent = slow ? 'Slow run before shot [s]' : 'Wait before shot [s]';
  $('postLbl').firstChild.textContent = slow ? 'Slow run after shot [s]' : 'Wait after shot [s]';

  const len = Math.abs((m.rangeTo || 0) - (m.rangeFrom || 0));
  if (!(m.photoStep > 0) || !len) { $('photoInfo').textContent = ''; return; }
  const n = Math.floor(len / m.photoStep + 0.001) + 1;
  const last = (n - 1) * m.photoStep;
  let t = `${n} shots per pass (From + every ${f1(m.photoStep)} mm)`;
  if (len - last > 0.05) t += ` – last shot ${f1(len - last)} mm before “To”`;
  if (slow) {
    const v = Math.min(m.photoSlowSpeed || 0, m.speedOut || 0);
    t += `. Around each point the carriage slows to ${v} mm/s for ${f1(v * ((m.photoPre || 0) + (m.photoPost || 0)))} mm and fires while passing it.`;
    if (v * ((m.photoPre || 0) + (m.photoPost || 0)) >= m.photoStep) t += ' Slow zones overlap – the carriage will stay slow the whole way.';
  } else {
    const per = (m.photoPre || 0) + Math.max(m.photoPost || 0, 0.25);
    const total = n * per + len / (m.speedOut || 1);
    t += `. Carriage stops at each point. One pass takes ≈ ${fmtTime(total)} (without acceleration).`;
  }
  $('photoInfo').textContent = t;
}
const fmtTime = s => s >= 3600 ? `${Math.floor(s / 3600)} h ${Math.round(s % 3600 / 60)} min`
  : s >= 60 ? `${Math.floor(s / 60)} min ${Math.round(s % 60)} s` : `${Math.round(s)} s`;

// ---------------- WiFi scan ----------------
function onScan(list) {
  const n = $('nets');
  n.innerHTML = '';
  if (!list.length) { n.innerHTML = '<div class="hint">No networks found</div>'; return; }
  list.sort((a, b) => b.rssi - a.rssi).forEach(e => {
    const b = document.createElement('button');
    b.innerHTML = `<span>${e.open ? '' : '🔒 '}${esc(e.ssid)}</span><span class="hint">${e.rssi} dBm</span>`;
    b.onclick = () => { $('wSsid').value = e.ssid; $('wPass').focus(); };
    n.appendChild(b);
  });
}

// ---------------- ruční posuv ----------------
function holdStart(dir, btn) {
  holdEnd();
  btn.classList.add('active');
  send({ cmd: 'jogHold', dir });
  jogTimer = setInterval(() => send({ cmd: 'jogHold', dir }), 200);
}
function holdEnd() {
  qa('.hold.active').forEach(b => b.classList.remove('active'));
  if (jogTimer) {
    clearInterval(jogTimer);
    jogTimer = null;
    if (ws && ws.readyState === 1) ws.send('{"cmd":"jogRelease"}');
  }
}

// ---------------- události ----------------
$('bHome').onclick = () => send({ cmd: 'home' });
$('bStop').onclick = () => send({ cmd: 'stop' });
$('bOff').onclick = () => { if (confirm('Release motor? Position will be lost and homing will be required.')) send({ cmd: 'disable' }); };
$('bGoto').onclick = () => {
  const v = parseFloat($('gotoPos').value);
  if (isNaN(v)) return toast('Enter a target position', true);
  send({ cmd: 'goto', pos: v });
};
qa('[data-d]').forEach(b => b.onclick = () => send({ cmd: 'jogStep', d: parseFloat(b.dataset.d) }));
qa('.hold').forEach(b => {
  b.addEventListener('pointerdown', e => { e.preventDefault(); b.setPointerCapture(e.pointerId); holdStart(+b.dataset.dir, b); });
  ['pointerup', 'pointercancel', 'lostpointercapture'].forEach(ev => b.addEventListener(ev, holdEnd));
  b.addEventListener('contextmenu', e => e.preventDefault());
});
window.addEventListener('blur', holdEnd);
document.addEventListener('visibilitychange', holdEnd);

$('rail').onclick = e => {
  const r = $('rail').getBoundingClientRect();
  const mm = Math.max(0, Math.min(maxPos, (e.clientX - r.left) / r.width * maxPos));
  $('gotoPos').value = f1(mm);
  toast(`Target ${f1(mm)} mm – press “Go to position”`);
};

qa('[data-g="motion"]').forEach(el => el.addEventListener('change', () => {
  clearTimeout(saveTimer);
  saveTimer = setTimeout(saveMotion, 300);
}));
qa('[data-k="rangeFrom"],[data-k="rangeTo"]').forEach(el => el.addEventListener('input', drawRange));
qa('[name="ci"]').forEach(el => el.addEventListener('change', updateCountVis));
qa('[data-g="motion"]').forEach(el => el.addEventListener('input', updatePhoto));
qa('[data-g="config"]').forEach(el => el.addEventListener('input', updateSpm));

$('bCycle').onclick = () => { saveMotion(); send({ cmd: 'cycle' }); };
$('bSingle').onclick = () => { saveMotion(); send({ cmd: 'single' }); };

$('bCamPair').onclick = () => send({ cmd: 'camPair' });
$('bCamShot').onclick = () => send({ cmd: 'camShot' });
$('bCamForget').onclick = () => { if (confirm('Forget the paired camera? You will have to pair it again.')) send({ cmd: 'camForget' }); };

$('bCfg').onclick = () => send({ cmd: 'saveConfig', s: collect('config') });
$('bFactory').onclick = () => { if (confirm('Restore factory settings? Everything including WiFi will be erased.')) send({ cmd: 'factoryReset' }); };

$('bScan').onclick = () => { $('nets').innerHTML = '<div class="hint">Scanning…</div>'; send({ cmd: 'scan' }); };
$('bWifi').onclick = () => {
  const ssid = $('wSsid').value.trim();
  if (!ssid) return toast('Enter SSID', true);
  send({ cmd: 'saveWifi', ssid, pass: $('wPass').value });
};
$('bWifiForget').onclick = () => { if (confirm('Forget the WiFi network?')) send({ cmd: 'saveWifi', ssid: '', pass: '' }); };

// Space / Esc = STOP (outside input fields)
document.addEventListener('keydown', e => {
  if ((e.key === ' ' || e.key === 'Escape') && !['INPUT', 'SELECT', 'TEXTAREA'].includes(document.activeElement.tagName)) {
    e.preventDefault();
    send({ cmd: 'stop' });
  }
});

connect();
