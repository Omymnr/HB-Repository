// Carga noticias desde raw.githubusercontent (si existe)
const newsEl = document.getElementById('newsContent');
fetch('https://raw.githubusercontent.com/Omymnr/HB-Repository/main/updates/news.txt')
  .then(r => { if (!r.ok) throw new Error('No news'); return r.text(); })
  .then(txt => { newsEl.textContent = txt; })
  .catch(_ => { newsEl.textContent = 'No hay noticias disponibles.'; });

// Estado del servidor (placeholder)
const statusEl = document.getElementById('serverStatus');
// Aquí podrías implementar una llamada a tu servidor para verificar estado real
statusEl.textContent = 'Online (placeholder)';
