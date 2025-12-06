// Carga noticias desde raw.githubusercontent (si existe)
const newsEl = document.getElementById('newsContent');
fetch('https://raw.githubusercontent.com/Omymnr/HB-Repository/main/updates/news.txt')
  .then(r => { if (!r.ok) throw new Error('No news'); return r.text(); })
  .then(txt => { newsEl.textContent = txt; })
  .catch(_ => { newsEl.textContent = 'No hay noticias disponibles.'; });

// Estado del servidor (intenta leer updates/status.json en GH Pages)
const serverText = document.getElementById('serverText');
const serverBadge = document.getElementById('serverBadge');
const playersCount = document.getElementById('playersCount');

function setServerStatus(online, players){
  const dot = serverBadge.querySelector('.dot');
  if(online){
    dot.style.background = '#2ecc71';
    serverText.textContent = 'Online';
    playersCount.textContent = players != null ? players : '-';
  } else {
    dot.style.background = '#e74c3c';
    serverText.textContent = 'Offline';
    playersCount.textContent = '-';
  }
}

fetch('/updates/status.json').then(r => {
  if(!r.ok) throw new Error('no status');
  return r.json();
}).then(js => {
  setServerStatus(js.online === true, js.players || 0);
}).catch(_ => {
  // Fallback: no status endpoint — intenta leer versión para comprobar conexión a GH
  fetch('https://raw.githubusercontent.com/Omymnr/HB-Repository/main/updates/version.txt')
    .then(r => { if (!r.ok) throw new Error('no v'); return r.text(); })
    .then(v => { setServerStatus(true, null); })
    .catch(__ => { setServerStatus(false, null); });
});
