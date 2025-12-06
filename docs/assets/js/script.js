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
