// PalTrainerUltra — Application web
// Projection bounds-derived Palworld 1.0 (palworld-save-pal)

const MAP_SIZE = 8192;
const TRANSLATION_X = 123930.0;
const TRANSLATION_Y = 157935.0;
const SCALE = 459.0;

const MAP_AREAS = {
    Tree: {
        texture: 'assets/map/palworld-treemap.webp',
        min: { x: 347351.5, y: -818197.0 },
        max: { x: 689148.5, y: -476400.0 },
    },
    MainMap: {
        texture: 'assets/map/palworld-map.webp',
        min: { x: -1099400.0, y: -724400.0 },
        max: { x: 349400.0, y: 724400.0 },
    },
};

let currentArea = 'MainMap';
let currentMapLayer = null;
let autoFollowPlayer = true;

function cmPerPx(area) {
    const b = MAP_AREAS[area];
    return (b.max.x - b.min.x) / MAP_SIZE;
}

function mapOf(worldX, worldY) {
    for (const area of ['Tree', 'MainMap']) {
        const b = MAP_AREAS[area];
        if (worldX >= b.min.x && worldX <= b.max.x && worldY >= b.min.y && worldY <= b.max.y)
            return area;
    }
    return null;
}

function worldToUv(worldX, worldY, area) {
    const b = MAP_AREAS[area];
    const cm = cmPerPx(area);
    const px = (worldY - b.min.y) / cm;
    const py = (worldX - b.min.x) / cm;
    return {
        u: Math.min(1, Math.max(0, px / MAP_SIZE)),
        v: Math.min(1, Math.max(0, 1 - py / MAP_SIZE)),
    };
}

function worldToLeaflet(worldX, worldY) {
    const area = mapOf(worldX, worldY) || currentArea;
    const uv = worldToUv(worldX, worldY, area);
    return L.latLng(MAP_SIZE * (1 - uv.v), MAP_SIZE * uv.u);
}

function worldToMap(worldX, worldY) {
    const mapX = Math.round((worldY - TRANSLATION_Y) / SCALE);
    const mapY = Math.round((worldX + TRANSLATION_X) / SCALE) * -1;
    return { x: mapX, y: mapY };
}
function mapToWorld(mapX, mapY) {
    const worldX = mapY * -1 * SCALE - TRANSLATION_X;
    const worldY = mapX * SCALE + TRANSLATION_Y;
    return { x: worldX, y: worldY };
}

const iconAsset = {
    fastTravelPoint: 'assets/map/T_icon_compass_FTtower.webp',
    towerTravelPoint: 'assets/map/T_icon_compass_tower.webp',
    dungeon: 'assets/map/T_icon_compass_dungeon.webp',
    egg: 'assets/map/T_icon_compass_egg.webp',
    treasure: 'assets/map/T_icon_compass_treasure.webp',
    strongEnemy: 'assets/map/T_icon_enemy_strong.webp',
    alpha_pal: 'assets/map/T_icon_enemy_strong.webp',
    predator_pal: 'assets/map/T_icon_enemy_strong.webp',
    bounty: 'assets/map/T_icon_compass_Bounty.webp',
    enemyCamp: 'assets/map/T_icon_compass_EnemyCamp.webp',
    oilrig: 'assets/map/T_icon_compass_Oilrig.webp',
    base: 'assets/map/T_icon_compass_camp.webp'
};

const typeLabelsFr = {
    fastTravelPoint: 'Voyage rapide',
    towerTravelPoint: 'Tour',
    dungeon: 'Donjon',
    egg: 'Œuf',
    treasure: 'Trésor',
    strongEnemy: 'Ennemi fort',
    alpha_pal: 'Alpha Pal',
    predator_pal: 'Pal prédateur',
    bounty: 'Prime',
    enemyCamp: 'Camp ennemi',
    oilrig: 'Plateforme pétrolière',
    base: 'Base'
};

const map = L.map('map', {
    crs: L.CRS.Simple,
    minZoom: -4,
    maxZoom: 6,
    zoom: -1,
    center: [MAP_SIZE / 2, MAP_SIZE / 2]
});

