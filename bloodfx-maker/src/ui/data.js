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

  // Tracer presets. `from` names the reference pack a preset is modelled on.
  const T = (o) => ({ glow: 15, zoom: 1, far: 200, speed: 1, stay: 0.1, life: 0.07, ...o });
  const TRACER_PRESETS = [
    { id: 'pack-recolour', group: 'Your pack', name: 'Recolour pack smoke', sub: "Keeps your pack's smoke, new colour", set: { style: 'keep', smoke: 'recolour', smokeColour: '#ffc21f' }, motion: false },
    { id: 'pack-recolour-beam', group: 'Your pack', name: 'Recolour pack beam', sub: "Keeps your pack's beam, new colour", set: { style: 'recolour', colour: '#39e6ff', smoke: 'keep' }, motion: false },
    { id: 'slow-beam', group: 'Like your packs', name: 'Slow beam', sub: 'Cyan line at half speed, hangs for a second', from: 'Slow Solarsystem',
      set: T({ style: 'solid', colour: '#39e6ff', width: 3, smoke: 'off', glow: 20, zoom: 1.6, far: 500, speed: 0.5, stay: 1, life: 0.09 }) },
    { id: 'pink-smoke', group: 'Like your packs', name: 'Pink smoke', sub: 'No beam, wavy pink smoke', from: 'pink tracers',
      set: T({ style: 'none', smoke: 'wavy', smokeColour: '#e7c3f7', smokeWidth: 4, stay: 0.4 }) },
    { id: 'slow-confetti', group: 'Like your packs', name: 'Slow confetti', sub: 'White line at a fifth of the speed', from: 'Slow Confetti',
      set: T({ style: 'solid', colour: '#ffffff', width: 3, smoke: 'off', speed: 0.2, stay: 0.7 }) },
    { id: 'red-line', group: 'Like your packs', name: 'Red line', sub: 'Thin red line', from: 'blood splat',
      set: T({ style: 'solid', colour: '#ff0029', width: 3, smoke: 'off' }) },
    { id: 'light-blue', group: 'Like your packs', name: 'Light blue', sub: 'Thin cyan line', from: '55$ light blue',
      set: T({ style: 'solid', colour: '#00ffff', width: 3, smoke: 'off' }) },
    { id: 'thick-bar', group: 'Like your packs', name: 'Thick bar', sub: 'Wide solid red', from: 'w2mv x Rem',
      set: T({ style: 'solid', colour: '#ff1e1e', width: 8, smoke: 'off', glow: 18, zoom: 1.3, far: 500 }) },
    { id: 'green-line', group: 'Like your packs', name: 'Green line', sub: 'Thin lime line', from: 'uglyscrim',
      set: T({ style: 'solid', colour: '#73eb31', width: 3, smoke: 'off', stay: 0.4, speed: 0.4 }) },
    { id: 'smoke-trail', group: 'Smoke', name: 'Smoke trail', sub: 'No beam, white smoke line', set: T({ style: 'none', smoke: 'line', smokeColour: '#ffffff', smokeWidth: 4, stay: 0.6 }) },
    { id: 'gold-smoke', group: 'Smoke', name: 'Gold smoke', sub: 'No beam, gold smoke line', set: T({ style: 'none', smoke: 'line', smokeColour: '#ffc21f', smokeWidth: 4, stay: 0.6 }) },
    { id: 'toxic-smoke', group: 'Smoke', name: 'Toxic smoke', sub: 'No beam, wavy green smoke', set: T({ style: 'none', smoke: 'wavy', smokeColour: '#7dff6a', smokeWidth: 4, stay: 0.6 }) },
    { id: 'blood-smoke', group: 'Smoke', name: 'Blood smoke', sub: 'No beam, dark red smoke line', set: T({ style: 'none', smoke: 'line', smokeColour: '#c8102e', smokeWidth: 5, stay: 0.8 }) },
    { id: 'ice-smoke', group: 'Smoke', name: 'Ice smoke', sub: 'No beam, pale blue wavy smoke', set: T({ style: 'none', smoke: 'wavy', smokeColour: '#bfe9ff', smokeWidth: 3, stay: 0.8, speed: 0.6 }) },
    { id: 'laser', group: 'Beams', name: 'Laser', sub: 'Thin red neon with extra glow', set: T({ style: 'neon', colour: '#ff2a3d', core: '#ffffff', width: 3, smoke: 'off', glow: 30, zoom: 1.1, far: 500 }) },
    { id: 'plasma', group: 'Beams', name: 'Plasma', sub: 'Thick purple neon, slow', set: T({ style: 'neon', colour: '#9b5cff', core: '#f1e8ff', width: 6, smoke: 'off', glow: 28, zoom: 1.4, far: 600, speed: 0.6, stay: 0.4, life: 0.09 }) },
    { id: 'gold', group: 'Beams', name: 'Gold', sub: 'Warm gold neon, a little slower', set: T({ style: 'neon', colour: '#ffc21f', core: '#fff6d6', width: 4, smoke: 'off', glow: 24, zoom: 1.3, far: 500, speed: 0.8, stay: 0.5, life: 0.08 }) },
    { id: 'ice', group: 'Beams', name: 'Ice', sub: 'Pale blue line that lingers', set: T({ style: 'solid', colour: '#bfe9ff', width: 3, smoke: 'line', smokeColour: '#e6f6ff', smokeWidth: 2, glow: 18, speed: 0.7, stay: 1.2, life: 0.08 }) },
    { id: 'hellfire', group: 'Beams', name: 'Hellfire', sub: 'Orange neon with a red wavy trail', set: T({ style: 'neon', colour: '#ff6a00', core: '#ffe2c2', width: 4, smoke: 'wavy', smokeColour: '#ff2a1a', smokeWidth: 3, glow: 26, zoom: 1.2, far: 500, stay: 0.5 }) },
    { id: 'twin-rails', group: 'Beams', name: 'Twin rails', sub: 'Two green lines and a thin trail', set: T({ style: 'twin', colour: '#7dff6a', width: 9, smoke: 'line', smokeColour: '#7dff6a', smokeWidth: 1, glow: 20, zoom: 1.2, far: 400, speed: 0.7, stay: 0.6, life: 0.08 }) },
    { id: 'sniper-streak', group: 'Beams', name: 'Sniper streak', sub: 'Long white streak, seen from far away', set: T({ style: 'solid', colour: '#ffffff', width: 2, smoke: 'off', glow: 18, zoom: 1, far: 800, speed: 1, stay: 0.3, life: 0.2 }) },
    { id: 'ghost', group: 'Beams', name: 'Ghost', sub: 'Faint white trail that hangs around', set: T({ style: 'solid', colour: '#e8f1ff', width: 2, smoke: 'line', smokeColour: '#ffffff', smokeWidth: 1, glow: 8, zoom: 0.8, far: 300, speed: 0.35, stay: 2, life: 0.1 }) },
    { id: 'stock', group: 'Beams', name: 'Stock look', sub: "GTA's normal timing and glow", set: T({ style: 'keep', smoke: 'keep' }) },
    { id: 'nebula', group: 'Colour along the tracer', name: 'Nebula', sub: 'Pink fading to cyan', set: T({ style: 'neon', colour: '#ff3dd6', colour2: '#38d9ff', core: '#ffffff', width: 4, smoke: 'off', glow: 22, zoom: 1.2, far: 500 }) },
    { id: 'cotton-candy', group: 'Colour along the tracer', name: 'Cotton candy', sub: 'Baby pink to baby blue', set: T({ style: 'solid', colour: '#ffb3e6', colour2: '#a8d8ff', width: 4, smoke: 'off', glow: 18 }) },
    { id: 'lava-water', group: 'Colour along the tracer', name: 'Lava and water', sub: 'Orange fading to blue', set: T({ style: 'solid', colour: '#ff5a00', colour2: '#1e8bff', width: 4, smoke: 'off', glow: 22, zoom: 1.2 }) },
    { id: 'sunset', group: 'Colour along the tracer', name: 'Sunset', sub: 'Gold to purple', set: T({ style: 'neon', colour: '#ffb21f', colour2: '#8a2be2', core: '#fff3d6', width: 4, smoke: 'off', glow: 22, zoom: 1.2 }) },
    { id: 'aurora', group: 'Colour along the tracer', name: 'Aurora', sub: 'Green to violet with a trail', set: T({ style: 'solid', colour: '#3dff9a', colour2: '#9b5cff', width: 3, smoke: 'line', smokeColour: '#3dff9a', smokeColour2: '#9b5cff', smokeWidth: 2, glow: 20, stay: 0.6, speed: 0.7 }) },
    { id: 'blood-moon', group: 'Colour along the tracer', name: 'Blood moon', sub: 'Red to black-red, slow', set: T({ style: 'solid', colour: '#ff2a3d', colour2: '#4a0010', width: 5, smoke: 'off', glow: 16, zoom: 1.3, speed: 0.6, stay: 0.6 }) },
    { id: 'vaporwave', group: 'Colour along the tracer', name: 'Vaporwave', sub: 'Hot pink to teal, wavy trail', set: T({ style: 'neon', colour: '#ff71ce', colour2: '#01cdfe', core: '#ffffff', width: 4, smoke: 'wavy', smokeColour: '#b967ff', smokeWidth: 3, glow: 24, stay: 0.5 }) },
  ];

  // Blood presets: each cell list says which hits play which effect.
  const hit = (zones, fx, size, weapons, armour) => ({ zones, weapons: weapons || GUNS, fx, armour: armour !== false, size: size === undefined ? 0.5 : size });
  const BLOOD_PRESETS = [
    { id: 'normal', group: 'Basics', name: 'Normal blood', sub: 'Everything back to blood', fx: 'blood_entry', cells: [] },
    { id: 'none', group: 'Basics', name: 'No particles', sub: 'Hits play no effect', fx: '-', cells: [hit(['head', 'body', 'animals'], '-', null)] },
    { id: 'confetti', group: 'Headshots', name: 'Confetti heads', sub: 'Confetti balloon pops', fx: 'ent_dst_inflate_ball_clr', cells: [hit(['head'], 'ent_dst_inflate_ball_clr')] },
    { id: 'balloons', group: 'Headshots', name: 'Balloons', sub: 'Plain balloon pops', fx: 'ent_dst_inflate_ball', cells: [hit(['head'], 'ent_dst_inflate_ball')] },
    { id: 'gumballs', group: 'Headshots', name: 'Gumballs', sub: 'Gumballs spill out', fx: 'ent_dst_gen_gobstop', cells: [hit(['head'], 'ent_dst_gen_gobstop')] },
    { id: 'cash', group: 'Headshots', name: 'Cash', sub: 'Banknotes fly', fx: 'ent_brk_banknotes', cells: [hit(['head'], 'ent_brk_banknotes')] },
    { id: 'coins', group: 'Headshots', name: 'Coins', sub: 'A burst of coins', fx: 'ent_brk_coins', cells: [hit(['head'], 'ent_brk_coins')] },
    { id: 'water', group: 'Headshots', name: 'Water', sub: 'Water splash', fx: 'bul_water_heli', cells: [hit(['head'], 'bul_water_heli')] },
    { id: 'fireworks', group: 'Headshots', name: 'Fireworks', sub: 'Needs a pack that adds it', fx: 'scr_xmas_firework_burst_fizzle', cells: [hit(['head'], 'scr_xmas_firework_burst_fizzle')] },
    { id: 'stars', group: 'Headshots', name: 'Star flash', sub: 'A bright star', fx: 'muz_alternate_star', cells: [hit(['head'], 'muz_alternate_star')] },
    { id: 'glass', group: 'Headshots', name: 'Glass', sub: 'Shattering glass', fx: 'glass_smash', cells: [hit(['head'], 'glass_smash')] },
    { id: 'hatchet', group: 'Gore', name: 'Hatchet gore', sub: 'Hatchet blood on heads', fx: 'td_blood_hatchet', cells: [hit(['head'], 'td_blood_hatchet')] },
    { id: 'gore', group: 'Gore', name: 'Heavy gore', sub: 'Rotor-blade blood everywhere', fx: 'blood_chopper', cells: [hit(['head', 'body'], 'blood_chopper', 0.8, GUNS, false)] },
    { id: 'sniper-gore', group: 'Gore', name: 'Sniper gore', sub: 'Sniper headshot blood from any gun', fx: 'blood_entry_head_sniper', cells: [hit(['head'], 'blood_entry_head_sniper', 1, GUNS, false)], splatter: 1.5 },
    { id: 'mist', group: 'Gore', name: 'Blood mist', sub: 'A red mist on every hit', fx: 'blood_mist', cells: [hit(['head', 'body'], 'blood_mist', 0.8, GUNS, false)] },
    { id: 'oil', group: 'Whole body', name: 'Oil everywhere', sub: 'Black oil splashes', fx: 'liquid_splash_oil', cells: [hit(['head', 'body'], 'liquid_splash_oil')] },
    { id: 'paint', group: 'Whole body', name: 'Paintball', sub: 'Paint splashes', fx: 'liquid_splash_paint', cells: [hit(['head', 'body'], 'liquid_splash_paint')] },
    { id: 'slime', group: 'Whole body', name: 'Slime', sub: 'Green gloop', fx: 'liquid_splash_gloopy', cells: [hit(['head', 'body'], 'liquid_splash_gloopy')], splat: '#5cff3a' },
    { id: 'robot', group: 'Themes', name: 'Robot', sub: 'Sparks on heads, oil on bodies', fx: 'ent_dst_electrical', cells: [hit(['head'], 'ent_dst_electrical'), hit(['body'], 'liquid_splash_oil')] },
    { id: 'party', group: 'Themes', name: 'Party', sub: 'Confetti heads, balloon bodies', fx: 'ent_dst_inflate_ball_clr', cells: [hit(['head'], 'ent_dst_inflate_ball_clr'), hit(['body'], 'ent_dst_inflate_ball')] },
    { id: 'money', group: 'Themes', name: 'Money', sub: 'Cash heads, coin bodies', fx: 'ent_brk_banknotes', cells: [hit(['head'], 'ent_brk_banknotes'), hit(['body'], 'ent_brk_coins')] },
    { id: 'winter', group: 'Themes', name: 'Winter', sub: 'Snow heads, feather bodies', fx: 'ent_dst_snow_tombs', cells: [hit(['head'], 'ent_dst_snow_tombs'), hit(['body'], 'ent_sht_feathers')] },
    { id: 'office', group: 'Themes', name: 'Office', sub: 'Paper everywhere', fx: 'ent_dst_office_paper', cells: [hit(['head', 'body'], 'ent_dst_office_paper')] },
    { id: 'bubbly', group: 'Themes', name: 'Bubbly', sub: 'Champagne heads, foam bodies', fx: 'ent_brk_champagne_case', cells: [hit(['head'], 'ent_brk_champagne_case'), hit(['body'], 'bul_foam')] },
  ];

  // Effect browser categories, matched by name. The first match wins.
  const CATEGORIES = [
    ['fun', 'Fun', /inflate|gobstop|banknote|coins|feather|confetti|firework|champagne|office_paper|star/],
    ['blood', 'Blood', /blood|gore|maul|shark|throat|stab/],
    ['tracer', 'Tracers', /tracer/],
    ['muzzle', 'Muzzle flash', /^muz_/],
    ['impact', 'Bullet impacts', /^bul_|^bang_|^mel_|^scrape_/],
    ['explosion', 'Explosions', /^exp_|explo/],
    ['fire', 'Fire', /fire|flame|molotov|burn|torch|candle/],
    ['smoke', 'Smoke and steam', /smoke|steam|fog|mist|vapour|exhaust|cig/],
    ['water', 'Water', /water|splash|wtr_|drip|rain|wave|bubble|liquid|sprinkler/],
    ['sparks', 'Sparks and electric', /spark|elec|weld/],
    ['dust', 'Dust and dirt', /dust|dirt|sand|mud|debris/],
    ['glass', 'Glass and breakables', /glass|brk_|dst_|shard|bottle/],
    ['vehicle', 'Vehicles', /^veh_|wheel_|^water_boat|^water_jet|^water_amph|plane|heli/],
    ['weather', 'Weather and sky', /^env_|snow|lens_|wind|leaves|petal|blossom/],
  ];
  const categoryOf = (id) => {
    for (const [key, , re] of CATEGORIES) if (re.test(id)) return key;
    return 'other';
  };
  const CATEGORY_LABEL = Object.fromEntries([...CATEGORIES.map(([k, l]) => [k, l]), ['other', 'Everything else']]);
  const CATEGORY_KIND = { fun: 'confetti', blood: 'blood', tracer: 'star', muzzle: 'star', impact: 'puff', explosion: 'firework', fire: 'sparks',
    smoke: 'foam', water: 'water', sparks: 'sparks', dust: 'mud', glass: 'glass', vehicle: 'oil', weather: 'snow', other: 'puff' };

  const ACCENTS = [
    { id: 'blue', label: 'Blue', value: '#3b82ff' },
    { id: 'amber', label: 'Tracer amber', value: '#ff9a4d' },
    { id: 'violet', label: 'Violet', value: '#9b6bff' },
    { id: 'cyan', label: 'Cyan', value: '#22c7e8' },
    { id: 'green', label: 'Green', value: '#2fd38a' },
    { id: 'pink', label: 'Pink', value: '#ff4fa8' },
    { id: 'red', label: 'Red', value: '#ff4d5e' },
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
    home: LINE('M4 11l8-7 8 7M6 10v10h12V10M10 20v-6h4v6'),
    tracer: LINE('M3 17L21 7M3 17l3-1M21 7l-3 1'),
    muzzle: LINE('M3 12h7M10 8l4 4-4 4M15 6l2-2M15 18l2 2M17 12h4'),
    explorer: LINE('M4 6h16M4 12h16M4 18h10M18 16l2 2 0 0M17 17a2 2 0 1 0 0 .1'),
    settings: LINE('M12 9a3 3 0 1 0 0 6a3 3 0 1 0 0-6M19 12l2-1-1-3-2 .5-1.5-1.5L17 5l-3-1-1 2h-2L10 4 7 5l.5 2L6 8.5 4 8l-1 3 2 1v1l-2 1 1 3 2-.5L7.5 17 7 19l3 1 1-2h2l1 2 3-1-.5-2 1.5-1.5 2 .5 1-3-2-1z'),
    image: LINE('M4 5h16v14H4zM4 15l5-5 4 4 3-3 4 4M15 9a1 1 0 1 0 0 .1'),
    grid: LINE('M4 4h7v7H4zM13 4h7v7h-7zM4 13h7v7H4zM13 13h7v7h-7z'),
    list: LINE('M8 6h12M8 12h12M8 18h12M4 6h.5M4 12h.5M4 18h.5'),
    layers: LINE('M12 4l9 5-9 5-9-5zM3 14l9 5 9-5'),
    drop: LINE('M12 3c3 4.5 6 7.6 6 11a6 6 0 0 1-12 0c0-3.4 3-6.5 6-11z'),
    info: LINE('M12 3a9 9 0 1 0 0 18a9 9 0 1 0 0-18M12 11v6M12 7.5v.5'),
    flip: LINE('M7 4v16M7 4l-3 3M7 4l3 3M17 20V4M17 20l-3-3M17 20l3-3'),
  };
  const icon = (name) => ICONS[name] || '';

  const LOGO = svg('<rect x="1.5" y="1.5" width="29" height="29" rx="8" fill="#151a24" stroke="#2e384a"/>' +
    '<path d="M6 23L24 8" stroke="#ff9a4d" stroke-width="3.4" stroke-linecap="round"/><path d="M6 23L24 8" stroke="#fff3e6" stroke-width="1.2" stroke-linecap="round"/>' +
    '<path d="M22.5 16.5c1.6 2.3 3.2 3.9 3.2 5.7a3.2 3.2 0 0 1-6.4 0c0-1.8 1.6-3.4 3.2-5.7z" fill="#e0213a"/>', '0 0 32 32');

  root.BfxData = {
    EFFECTS, GROUPS, EFFECT_BY_ID, effectLabel, effectKind, ZONES, WEAPONS, GUNS, STOCK, SWATCHES,
    TRACER_PRESETS, BLOOD_PRESETS, fxIcon, icon, LOGO, CATEGORIES, categoryOf, CATEGORY_LABEL, CATEGORY_KIND, ACCENTS,
    BASE_DAT_B64: '@BASE_DAT@',
    CORE_FX: '@EFFECTS@',
    LONG_FX: '@LONG@',
  };
})(typeof self !== 'undefined' ? self : this);
