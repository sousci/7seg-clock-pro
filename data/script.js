const $ = (selector) => document.querySelector(selector);
const status = $('#status');
const result = $('#result');
const brightness = $('#brightness');

const hex = (color) => `#${[color.r, color.g, color.b].map(v => Number(v).toString(16).padStart(2, '0')).join('')}`;
const rgb = (value) => ({r: parseInt(value.slice(1, 3), 16), g: parseInt(value.slice(3, 5), 16), b: parseInt(value.slice(5, 7), 16)});
const timeString = (hour, minute) => `${String(hour).padStart(2, '0')}:${String(minute).padStart(2, '0')}`;

function addChime(value = {enabled: true, hour: 8, minute: 30, weekdays: 62}) {
  if ($('#chimes').children.length >= 24) return;
  const row = document.createElement('div');
  row.className = 'chime';
  row.innerHTML = `
    <label><input class="enabled" type="checkbox"> 有効</label>
    <input class="time" type="time">
    <label>月〜金 <input class="weekdays" type="checkbox"></label>
    <button type="button">削除</button>
  `;
  row.querySelector('.enabled').checked = value.enabled;
  row.querySelector('.time').value = timeString(value.hour, value.minute);
  row.querySelector('.weekdays').checked = (value.weekdays & 62) === 62;
  row.querySelector('button').onclick = () => row.remove();
  $('#chimes').append(row);
}

brightness.addEventListener('input', () => $('#brightnessValue').value = brightness.value);

async function getJson(path) {
  const response = await fetch(path);
  if (!response.ok) throw new Error(`HTTP ${response.status}`);
  return response.json();
}

async function load() {
  const [config, device] = await Promise.all([getJson('/api/config'), getJson('/api/status')]);
  const form = $('#config');
  form.webAdminUser.value = config.webAdminUser || 'admin';
  form.webAdminPassword.value = '';
  form.otaPassword.value = '';
  form.wifiSsid.value = config.wifiSsid || '';
  form.wifiPassword.value = '';
  form.ntp0.value = (config.ntpServers || [])[0] || 'ntp.nict.jp';
  brightness.value = config.brightness ?? 160;
  $('#brightnessValue').value = brightness.value;
  form.ledMode.value = config.ledMode ?? 2;
  form.solidColor.value = hex(config.solidColor || {r:0,g:128,b:255});
  [0, 1, 2, 3].forEach(i => form[`digit${i}`].value = hex((config.digitColors || [])[i] || {r:255,g:255,b:255}));
  form.animationSpeed.value = config.animationSpeed ?? 32;
  form.sleepEnabled.checked = config.sleepEnabled;
  form.sleepStart.value = timeString(config.sleepStartHour || 22, config.sleepStartMinute || 0);
  form.sleepEnd.value = timeString(config.sleepEndHour || 6, config.sleepEndMinute || 0);
  $('#chimes').replaceChildren();
  (config.chimes || []).filter(c => c.enabled).forEach(addChime);
  status.textContent = device.stationConnected
    ? `v${device.firmwareVersion || '-'} / Wi-Fi接続済み: ${device.stationIp}${device.timeValid ? ' / NTP同期済み' : ' / NTP同期中'}`
    : `v${device.firmwareVersion || '-'} / 設定用AP: ${device.apIp}`;
  if (device.otaActive) status.textContent += ` / OTA active: ${device.otaRemainingSeconds}s`;
}

$('#config').addEventListener('submit', async (event) => {
  event.preventDefault();
  const form = event.currentTarget;
  const payload = {
    webAdminUser: form.webAdminUser.value.trim() || 'admin',
    wifiSsid: form.wifiSsid.value.trim(),
    ntpServers: [form.ntp0.value.trim(), 'time.google.com', 'ntp.jst.mfeed.ad.jp'],
    brightness: Number(brightness.value),
    ledMode: Number(form.ledMode.value),
    solidColor: rgb(form.solidColor.value),
    digitColors: [0, 1, 2, 3].map(i => rgb(form[`digit${i}`].value)),
    animationSpeed: Number(form.animationSpeed.value),
    sleepEnabled: form.sleepEnabled.checked,
    sleepStartHour: Number(form.sleepStart.value.slice(0,2)),
    sleepStartMinute: Number(form.sleepStart.value.slice(3,5)),
    sleepEndHour: Number(form.sleepEnd.value.slice(0,2)),
    sleepEndMinute: Number(form.sleepEnd.value.slice(3,5)),
    chimes: [...document.querySelectorAll('.chime')].map(row => {
      const [h, m] = row.querySelector('.time').value.split(':').map(Number);
      return {
        enabled: row.querySelector('.enabled').checked,
        hour: h,
        minute: m,
        melodyId: 0,
        weekdays: row.querySelector('.weekdays').checked ? 62 : 127
      };
    }),
  };
  if (form.webAdminPassword.value) payload.webAdminPassword = form.webAdminPassword.value;
  if (form.otaPassword.value) payload.otaPassword = form.otaPassword.value;
  if (form.wifiPassword.value) payload.wifiPassword = form.wifiPassword.value;

  result.className = '';
  result.textContent = '保存中...';
  try {
    const response = await fetch('/api/config', {method: 'POST', headers: {'Content-Type': 'application/json'}, body: JSON.stringify(payload)});
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
    result.className = 'ok';
    result.textContent = '保存しました。認証情報を変更した場合は、次回アクセス時に新しい認証情報を使用してください。';
    setTimeout(load, 3000);
  } catch (error) {
    result.className = 'error';
    result.textContent = `保存に失敗しました: ${error.message}`;
  }
});

$('#addChime').onclick = () => addChime();

$('#setTime').onclick = async () => {
  const value = $('#manualTime').value;
  if (!value) return;
  const response = await fetch('/api/time', {
    method:'POST',
    headers:{'Content-Type':'application/x-www-form-urlencoded'},
    body:`epoch=${Math.floor(new Date(value).getTime()/1000)}`
  });
  result.textContent = response.ok ? '手動時刻を設定しました。' : '手動時刻の設定に失敗しました。';
};

$('#syncNtp').onclick = async () => {
  const response = await fetch('/api/ntp-sync', {method:'POST'});
  result.textContent = response.ok ? 'NTP同期を開始しました。完了まで数十秒待ってください。' : 'NTP同期要求に失敗しました。';
};

$('#debugToggle').onclick = () => $('#debugButtons').classList.toggle('hidden');
document.querySelectorAll('[data-debug]').forEach((button) => {
  button.onclick = async () => {
    result.className = '';
    result.textContent = `${button.textContent}: started`;
    try {
      const response = await fetch(button.dataset.debug, {method:'POST'});
      if (!response.ok) throw new Error(`HTTP ${response.status}`);
    } catch (error) {
      result.className = 'error';
      result.textContent = `${button.textContent}: failed: ${error.message}`;
    }
  };
});

load().catch((error) => { status.textContent = `状態取得に失敗しました: ${error.message}`; });