function setMapArea(area) {
    currentArea = area;
    if (currentMapLayer) map.removeLayer(currentMapLayer);
    currentMapLayer = L.imageOverlay(MAP_AREAS[area].texture, [[0, 0], [MAP_SIZE, MAP_SIZE]]).addTo(map);
    // Re-add poiLayer and player on top
    poiLayer.addTo(map);
    player.addTo(map);
    // Update POI visibility for this area
    refreshPoiVisibility();
    // Update switcher buttons
    document.querySelectorAll('.area-btn').forEach(btn => {
        btn.classList.toggle('active', btn.dataset.area === area);
    });
}

// Area switcher
const areaSwitcher = L.control({ position: 'topleft' });
areaSwitcher.onAdd = function() {
    const div = L.DomUtil.create('div', 'area-switcher');
    div.innerHTML = '<button class="area-btn" data-area="MainMap">Palpagos</button>' +
                     '<button class="area-btn" data-area="Tree">Arbre Monde</button>';
    div.style.backgroundColor = 'rgba(20,20,20,0.8)';
    div.style.padding = '4px';
    div.style.borderRadius = '4px';
    L.DomEvent.disableClickPropagation(div);
    div.querySelectorAll('.area-btn').forEach(btn => {
        L.DomEvent.on(btn, 'click', () => setMapArea(btn.dataset.area));
    });
    return div;
};
areaSwitcher.addTo(map);

// Follow player toggle
const followControl = L.control({ position: 'topleft' });
followControl.onAdd = function() {
    const div = L.DomUtil.create('div', 'follow-control');
    div.innerHTML = '<button id="btn-follow" class="follow-btn active">Suivre le joueur</button>';
    div.style.backgroundColor = 'rgba(20,20,20,0.8)';
    div.style.padding = '4px';
    div.style.borderRadius = '4px';
    div.style.marginTop = '4px';
    L.DomEvent.disableClickPropagation(div);
    const btn = div.querySelector('#btn-follow');
    L.DomEvent.on(btn, 'click', () => {
        autoFollowPlayer = !autoFollowPlayer;
        btn.classList.toggle('active', autoFollowPlayer);
        if (autoFollowPlayer && lastStatus && lastStatus.ready && lastStatus.player) {
            const latlng = worldToLeaflet(lastStatus.player.x, lastStatus.player.y);
            map.setView(latlng, map.getZoom());
        }
    });
    return div;
};
followControl.addTo(map);

const playerMarker = L.divIcon({ className: 'player-marker', iconSize: [16, 16], iconAnchor: [8, 8] });
let player = L.marker([MAP_SIZE/2, MAP_SIZE/2], { icon: playerMarker, zIndexOffset: 1000 }).addTo(map);

const poiLayer = L.layerGroup().addTo(map);
let allPois = [];

setMapArea('MainMap');

function makePoiIcon(type) {
    const url = iconAsset[type] || iconAsset.fastTravelPoint;
    const alt = typeLabelsFr[type] || type;
    return L.divIcon({
        className: 'poi-icon',
        html: `<img src="${url}" alt="${alt}">`,
        iconSize: [26, 26],
        iconAnchor: [13, 13]
    });
}

function refreshPoiVisibility() {
    poiLayer.eachLayer(m => {
        if (!m._poiWorldX) return;
        const poiArea = mapOf(m._poiWorldX, m._poiWorldY) || 'MainMap';
        const areaMatch = poiArea === currentArea;
        const filterOk = document.getElementById('show-' + (m.poiType === 'fastTravelPoint' ? 'fast' :
            m.poiType === 'towerTravelPoint' ? 'tower' :
            m.poiType === 'strongEnemy' ? 'enemy' :
            m.poiType === 'alpha_pal' ? 'alpha' :
            m.poiType === 'predator_pal' ? 'predator' :
            m.poiType === 'enemyCamp' ? 'camp' :
            m.poiType))?.checked !== false;
        m.setOpacity(areaMatch && filterOk ? 1 : 0);
    });
}

async function loadPois() {
    try {
        const res = await fetch('mapObjects.json');
        const pois = await res.json();
        allPois = pois;
        for (const p of pois) {
            if (!p.location) continue;
            const latlng = worldToLeaflet(p.location.X, p.location.Y);
            const marker = L.marker(latlng, { icon: makePoiIcon(p.type) });
            const typeName = typeLabelsFr[p.type] || p.type;
            marker.bindPopup(`<b>${p.label || p.id}</b><br>${typeName}`);
            marker.poiType = p.type;
            marker._poiWorldX = p.location.X;
            marker._poiWorldY = p.location.Y;
            poiLayer.addLayer(marker);
        }
        refreshPoiVisibility();
    } catch (e) {
        console.error('Échec du chargement des points d\'intérêt', e);
    }
}

