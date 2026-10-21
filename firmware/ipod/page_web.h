// Page web servie par l'ESP32 (HTML + CSS + JS dans un seul fichier, stocke en flash)
#pragma once

const char PAGE_WEB[] PROGMEM = R"rawliteral(<!doctype html>
<html lang="fr">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>iPod OK Computer</title>
<style>
:root{--fond:#f2f2f4;--carte:#fff;--texte:#1c1c1e;--doux:#6e6e73;--ligne:#e3e3e8;--bleu:#1e91fa}
@media (prefers-color-scheme:dark){:root{--fond:#000;--carte:#1c1c1e;--texte:#f2f2f7;--doux:#98989f;--ligne:#2c2c2e}}
*{box-sizing:border-box}
body{margin:0;background:var(--fond);color:var(--texte);font:16px/1.4 -apple-system,system-ui,Segoe UI,Roboto,sans-serif}
main{max-width:560px;margin:0 auto;padding:16px}
h1{font-size:20px;margin:4px 0 16px;text-align:center}
h2{font-size:15px;text-transform:uppercase;letter-spacing:.05em;color:var(--doux);margin:24px 4px 8px}
.carte{background:var(--carte);border-radius:14px;padding:16px}
#lecture{display:flex;gap:14px;align-items:center}
#pochette{width:96px;height:96px;border-radius:8px;background:var(--ligne);object-fit:cover;flex:none}
#infos{min-width:0;flex:1}
#titre{font-weight:600;font-size:18px;white-space:nowrap;overflow:hidden;text-overflow:ellipsis}
#sous{color:var(--doux);font-size:14px;white-space:nowrap;overflow:hidden;text-overflow:ellipsis}
#rang{color:var(--doux);font-size:12px;margin-top:2px}
.barre{height:6px;background:var(--ligne);border-radius:3px;margin-top:14px;overflow:hidden}
.barre div{height:100%;width:0;background:var(--bleu)}
.temps{display:flex;justify-content:space-between;font-size:12px;color:var(--doux);margin-top:4px;font-variant-numeric:tabular-nums}
.commandes{display:flex;justify-content:center;align-items:center;gap:18px;margin-top:10px}
button{font:inherit;border:0;background:none;color:var(--texte);cursor:pointer}
.rond{width:52px;height:52px;border-radius:50%;background:var(--ligne);font-size:20px}
.rond.grand{width:64px;height:64px;background:var(--bleu);color:#fff;font-size:24px}
.volume{display:flex;align-items:center;gap:12px;margin-top:14px}
.volume button{width:40px;height:40px;border-radius:50%;background:var(--ligne);font-size:22px}
.volume .barre{flex:1;margin:0}
#vol{width:28px;text-align:right;font-size:13px;color:var(--doux);font-variant-numeric:tabular-nums}
input[type=search]{width:100%;padding:10px 12px;border-radius:10px;border:1px solid var(--ligne);background:var(--carte);color:var(--texte);font:inherit;margin-bottom:10px}
.artiste{font-weight:700;padding:12px 4px 4px}
.album{color:var(--doux);font-size:13px;padding:6px 4px 2px}
.morceau{display:flex;gap:10px;padding:10px 12px;background:var(--carte);border-bottom:1px solid var(--ligne);cursor:pointer}
.morceau:hover{background:var(--ligne)}
.morceau span{color:var(--doux);width:22px;text-align:right;flex:none;font-variant-numeric:tabular-nums}
.morceau.actif{color:var(--bleu);font-weight:600}
#depot{border:2px dashed var(--ligne);border-radius:14px;padding:24px;text-align:center;color:var(--doux);cursor:pointer}
#depot.survol{border-color:var(--bleu);color:var(--bleu)}
.envoi{display:flex;justify-content:space-between;gap:10px;font-size:14px;padding:8px 4px;border-bottom:1px solid var(--ligne)}
.envoi b{font-weight:500;overflow:hidden;text-overflow:ellipsis;white-space:nowrap}
.envoi i{font-style:normal;color:var(--doux);flex:none}
.envoi i.ok{color:#34c759}.envoi i.err{color:#ff3b30}
</style>
</head>
<body>
<main>
<h1>iPod OK Computer</h1>

<div class="carte" id="telecommande">
  <div id="lecture">
    <img id="pochette" alt="">
    <div id="infos">
      <div id="titre">Rien en lecture</div>
      <div id="sous"></div>
      <div id="rang"></div>
    </div>
  </div>
  <div class="barre"><div id="prog"></div></div>
  <div class="temps"><span id="ecoule">0:00</span><span id="reste">-0:00</span></div>
  <div class="commandes">
    <button class="rond" onclick="cmd('b')" aria-label="Précédent">&#9198;</button>
    <button class="rond grand" id="bpause" onclick="cmd('p')" aria-label="Lecture / pause">&#9199;</button>
    <button class="rond" onclick="cmd('n')" aria-label="Suivant">&#9197;</button>
  </div>
  <div class="volume">
    <button onclick="cmd('-')" aria-label="Volume moins">&minus;</button>
    <div class="barre"><div id="vbar"></div></div>
    <span id="vol">0</span>
    <button onclick="cmd('+')" aria-label="Volume plus">+</button>
  </div>
</div>

<h2>Envoyer des MP3</h2>
<div id="depot">Touchez ou glissez des fichiers .mp3 ici</div>
<input type="file" id="fichiers" accept=".mp3,audio/mpeg" multiple hidden>
<div id="envois"></div>

<div id="zoneBiblio">
<h2>Bibliothèque</h2>
<input type="search" id="recherche" placeholder="Rechercher un titre, un artiste, un album">
<div id="biblio"></div>
</div>
</main>

<script>
const $ = id => document.getElementById(id);
let biblio = [], courant = -1, pochetteDe = null;

function minSec(s){ s = Math.max(0, s|0); return (s/60|0) + ':' + String(s%60).padStart(2,'0'); }

let minuteur = null;
function afficherEtat(e){
  if (e.telecommande === false){   // version finale : seulement l'envoi de MP3
    $('telecommande').hidden = true; $('zoneBiblio').hidden = true;
    clearInterval(minuteur);
    return;
  }
  $('vol').textContent = e.volume;
  $('vbar').style.width = (e.volume/e.volmax*100) + '%';
  $('bpause').innerHTML = (e.titre && !e.pause) ? '&#9208;' : '&#9654;';
  if (!e.titre){ return; }
  $('titre').textContent = e.titre;
  $('sous').textContent = e.artiste + ' : ' + e.album;
  $('rang').textContent = e.rang + ' sur ' + e.total;
  $('prog').style.width = (e.duree ? e.pos/e.duree*100 : 0) + '%';
  $('ecoule').textContent = minSec(e.pos);
  $('reste').textContent = '-' + minSec(e.duree - e.pos);
  if (pochetteDe !== e.i){
    pochetteDe = e.i;
    $('pochette').src = '/api/pochette?i=' + e.i;
  }
  if (courant !== e.i){ courant = e.i; dessinerBiblio(); }
}

async function etat(){
  try { afficherEtat(await (await fetch('/api/etat')).json()); } catch(e){}
}
async function cmd(c){
  try { afficherEtat(await (await fetch('/api/cmd?c=' + encodeURIComponent(c), {method:'POST'})).json()); } catch(e){}
}
async function jouer(i){
  try { afficherEtat(await (await fetch('/api/jouer?i=' + i, {method:'POST'})).json()); } catch(e){}
}
$('pochette').onerror = function(){ this.removeAttribute('src'); };

async function chargerBiblio(){
  try { biblio = await (await fetch('/api/biblio')).json(); } catch(e){ return; }
  biblio.sort((a,b) => a.a.localeCompare(b.a) || a.al.localeCompare(b.al) || (a.n - b.n) || a.t.localeCompare(b.t));
  dessinerBiblio();
}

function dessinerBiblio(){
  const q = $('recherche').value.trim().toLowerCase();
  const zone = $('biblio');
  zone.textContent = '';
  let artiste = null, album = null;
  for (const m of biblio){
    if (q && !(m.t + ' ' + m.a + ' ' + m.al).toLowerCase().includes(q)) continue;
    if (m.a !== artiste){
      artiste = m.a; album = null;
      const d = document.createElement('div'); d.className = 'artiste'; d.textContent = m.a; zone.append(d);
    }
    if (m.al !== album){
      album = m.al;
      const d = document.createElement('div'); d.className = 'album'; d.textContent = m.al; zone.append(d);
    }
    const d = document.createElement('div');
    d.className = 'morceau' + (m.i === courant ? ' actif' : '');
    const n = document.createElement('span'); n.textContent = m.n || '';
    d.append(n, document.createTextNode(m.t));
    d.onclick = () => jouer(m.i);
    zone.append(d);
  }
  if (!zone.children.length) zone.textContent = biblio.length ? 'Aucun résultat' : 'Chargement…';
}
$('recherche').oninput = dessinerBiblio;

// --- Envoi de fichiers, un par un
const depot = $('depot'), choix = $('fichiers');
depot.onclick = () => choix.click();
choix.onchange = () => { envoyer([...choix.files]); choix.value = ''; };
depot.ondragover = e => { e.preventDefault(); depot.classList.add('survol'); };
depot.ondragleave = () => depot.classList.remove('survol');
depot.ondrop = e => { e.preventDefault(); depot.classList.remove('survol'); envoyer([...e.dataTransfer.files]); };

let fileEnvoi = Promise.resolve();
function envoyer(fichiers){
  for (const f of fichiers){
    const ligne = document.createElement('div'); ligne.className = 'envoi';
    const nom = document.createElement('b'); nom.textContent = f.name;
    const st = document.createElement('i'); st.textContent = 'en attente';
    ligne.append(nom, st); $('envois').append(ligne);
    if (!f.name.toLowerCase().endsWith('.mp3')){ st.textContent = 'pas un .mp3'; st.className = 'err'; continue; }
    fileEnvoi = fileEnvoi.then(() => new Promise(fin => {
      const x = new XMLHttpRequest();
      x.open('POST', '/api/envoi');
      x.upload.onprogress = e => { if (e.lengthComputable) st.textContent = Math.round(e.loaded/e.total*100) + ' %'; };
      x.onload = () => {
        const ok = x.status === 200;
        st.textContent = ok ? 'ajouté' : x.responseText; st.className = ok ? 'ok' : 'err';
        if (ok && !$('zoneBiblio').hidden) chargerBiblio();
        fin();
      };
      x.onerror = () => { st.textContent = 'échec réseau'; st.className = 'err'; fin(); };
      const donnees = new FormData(); donnees.append('f', f, f.name);
      x.send(donnees);
    }));
  }
}

etat().then(() => { if (!$('zoneBiblio').hidden){ chargerBiblio(); minuteur = setInterval(etat, 1000); } });
</script>
</body>
</html>
)rawliteral";
