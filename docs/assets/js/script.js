// Carga noticias desde raw.githubusercontent (si existe)
const newsEl = document.getElementById('newsContent');
fetch('https://raw.githubusercontent.com/Omymnr/HB-Repository/main/updates/news.txt')
  .then(r => { if (!r.ok) throw new Error('No news'); return r.text(); })
  .then(txt => { newsEl.textContent = txt; })
  .catch(_ => { newsEl.textContent = 'No hay noticias disponibles.'; });

// Estado del servidor: usa updates/status.json si está disponible
const serverText = document.getElementById('serverText');
const serverBadge = document.getElementById('serverBadge');
const serverMessage = document.getElementById('serverMessage');
const livePlayers = document.getElementById('livePlayers');
const currentVersion = document.getElementById('currentVersion');

function applyStatus(js){
  const dot = serverBadge.querySelector('.dot');
  if(js && js.online){
    dot.style.background = '#2ecc71';
    serverText.textContent = 'Online';
    livePlayers.textContent = js.players != null ? js.players : '-';
    serverMessage.textContent = js.message || '';
  } else {
    dot.style.background = '#e74c3c';
    serverText.textContent = 'Offline';
    livePlayers.textContent = '-';
    serverMessage.textContent = js && js.message ? js.message : '';
  }
}

// intenta leer status.json desde GH Pages site (relativo)
fetch('/updates/status.json').then(r => {
  if(!r.ok) throw new Error('no status');
  return r.json();
}).then(js => { applyStatus(js); }).catch(_ => {
  // fallback: intenta leer raw GH version y set online=true si accesible
  fetch('https://raw.githubusercontent.com/Omymnr/HB-Repository/main/updates/version.txt')
    .then(r => { if(!r.ok) throw new Error('no v'); return r.text(); })
    .then(v => { currentVersion.textContent = v.trim(); applyStatus({online:true,players:null}); })
    .catch(__ => { applyStatus({online:false}); });
});

// Lee version actual para hero (mejor experiencia)
fetch('https://raw.githubusercontent.com/Omymnr/HB-Repository/main/updates/version.txt')
  .then(r => { if(!r.ok) throw new Error('no v'); return r.text(); })
  .then(v => { currentVersion.textContent = v.trim(); })
  .catch(_ => { currentVersion.textContent = '?'; });

// --- Simple gallery/lightbox ---
const galleryItems = document.querySelectorAll('.gallery-item');
let lightbox;
function createLightbox(){
  lightbox = document.createElement('div');
  lightbox.className = 'lightbox';
  lightbox.innerHTML = '<img src="" alt="">';
  lightbox.addEventListener('click', () => { lightbox.classList.remove('open'); });
  document.body.appendChild(lightbox);
}
createLightbox();

galleryItems.forEach(a => {
  a.addEventListener('click', e => {
    e.preventDefault();
    const img = lightbox.querySelector('img');
    img.src = a.href;
    lightbox.classList.add('open');
  });
});

// Populate hero info box: launcher version, latest news line, small server status
function populateHeroBox(){
  const launcherEl = document.getElementById('launcherVersion');
  const newsElSmall = document.getElementById('latestNews');
  const smallServer = document.getElementById('smallServerStatus');
  const smallPlayers = document.getElementById('smallPlayers');

  // launcher version
  fetch('https://raw.githubusercontent.com/Omymnr/HB-Repository/main/updates/version.txt')
    .then(r => r.ok ? r.text() : Promise.reject())
    .then(t => { if(launcherEl) launcherEl.textContent = t.trim(); })
    .catch(_ => { if(launcherEl) launcherEl.textContent = '?'; });

  // latest news first non-empty line
  fetch('https://raw.githubusercontent.com/Omymnr/HB-Repository/main/updates/news.txt')
    .then(r => r.ok ? r.text() : Promise.reject())
    .then(tx => {
      if(!newsElSmall) return;
      const lines = tx.split(/\r?\n/).map(s=>s.trim()).filter(Boolean);
      newsElSmall.textContent = lines.length ? lines[0] : 'Sin noticias recientes.';
    }).catch(_ => { if(newsElSmall) newsElSmall.textContent = 'Sin noticias.'; });

  // server status small (from updates/status.json)
  fetch('/updates/status.json').then(r => {
    if(!r.ok) throw new Error('no status');
    return r.json();
  }).then(js => {
    if(smallServer) smallServer.textContent = js.online ? 'Online' : 'Offline';
    if(smallPlayers) smallPlayers.textContent = js.players != null ? js.players : '-';
  }).catch(_ => {
    // fallback: try raw GH version presence
    fetch('https://raw.githubusercontent.com/Omymnr/HB-Repository/main/updates/version.txt')
      .then(r=> r.ok ? r.text() : Promise.reject()).then(_=> { if(smallServer) smallServer.textContent = 'Online (GH)'; if(smallPlayers) smallPlayers.textContent = '-'; })
      .catch(__ => { if(smallServer) smallServer.textContent = 'Desconocido'; if(smallPlayers) smallPlayers.textContent = '-'; });
  });
}

document.addEventListener('DOMContentLoaded', () => { populateHeroBox(); });

// Render news page with cards (splits news.txt by blank line)
function renderNewsPage(){
  const newsList = document.getElementById('newsList');
  if(!newsList) return;
  fetch('https://raw.githubusercontent.com/Omymnr/HB-Repository/main/updates/news.txt')
    .then(r => { if(!r.ok) throw new Error('no news'); return r.text(); })
    .then(txt => {
      const entries = txt.split(/\n\s*\n/).map(e => e.trim()).filter(Boolean);
      newsList.innerHTML = '';
      entries.forEach(e => {
        const lines = e.split(/\r?\n/).map(s=>s.trim()).filter(Boolean);
        const title = lines[0] || 'Noticia';
        const body = lines.slice(1).join(' ');
        const card = document.createElement('article');
        card.className = 'news-card';
        card.innerHTML = `<h3>${escapeHtml(title)}</h3><div class="news-meta">${new Date().toLocaleDateString()}</div><p class="news-excerpt">${escapeHtml(body)}</p>`;
        newsList.appendChild(card);
      });
    }).catch(_ => { newsList.innerHTML = '<div class="news-card">No hay noticias disponibles.</div>'; });
}

function escapeHtml(s){ return s.replace(/[&<>\"]/g, c=> ({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;'}[c])); }

document.addEventListener('DOMContentLoaded', () => { renderNewsPage(); });