function applyFilters() {
    refreshPoiVisibility();
}

['show-fast','show-tower','show-dungeon','show-egg','show-treasure','show-enemy','show-alpha','show-predator','show-bounty','show-camp','show-oilrig']
    .forEach(id => document.getElementById(id).addEventListener('change', applyFilters));

loadPois();

const ui = {
    ready: document.getElementById('ready'),
    posX: document.getElementById('pos-x'),
    posY: document.getElementById('pos-y'),
    posZ: document.getElementById('pos-z'),
    lvl: document.getElementById('lvl'),
    hp: document.getElementById('hp'),
    sp: document.getElementById('sp'),
    weight: document.getElementById('weight'),
    speed: document.getElementById('speed'),
    godMode: document.getElementById('godMode'),
    infiniteHP: document.getElementById('infiniteHP'),
    infiniteSP: document.getElementById('infiniteSP'),
    infiniteWeight: document.getElementById('infiniteWeight'),
    superSpeed: document.getElementById('superSpeed'),
    superJump: document.getElementById('superJump'),
    flyMode: document.getElementById('flyMode'),
    noClip: document.getElementById('noClip'),
    speedValue: document.getElementById('speedValue'),
    jumpValue: document.getElementById('jumpValue'),
    weightValue: document.getElementById('weightValue'),
    teleportDistance: document.getElementById('teleportDistance')
};

let lastStatus = null;

// ---------------------------------------------------------------------------
// Contrôles avancés Wand Pro (générés dans le panneau)
// ---------------------------------------------------------------------------
const advancedControls = {};

