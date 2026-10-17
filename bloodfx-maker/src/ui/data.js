/* BloodFX Maker catalogue: effects, presets, colours and the small icons the UI draws. */
(function (root) {
  'use strict';

  // Effects that make sense on a hit. kind picks the preview particles and the icon.
  const EFFECTS = [
    { id: 'ent_dst_inflate_ball_clr', label: 'Confetti balloon pop', kind: 'confetti', group: 'packs' },
    { id: 'ent_dst_inflate_ball', label: 'Balloon pop', kind: 'balloon', group: 'packs' },
    { id: 'ent_dst_gen_gobstop', label: 'Gumballs', kind: 'gumballs', group: 'packs' },
    { id: 'ent_brk_banknotes', label: 'Cash', kind: 'cash', group: 'packs' },
    { id: 'bul_water_heli', label: 'Water splash', kind: 'water', group: 'packs' },
    { id: 'liquid_splash_oil', label: 'Oil splash', kind: 'oil', group: 'packs' },
    { id: 'blood_chopper', label: 'Heavy gore', kind: 'gore', group: 'packs' },
    { id: 'td_blood_hatchet', label: 'Hatchet blood', kind: 'blood', group: 'packs' },
    { id: 'scr_xmas_firework_burst_fizzle', label: 'Firework fizzle', kind: 'firework', group: 'packs', note: 'Only in packs that add it' },
    { id: 'ent_brk_coins', label: 'Coins', kind: 'coins', group: 'gta' },
    { id: 'ent_sht_feathers', label: 'Feathers', kind: 'feathers', group: 'gta' },
    { id: 'ent_dst_office_paper', label: 'Paper', kind: 'paper', group: 'gta' },
    { id: 'glass_smash', label: 'Glass smash', kind: 'glass', group: 'gta' },
    { id: 'ent_dst_electrical', label: 'Electric sparks', kind: 'sparks', group: 'gta' },
    { id: 'liquid_splash_paint', label: 'Paint splash', kind: 'paint', group: 'gta' },
    { id: 'liquid_splash_gloopy', label: 'Gloopy splash', kind: 'gloop', group: 'gta' },
    { id: 'bul_foam', label: 'Foam', kind: 'foam', group: 'gta' },
    { id: 'bang_mud', label: 'Mud', kind: 'mud', group: 'gta' },
    { id: 'ent_dst_snow_tombs', label: 'Snow', kind: 'snow', group: 'gta' },
    { id: 'ent_brk_champagne_case', label: 'Champagne', kind: 'champagne', group: 'gta' },
    { id: 'ent_dst_inflate_ring', label: 'Swim ring pop', kind: 'balloon', group: 'gta' },
    { id: 'muz_alternate_star', label: 'Star flash', kind: 'star', group: 'gta' },
    { id: 'exp_water', label: 'Water burst', kind: 'water', group: 'gta', note: 'Big' },
    { id: 'blood_entry', label: 'Normal blood', kind: 'blood', group: 'blood' },
    { id: 'blood_headshot', label: 'Headshot blood', kind: 'blood', group: 'blood' },
    { id: 'blood_entry_head_sniper', label: 'Sniper headshot', kind: 'gore', group: 'blood' },
    { id: 'blood_entry_shotgun', label: 'Shotgun blood', kind: 'blood', group: 'blood' },
    { id: 'blood_exit', label: 'Exit wound', kind: 'blood', group: 'blood' },
    { id: 'blood_mist', label: 'Blood mist', kind: 'mist', group: 'blood' },
    { id: 'liquid_splash_blood', label: 'Blood splash', kind: 'blood', group: 'blood' },
    { id: 'ent_brk_blood', label: 'Blood burst', kind: 'blood', group: 'blood' },
    { id: 'blood_stab', label: 'Stab', kind: 'blood', group: 'blood' },
    { id: 'blood_animal_maul', label: 'Animal maul', kind: 'gore', group: 'blood' },
    { id: '-', label: 'Nothing', kind: 'none', group: 'none' },
  ];
  const GROUPS = { packs: 'Seen in your packs', gta: 'More from GTA', blood: 'Blood', none: 'No effect' };
  const EFFECT_BY_ID = new Map(EFFECTS.map((e) => [e.id, e]));
  // Names that turn up in bloodfx.dat but aren't offered in the picker.
  const MORE_LABELS = {
    blood_melee_punch: 'Punch blood', blood_melee_blunt: 'Blunt hit blood', blood_throat: 'Throat cut', blood_entry_sniper: 'Sniper blood',
    blood_armour: 'Armour spark', blood_armour_heavy: 'Heavy armour spark', blood_stab_uw: 'Underwater blood', blood_stungun: 'Taser',
    blood_mouth: 'Mouth blood', blood_nose: 'Nose blood',
  };
  const effectLabel = (id) => (EFFECT_BY_ID.get(id) || {}).label || MORE_LABELS[id] || id;
  const effectKind = (id) => {
    const e = EFFECT_BY_ID.get(id);
    if (e) return e.kind;
    if (/blood|gore/.test(id)) return 'blood';
    if (/water/.test(id)) return 'water';
    return 'puff';
  };

  const ZONES = [
    { id: 'head', label: 'Head', sub: 'Head and neck' },
    { id: 'body', label: 'Body', sub: 'Torso, arms and legs' },
    { id: 'animals', label: 'Animals', sub: 'Dogs, deer and the rest' },
  ];
  const WEAPONS = [
    { id: 'guns', label: 'Guns', sub: 'Pistols, SMGs, rifles, MGs' },
    { id: 'shotguns', label: 'Shotguns', sub: 'Every shotgun' },
    { id: 'snipers', label: 'Snipers', sub: 'Sniper rifles' },
    { id: 'melee', label: 'Melee', sub: 'Fists, bats, knives' },
    { id: 'explosions', label: 'Explosions', sub: 'Rockets, grenades, blasts' },
  ];
  const GUNS = ['guns', 'shotguns', 'snipers'];

  const STOCK = { glow: 15, zoom: 1, far: 200, speed: 1, stay: 0.1, life: 0.07 };
  const SWATCHES = ['#ff2bd6', '#39e6ff', '#ff2a3d', '#7dff6a', '#ffd23f', '#8a6bff', '#ffffff', '#ff9a4d'];

  const TRACER_PRESETS = [
    { id: 'slow-beam', name: 'Slow beam', sub: 'Cyan neon at half speed, hangs for a second',
      set: { style: 'neon', colour: '#39e6ff', core: '#ffffff', width: 5, smoke: 'off', glow: 22, zoom: 1.6, far: 500, speed: 0.5, stay: 1, life: 0.09 } },
    { id: 'laser', name: 'Laser', sub: 'Thin red neon with extra glow',
      set: { style: 'neon', colour: '#ff2a3d', core: '#ffffff', width: 3, smoke: 'off', glow: 30, zoom: 1.1, far: 500, speed: 1, stay: 0.1, life: 0.07 } },
    { id: 'pink-smoke', name: 'Pink smoke', sub: 'Pink line with a wavy smoke trail',
      set: { style: 'solid', colour: '#f0a8ff', width: 3, smoke: 'wavy', smokeColour: '#e7c3f7', smokeWidth: 4, glow: 15, zoom: 1, far: 200, speed: 1, stay: 0.4, life: 0.07 } },
    { id: 'gold', name: 'Gold', sub: 'Warm gold neon, a little slower',
      set: { style: 'neon', colour: '#ffc21f', core: '#fff6d6', width: 4, smoke: 'off', glow: 24, zoom: 1.3, far: 500, speed: 0.8, stay: 0.5, life: 0.08 } },
    { id: 'slow-confetti', name: 'Slow confetti', sub: 'White line at a fifth of the speed',
      set: { style: 'solid', colour: '#ffffff', width: 3, smoke: 'off', glow: 15, zoom: 1, far: 200, speed: 0.2, stay: 0.7, life: 0.07 } },
    { id: 'thick-bar', name: 'Thick bar', sub: 'Wide solid red',
      set: { style: 'solid', colour: '#ff1e1e', width: 8, smoke: 'off', glow: 18, zoom: 1.3, far: 500, speed: 1, stay: 0.1, life: 0.07 } },
    { id: 'twin-rails', name: 'Twin rails', sub: 'Two green lines and a thin trail',
      set: { style: 'twin', colour: '#7dff6a', width: 9, smoke: 'line', smokeColour: '#7dff6a', smokeWidth: 1, glow: 20, zoom: 1.2, far: 400, speed: 0.7, stay: 0.6, life: 0.08 } },
    { id: 'ghost', name: 'Ghost', sub: 'Faint white trail that hangs around',
      set: { style: 'solid', colour: '#e8f1ff', width: 2, smoke: 'line', smokeColour: '#ffffff', smokeWidth: 1, glow: 8, zoom: 0.8, far: 300, speed: 0.35, stay: 2, life: 0.1 } },
  ];

  const headHits = (fx, size) => [{ zones: ['head'], weapons: GUNS, fx, armour: true, size }];
  const BLOOD_PRESETS = [
    { id: 'confetti', name: 'Confetti heads', fx: 'ent_dst_inflate_ball_clr', cells: headHits('ent_dst_inflate_ball_clr', 0.5) },
    { id: 'balloons', name: 'Balloons', fx: 'ent_dst_inflate_ball', cells: headHits('ent_dst_inflate_ball', 0.5) },
    { id: 'gumballs', name: 'Gumballs', fx: 'ent_dst_gen_gobstop', cells: headHits('ent_dst_gen_gobstop', 0.5) },
    { id: 'cash', name: 'Cash', fx: 'ent_brk_banknotes', cells: headHits('ent_brk_banknotes', 0.5) },
    { id: 'water', name: 'Water', fx: 'bul_water_heli', cells: headHits('bul_water_heli', 0.5) },
    { id: 'oil', name: 'Oil everywhere', fx: 'liquid_splash_oil', cells: [{ zones: ['head', 'body'], weapons: GUNS, fx: 'liquid_splash_oil', armour: true, size: 0.5 }] },
    { id: 'hatchet', name: 'Hatchet gore', fx: 'td_blood_hatchet', cells: headHits('td_blood_hatchet', 0.5) },
    { id: 'gore', name: 'Heavy gore', fx: 'blood_chopper', cells: [{ zones: ['head', 'body'], weapons: GUNS, fx: 'blood_chopper', armour: false, size: 0.8 }] },
    { id: 'fireworks', name: 'Fireworks', fx: 'scr_xmas_firework_burst_fizzle', cells: headHits('scr_xmas_firework_burst_fizzle', 0.5) },
    { id: 'coins', name: 'Coin heads', fx: 'ent_brk_coins', cells: headHits('ent_brk_coins', 0.5) },
    { id: 'none', name: 'No particles', fx: '-', cells: [{ zones: ['head', 'body', 'animals'], weapons: GUNS, fx: '-', armour: true, size: null }] },
    { id: 'normal', name: 'Normal blood', fx: 'blood_entry', cells: [] },
  ];

  // ---------------------------------------------------------------- icons (static markup, no user text)

  const svg = (body, vb) => `<svg viewBox="${vb || '0 0 24 24'}" aria-hidden="true" focusable="false">${body}</svg>`;
  const FX_ICONS = {
    confetti: '<rect x="4" y="5" width="5" height="3" rx="1" fill="#ff3d8b" transform="rotate(-20 6 6)"/><rect x="13" y="4" width="5" height="3" rx="1" fill="#ffd23f" transform="rotate(25 15 5)"/><rect x="6" y="14" width="5" height="3" rx="1" fill="#3ddcff" transform="rotate(15 8 15)"/><rect x="14" y="13" width="5" height="3" rx="1" fill="#7dff6a" transform="rotate(-30 16 14)"/><rect x="10" y="9" width="4" height="3" rx="1" fill="#b26bff"/>',
    balloon: '<ellipse cx="12" cy="10" rx="6" ry="7" fill="#e8344a"/><path d="M12 17v4" stroke="#c9d2e3" stroke-width="1.2"/><path d="M9 7c.8-1.5 2-2 3-2" stroke="#fff" stroke-opacity=".5" stroke-width="1.4" fill="none" stroke-linecap="round"/>',
    gumballs: '<circle cx="8" cy="9" r="4" fill="#ff3d8b"/><circle cx="16" cy="9" r="4" fill="#ffd23f"/><circle cx="12" cy="16" r="4" fill="#3ddcff"/>',
    cash: '<rect x="3" y="7" width="18" height="10" rx="1.5" fill="#3ea45a"/><circle cx="12" cy="12" r="2.6" fill="#a8e3a0"/><rect x="5" y="9" width="2" height="6" fill="#2b7a40"/><rect x="17" y="9" width="2" height="6" fill="#2b7a40"/>',
    coins: '<ellipse cx="10" cy="15" rx="6" ry="3" fill="#c99a1e"/><ellipse cx="10" cy="13" rx="6" ry="3" fill="#f5c542"/><ellipse cx="15" cy="9" rx="5" ry="2.6" fill="#f5c542" stroke="#c99a1e"/>',
    water: '<path d="M12 3c3 4.5 6 7.6 6 11a6 6 0 0 1-12 0c0-3.4 3-6.5 6-11z" fill="#5cc8ff"/><path d="M9.5 14a2.5 2.5 0 0 0 2.5 2.5" stroke="#e6f7ff" stroke-width="1.3" fill="none" stroke-linecap="round"/>',
    oil: '<path d="M12 3c3 4.5 6 7.6 6 11a6 6 0 0 1-12 0c0-3.4 3-6.5 6-11z" fill="#1c1a17" stroke="#7a7263" stroke-width="1.2"/><path d="M9.5 14a2.5 2.5 0 0 0 2.5 2.5" stroke="#8a8170" stroke-width="1.2" fill="none"/>',
    paint: '<path d="M12 3c3 4.5 6 7.6 6 11a6 6 0 0 1-12 0c0-3.4 3-6.5 6-11z" fill="#ff3355"/><path d="M12 9c1.8 2.4 3.5 4.2 3.5 6a3.5 3.5 0 0 1-7 0c0-1.8 1.7-3.6 3.5-6z" fill="#33aaff"/><circle cx="12" cy="15" r="1.6" fill="#ffdd33"/>',
    gloop: '<path d="M5 14c0-5 3-9 7-9s7 4 7 9c0 3-2 5-4 5-1.5 0-1.5-2-3-2s-1.5 2-3 2c-2.2 0-4-2-4-5z" fill="#8bd450"/><circle cx="10" cy="10" r="1.4" fill="#d9f5bf"/>',
    gore: '<path d="M8 4c2.4 3.6 4.8 6 4.8 8.8a4.8 4.8 0 0 1-9.6 0C3.2 10 5.6 7.6 8 4z" fill="#8a0d14"/><path d="M16.5 9c1.8 2.7 3.5 4.5 3.5 6.6a3.5 3.5 0 0 1-7 0c0-2.1 1.7-3.9 3.5-6.6z" fill="#c8102e"/><circle cx="12" cy="20" r="1.4" fill="#c8102e"/>',
    blood: '<path d="M12 3c3 4.5 6 7.6 6 11a6 6 0 0 1-12 0c0-3.4 3-6.5 6-11z" fill="#b30f1f"/><path d="M9.5 14a2.5 2.5 0 0 0 2.5 2.5" stroke="#ff8a95" stroke-width="1.3" fill="none" stroke-linecap="round"/>',
    mist: '<circle cx="12" cy="12" r="8" fill="#b30f1f" fill-opacity=".25"/><circle cx="12" cy="12" r="5" fill="#b30f1f" fill-opacity=".45"/><circle cx="12" cy="12" r="2.4" fill="#d01a2c"/>',
    feathers: '<path d="M6 19C9 9 14 5 19 4c-1 5-5 10-13 15z" fill="#f2f2f2"/><path d="M6 19l9-10" stroke="#9aa4b5" stroke-width="1"/>',
    paper: '<rect x="6" y="3" width="12" height="16" rx="1" fill="#f4f4ef" transform="rotate(-8 12 11)"/><path d="M9 8h6M9 11h6M9 14h4" stroke="#9aa4b5" stroke-width="1" transform="rotate(-8 12 11)"/>',
    glass: '<path d="M4 6l6-2 2 7-6 3z" fill="#bfe9ff" fill-opacity=".85"/><path d="M14 4l6 4-4 5-4-3z" fill="#9fdcff" fill-opacity=".85"/><path d="M8 16l6-2 3 6-7 1z" fill="#d9f3ff" fill-opacity=".85"/>',
    sparks: '<g stroke="#fff3b0" stroke-width="1.8" stroke-linecap="round"><path d="M12 12L5 5M12 12l7-7M12 12l-8 3M12 12l8 4M12 12l-1 9M12 12l2-9"/></g><circle cx="12" cy="12" r="2" fill="#fff"/>',
    foam: '<circle cx="8" cy="13" r="4" fill="#eef3fa"/><circle cx="14" cy="10" r="5" fill="#ffffff"/><circle cx="16" cy="16" r="3" fill="#dfe7f2"/>',
    mud: '<path d="M12 4c3 4.5 6 7.6 6 11a6 6 0 0 1-12 0c0-3.4 3-6.5 6-11z" fill="#6b4a2b"/><circle cx="10" cy="15" r="1.5" fill="#8d6a45"/>',
    snow: '<g stroke="#ffffff" stroke-width="1.8" stroke-linecap="round"><path d="M12 3v18M4.2 7.5l15.6 9M4.2 16.5l15.6-9"/></g>',
    champagne: '<circle cx="8" cy="15" r="2.4" fill="#f6e7a1"/><circle cx="14" cy="9" r="3" fill="#f6e7a1"/><circle cx="16" cy="17" r="1.8" fill="#ffffff"/><circle cx="10" cy="6" r="1.6" fill="#ffffff"/>',
    star: '<path d="M12 2l2.2 7.8L22 12l-7.8 2.2L12 22l-2.2-7.8L2 12l7.8-2.2z" fill="#fff6c2"/>',
    firework: '<g stroke-width="1.8" stroke-linecap="round"><path d="M12 12L4 4" stroke="#ffcf4a"/><path d="M12 12l8-8" stroke="#ff5ea8"/><path d="M12 12l-9 4" stroke="#7ee8ff"/><path d="M12 12l9 4" stroke="#ffcf4a"/><path d="M12 12v9" stroke="#ff5ea8"/><path d="M12 12V2" stroke="#7ee8ff"/></g>',
    none: '<circle cx="12" cy="12" r="8" fill="none" stroke="#6f7a8e" stroke-width="1.8"/><path d="M6.5 17.5l11-11" stroke="#6f7a8e" stroke-width="1.8"/>',
    puff: '<circle cx="12" cy="12" r="7" fill="#c8cdd7" fill-opacity=".6"/><circle cx="12" cy="12" r="3.5" fill="#e6eaf2"/>',
  };
  const fxIcon = (kind) => svg(FX_ICONS[kind] || FX_ICONS.puff);

  const LINE = (d) => svg(`<path d="${d}" fill="none" stroke="currentColor" stroke-width="1.6" stroke-linecap="round" stroke-linejoin="round"/>`);
  const ICONS = {
    guns: LINE('M3 9h14l2-2h2v4h-6l-1 2h-3l-1 5H6l1-5H3z'),
    shotguns: LINE('M2 9h17M2 12h17M19 8v5M10 12l-1.5 6h-3L7 12'),
    snipers: LINE('M12 3v4M12 17v4M3 12h4M17 12h4M12 12m-5 0a5 5 0 1 0 10 0a5 5 0 1 0-10 0'),
    melee: LINE('M4 20l7-7M10 14l2 2M13 11L19 5l1 3-6 6'),
    explosions: LINE('M12 2l1.8 5.2L19 5l-2.2 5L22 12l-5.2 1.8L19 19l-5.2-2.2L12 22l-1.8-5.2L5 19l2.2-5.2L2 12l5.2-1.8L5 5l5.2 2.2z'),
    head: LINE('M12 4a5 5 0 1 0 0 10a5 5 0 1 0 0-10M8 20c0-2.5 1.8-4 4-4s4 1.5 4 4'),
    body: LINE('M12 3a2.5 2.5 0 1 0 0 5a2.5 2.5 0 1 0 0-5M8 10h8l-1 6h-6zM9 16l-1 5M15 16l1 5'),
    animals: LINE('M7 9a1.8 1.8 0 1 0 0 .1M11 6a1.8 1.8 0 1 0 0 .1M15 6a1.8 1.8 0 1 0 0 .1M19 9a1.8 1.8 0 1 0 0 .1M8 17c0-3 2-5 4-5s4 2 4 5c0 1.5-1 2.5-2.5 2.5-1 0-1-.8-1.5-.8s-.5.8-1.5.8C9 19.5 8 18.5 8 17z'),
    open: LINE('M3 7h6l2 2h10v10H3zM3 7V5h6'),
    save: LINE('M12 4v11M7 10l5 5 5-5M5 19h14'),
    install: LINE('M12 3v10M8 9l4 4 4-4M4 15v4h16v-4'),
    undo: LINE('M9 7L4 12l5 5M5 12h9a5 5 0 0 1 0 10h-2'),
    redo: LINE('M15 7l5 5-5 5M19 12h-9a5 5 0 0 0 0 10h2'),
    fire: LINE('M12 3c1 3 4 4.5 4 9a4 4 0 0 1-8 0c0-2 1-3 2-4 0 2 1 3 2 3 0-3-1-5 0-8z'),
    pause: LINE('M9 5v14M15 5v14'),
    play: svg('<path d="M8 5l11 7-11 7z" fill="currentColor"/>'),
    slow: LINE('M12 7v5l3 2M12 3a9 9 0 1 0 0 18a9 9 0 1 0 0-18'),
    folder: LINE('M3 7h6l2 2h10v10H3z'),
    check: LINE('M5 12l4 4 10-10'),
    warn: LINE('M12 4l9 16H3zM12 10v4M12 17v.5'),
    x: LINE('M6 6l12 12M18 6L6 18'),
    reset: LINE('M4 12a8 8 0 1 0 2.3-5.6M4 4v4h4'),
    restore: LINE('M4 12a8 8 0 1 0 2.3-5.6M4 4v4h4M12 8v4l3 2'),
    search: LINE('M10.5 4a6.5 6.5 0 1 0 0 13a6.5 6.5 0 1 0 0-13M15.5 15.5L20 20'),
    copy: LINE('M8 8h11v11H8zM5 16V5h11'),
    chevron: LINE('M9 6l6 6-6 6'),
  };
  const icon = (name) => ICONS[name] || '';

  const LOGO = svg('<rect x="1.5" y="1.5" width="29" height="29" rx="8" fill="#151a24" stroke="#2e384a"/>' +
    '<path d="M6 23L24 8" stroke="#ff9a4d" stroke-width="3.4" stroke-linecap="round"/><path d="M6 23L24 8" stroke="#fff3e6" stroke-width="1.2" stroke-linecap="round"/>' +
    '<path d="M22.5 16.5c1.6 2.3 3.2 3.9 3.2 5.7a3.2 3.2 0 0 1-6.4 0c0-1.8 1.6-3.4 3.2-5.7z" fill="#e0213a"/>', '0 0 32 32');

  root.BfxData = {
    EFFECTS, GROUPS, EFFECT_BY_ID, effectLabel, effectKind, ZONES, WEAPONS, GUNS, STOCK, SWATCHES,
    TRACER_PRESETS, BLOOD_PRESETS, fxIcon, icon, LOGO,
    BASE_DAT_B64: '@BASE_DAT@',
    CORE_FX: '@EFFECTS@',
  };
})(typeof self !== 'undefined' ? self : this);