const ADVANCED_SECTIONS = [
  { title: 'Santé & endurance du joueur', items: [
    { key: 'unlimitedHealth', type: 'toggle', label: 'Santé illimitée' },
    { key: 'refillHealth', type: 'toggle', label: 'Régénérer la santé' },
    { key: 'unlimitedStamina', type: 'toggle', label: 'Endurance illimitée' },
    { key: 'unlimitedSatiety', type: 'toggle', label: 'Faim illimitée' },
    { key: 'refillSatiety', type: 'toggle', label: 'Régénérer la faim' },
    { key: 'unlimitedSanity', type: 'toggle', label: 'Santé mentale illimitée' },
    { key: 'temperatureAlwaysNormal', type: 'toggle', label: 'Température corporelle normale' },
    { key: 'healthRegenRate', type: 'number', label: 'Taux de régénération de santé', step: 0.1, default: -1 },
    { key: 'satietyDecreaseRate', type: 'number', label: 'Taux de perte de faim', step: 0.1, default: -1 }
  ]},
  { title: 'Inventaire & équipement', items: [
    { key: 'noItemWeight', type: 'toggle', label: 'Pas de poids d\'objet' },
    { key: 'noReload', type: 'toggle', label: 'Pas de rechargement' },
    { key: 'infiniteDurability', type: 'toggle', label: 'Durabilité infinie' },
    { key: 'instantCrafting', type: 'toggle', label: 'Artisanat instantané' }
  ]},
  { title: 'Capture', items: [
    { key: 'instantCapture', type: 'toggle', label: 'Capture instantanée' },
    { key: 'captureChanceAlways', type: 'toggle', label: 'Capture toujours réussie' },
    { key: 'everyoneCapturable', type: 'toggle', label: 'Tout le monde capturable' },
    { key: 'allPalsRare', type: 'toggle', label: 'Tous les Pals rares' },
    { key: 'palRandomizer', type: 'toggle', label: 'Randomiseur de Pals' },
    { key: 'captureMultiplier', type: 'number', label: 'Multiplicateur de capture', step: 0.1, default: 1 },
    { key: 'rarePalMultiplier', type: 'number', label: 'Multiplicateur de Pals rares', step: 0.1, default: 1 }
  ]},
  { title: 'Artisanat & construction', items: [
    { key: 'noCraftingRequirements', type: 'toggle', label: 'Artisanat sans requis' },
    { key: 'noBuildingRequirements', type: 'toggle', label: 'Construction sans requis' },
    { key: 'ignoreBuildingOverlap', type: 'toggle', label: 'Ignorer le chevauchement de construction' },
    { key: 'massiveWorkSpeedPlayer', type: 'toggle', label: 'Vitesse de travail massive (joueur)' },
    { key: 'massiveWorkSpeedAll', type: 'toggle', label: 'Vitesse de travail massive (tous)' },
    { key: 'workSpeedRate', type: 'number', label: 'Taux de vitesse de travail', step: 0.1, default: 10 }
  ]},
  { title: 'Monde & temps', items: [
    { key: 'stopTime', type: 'toggle', label: 'Arrêter le temps' },
    { key: 'noCrimeReporting', type: 'toggle', label: 'Pas de signalement de crimes' },
    { key: 'instantFishing', type: 'toggle', label: 'Pêche instantanée' },
    { key: 'unlimitedMoney', type: 'toggle', label: 'Argent illimité' },
    { key: 'setHour', type: 'number', label: 'Régler l\'heure', step: 1, default: -1 },
    { key: 'advanceHours', type: 'number', label: 'Avancer les heures', step: 1, default: 0 },
    { key: 'daySpeedRate', type: 'number', label: 'Vitesse du jour', step: 0.1, default: 1 },
    { key: 'nightSpeedRate', type: 'number', label: 'Vitesse de la nuit', step: 0.1, default: 1 },
    { key: 'fishSpeedPercent', type: 'number', label: '% vitesse de pêche', step: 0.1, default: 1 },
    { key: 'xpMultiplier', type: 'number', label: 'Multiplicateur d\'XP', step: 0.1, default: 1 },
    { key: 'lootDropMultiplier', type: 'number', label: 'Multiplicateur de butin', step: 0.1, default: 1 }
  ]},
  { title: 'Pals', items: [
    { key: 'palUnlimitedHealth', type: 'toggle', label: 'Santé illimitée des Pals' },
    { key: 'palUnlimitedStamina', type: 'toggle', label: 'Endurance illimitée des Pals' },
    { key: 'palUnlimitedSatiety', type: 'toggle', label: 'Faim illimitée des Pals' },
    { key: 'palUnlimitedSanity', type: 'toggle', label: 'Santé mentale illimitée des Pals' },
    { key: 'palMaxStats', type: 'toggle', label: 'Stats max des Pals' },
    { key: 'superDamage', type: 'toggle', label: 'Dégâts super' },
    { key: 'maxWorkerSanity', type: 'toggle', label: 'Santé mentale max des ouvriers' },
    { key: 'unlimitedBaseHP', type: 'toggle', label: 'PV illimités de la base' },
    { key: 'damageMultiplier', type: 'number', label: 'Multiplicateur de dégâts', step: 1, default: 10000 },
    { key: 'palLevelRandomMin', type: 'number', label: 'Niveau aléatoire min des Pals', step: 1, default: -1 },
    { key: 'palLevelRandomMax', type: 'number', label: 'Niveau aléatoire max des Pals', step: 1, default: -1 }
  ]},
  { title: 'Stats du personnage', items: [
    { key: 'setLevel', type: 'number', label: 'Définir le niveau', step: 1, default: -1 },
    { key: 'setXP', type: 'number', label: 'Définir l\'XP', step: 1, default: -1 },
    { key: 'setRank', type: 'number', label: 'Définir le rang', step: 1, default: -1 },
    { key: 'statPoints', type: 'number', label: 'Points de stats', step: 1, default: -1 },
    { key: 'techPoints', type: 'number', label: 'Points de technologie', step: 1, default: -1 },
    { key: 'ancientTechPoints', type: 'number', label: 'Points de techno ancienne', step: 1, default: -1 }
  ]},
  { title: 'Déplacement', items: [
    { key: 'walkSpeedMultiplier', type: 'number', label: 'Mult. vitesse marche', step: 0.1, default: 1 },
    { key: 'sprintSpeedMultiplier', type: 'number', label: 'Mult. vitesse sprint', step: 0.1, default: 1 },
    { key: 'jumpHeightMultiplier', type: 'number', label: 'Mult. hauteur saut', step: 0.1, default: 1 }
  ]},
  { title: 'Nouveaux cheats (Palworld 1.0)', items: [
    { key: 'infiniteShield', type: 'toggle', label: 'Bouclier infini' },
    { key: 'stealthMode', type: 'toggle', label: 'Mode furtif' },
    { key: 'dropRateAlways', type: 'toggle', label: '100% taux de butin' },
    { key: 'foodWontSpoil', type: 'toggle', label: 'Nourriture non périssable' },
    { key: 'infiniteExp', type: 'toggle', label: 'XP infinie' },
    { key: 'oneHitKill', type: 'toggle', label: 'Tueur d\'un coup' },
    { key: 'palInstantSkillCooldown', type: 'toggle', label: 'Compétences Pal sans cooldown' },
    { key: 'unlimitedBaseStats', type: 'toggle', label: 'Stats de base illimitées' },
    { key: 'unlockWorldTree', type: 'toggle', label: 'Débloquer l\'Arbre-Monde' },
    { key: 'unlockAwakening', type: 'toggle', label: 'Débloquer le donjon d\'Éveil' },
    { key: 'unlockAllTowerBosses', type: 'toggle', label: 'Débloquer tous les boss de tours' }
  ]}
];

const ADVANCED_ACTIONS = [
  { key: 'setAllItemCounts', label: 'Définir tous les comptes d\'objets', default: 999 },
  { key: 'setLifmunkEffigyCount', label: 'Définir les effigies Lifmunk', default: 99 }
];

function renderAdvancedUI() {
  const container = document.getElementById('advanced-cheats');
  for (const section of ADVANCED_SECTIONS) {
    const sec = document.createElement('details');
    sec.className = 'advanced-section';
    sec.open = false;
    sec.innerHTML = `<summary>${section.title}</summary>`;
    for (const item of section.items) {
      const label = document.createElement('label');
      label.className = 'row';
      if (item.type === 'toggle') {
        label.innerHTML = `<input type="checkbox" id="adv-${item.key}"> ${item.label}`;
      } else {
        label.innerHTML = `${item.label} <input type="number" id="adv-${item.key}" step="${item.step}" value="${item.default !== undefined ? item.default : ''}">`;
      }
      const el = label.querySelector('input');
      advancedControls[item.key] = el;
      el.addEventListener('change', () => { el._dirty = true; sendCommands(); });
      if (item.type === 'number') {
        el.addEventListener('input', () => { el._dirty = true; });
      }
      sec.appendChild(label);
    }
    container.appendChild(sec);
  }

  const actContainer = document.getElementById('advanced-actions');
  for (const item of ADVANCED_ACTIONS) {
    const wrap = document.createElement('div');
    wrap.className = 'row';
    wrap.innerHTML = `<label>${item.label} <input type="number" id="act-${item.key}" value="${item.default}" step="1"></label>`;
    const input = wrap.querySelector('input');
    const btn = document.createElement('button');
    btn.textContent = 'Appliquer';
    btn.style.marginLeft = '6px';
    btn.addEventListener('click', () => sendCommands({ [item.key]: parseInt(input.value, 10) }));
    wrap.appendChild(btn);
    actContainer.appendChild(wrap);
  }
}

async function pollStatus() {
    try {
        const res = await fetch('paltrainer.json?' + Date.now());
        if (!res.ok) return;
        const data = await res.json();
        lastStatus = data;

        if (data.ready && data.player) {
            const p = data.player;
            // Auto-switch to player's area
            const playerArea = mapOf(p.x, p.y);
            if (playerArea && playerArea !== currentArea) {
                setMapArea(playerArea);
            }
            const latlng = worldToLeaflet(p.x, p.y);
            player.setLatLng(latlng);
            if (autoFollowPlayer) {
                map.setView(latlng, map.getZoom(), { animate: true, duration: 0.25 });
            }
            ui.posX.textContent = p.x.toFixed(1);
            ui.posY.textContent = p.y.toFixed(1);
            ui.posZ.textContent = p.z.toFixed(1);
            ui.lvl.textContent = p.level;
            ui.hp.textContent = `${p.hp}/${p.maxHp}`;
            ui.sp.textContent = `${p.sp}/${p.maxSp}`;
            ui.weight.textContent = `${p.weight.toFixed(1)}/${p.maxWeight.toFixed(1)}`;
            ui.speed.textContent = p.speed.toFixed(0);
            ui.ready.textContent = 'Connectée';
            ui.ready.classList.add('online');

            if (data.cheats) syncUIFromCheats(data.cheats);
        } else {
            ui.ready.textContent = 'En attente du joueur…';
            ui.ready.classList.remove('online');
        }
    } catch (e) {
        ui.ready.textContent = 'En attente du joueur…';
        ui.ready.classList.remove('online');
    }
}

function syncUIFromCheats(c) {
    if (ui.godMode.checked !== c.godMode && !ui.godMode._dirty) ui.godMode.checked = c.godMode;
    if (ui.infiniteHP.checked !== c.infiniteHP && !ui.infiniteHP._dirty) ui.infiniteHP.checked = c.infiniteHP;
    if (ui.infiniteSP.checked !== c.infiniteSP && !ui.infiniteSP._dirty) ui.infiniteSP.checked = c.infiniteSP;
    if (ui.infiniteWeight.checked !== c.infiniteWeight && !ui.infiniteWeight._dirty) ui.infiniteWeight.checked = c.infiniteWeight;
    if (ui.superSpeed.checked !== c.superSpeed && !ui.superSpeed._dirty) ui.superSpeed.checked = c.superSpeed;
    if (ui.superJump.checked !== c.superJump && !ui.superJump._dirty) ui.superJump.checked = c.superJump;
    if (ui.flyMode.checked !== c.flyMode && !ui.flyMode._dirty) ui.flyMode.checked = c.flyMode;
    if (ui.noClip.checked !== c.noClip && !ui.noClip._dirty) ui.noClip.checked = c.noClip;
    if (document.activeElement !== ui.speedValue && c.speedValue !== undefined) ui.speedValue.value = c.speedValue;
    if (document.activeElement !== ui.jumpValue && c.jumpValue !== undefined) ui.jumpValue.value = c.jumpValue;
    if (document.activeElement !== ui.weightValue && c.weightValue !== undefined) ui.weightValue.value = c.weightValue;

    for (const [key, el] of Object.entries(advancedControls)) {
        if (c[key] === undefined) continue;
        if (el.type === 'checkbox') {
            if (el.checked !== c[key] && !el._dirty) el.checked = c[key];
        } else if (el.type === 'number') {
            if (document.activeElement !== el) el.value = c[key];
        }
    }
}

function buildCommands(oneShot = {}) {
    const cmd = {
        godMode: ui.godMode.checked,
        infiniteHP: ui.infiniteHP.checked,
        infiniteSP: ui.infiniteSP.checked,
        infiniteWeight: ui.infiniteWeight.checked,
        superSpeed: ui.superSpeed.checked,
        superJump: ui.superJump.checked,
        flyMode: ui.flyMode.checked,
        noClip: ui.noClip.checked,
        speedValue: parseFloat(ui.speedValue.value),
        jumpValue: parseFloat(ui.jumpValue.value),
        weightValue: parseFloat(ui.weightValue.value),
        teleport: oneShot.teleport || false,
        teleportDistance: parseFloat(ui.teleportDistance.value),
        unlockFastTravel: oneShot.unlockFastTravel || false,
        clearWeather: oneShot.clearWeather || false
    };

    for (const [key, el] of Object.entries(advancedControls)) {
        if (el.type === 'checkbox') {
            cmd[key] = el.checked;
        } else if (el.type === 'number') {
            cmd[key] = parseFloat(el.value);
        }
    }
    return Object.assign(cmd, oneShot);
}

async function sendCommands(oneShot = {}) {
    const payload = buildCommands(oneShot);
    try {
        await fetch('commands', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify(payload)
        });
    } catch (e) {
        console.error('Échec de l\'envoi des commandes', e);
    }
}

['godMode','infiniteHP','infiniteSP','infiniteWeight','superSpeed','superJump','flyMode','noClip']
    .forEach(id => {
        const el = document.getElementById(id);
        el.addEventListener('change', () => { el._dirty = true; sendCommands(); });
    });

[ui.speedValue, ui.jumpValue, ui.weightValue, ui.teleportDistance].forEach(el => {
    el.addEventListener('input', () => { el._dirty = true; });
    el.addEventListener('change', sendCommands);
});

document.getElementById('btn-apply').addEventListener('click', () => {
    [ui.speedValue, ui.jumpValue, ui.weightValue, ui.teleportDistance].forEach(el => el._dirty = false);
    sendCommands();
});

document.getElementById('btn-teleport').addEventListener('click', () => sendCommands({ teleport: true }));
document.getElementById('btn-weather').addEventListener('click', () => sendCommands({ clearWeather: true }));
document.getElementById('btn-fasttravel').addEventListener('click', () => sendCommands({ unlockFastTravel: true }));

renderAdvancedUI();

setInterval(pollStatus, 250);
pollStatus();
