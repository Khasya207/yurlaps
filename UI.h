#pragma once
#include <Arduino.h>

const char MAIN_page[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>YurLaps Pro</title>

<style>
* { box-sizing: border-box; }

body {
  margin: 0;
  font-family: Arial, sans-serif;
  background:
    radial-gradient(circle at top left, rgba(56,189,248,0.20), transparent 32%),
    radial-gradient(circle at top right, rgba(34,197,94,0.12), transparent 30%),
    linear-gradient(135deg, #020617, #0f172a);
  color: white;
  min-height: 100vh;
}

.header {
  height: 76px;
  padding: 0 28px;
  display: flex;
  align-items: center;
  justify-content: space-between;
  background: rgba(2, 6, 23, 0.86);
  border-bottom: 1px solid rgba(148,163,184,0.24);
}

.header-left { display: flex; align-items: center; gap: 14px; }
.header-right { display: flex; align-items: center; gap: 14px; }

.lang-select {
  width: auto;
  margin-top: 0;
  padding: 9px 14px;
  font-size: 13px;
  font-weight: 700;
  border-radius: 20px;
  background: rgba(30,41,59,0.9);
  border: 1px solid rgba(148,163,184,0.3);
  cursor: pointer;
}

/* Baris pengaturan gaya "iOS Settings" -- label di kiri, kontrol
   (select/switch) rapat di kanan, dipisah garis tipis antar baris di
   dalam grup yang sama. Dipakai buat Language & Voice, dan bisa dipakai
   ulang di tempat lain yang butuh layout serupa. */
.select-row {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 12px;
  padding: 12px 2px;
  border-bottom: 1px solid rgba(148,163,184,0.12);
}

.select-row:last-child { border-bottom: none; }

.select-row label {
  font-size: 14px;
  color: #e2e8f0;
  font-weight: 600;
}

.select-row select {
  width: auto;
  min-width: 150px;
  margin-top: 0;
  padding: 8px 12px;
  font-size: 13px;
  border-radius: 10px;
}
.brand-logo { height: 44px; width: auto; filter: drop-shadow(0 0 10px rgba(56,189,248,0.35)); }
.brand-text { display: flex; flex-direction: column; }

.logo { font-size: 30px; font-weight: 900; color: #38bdf8; line-height: 1; }
.subtitle-row { display: flex; align-items: center; gap: 8px; margin-top: 3px; }
.subtitle { font-size: 12px; color: #94a3b8; letter-spacing: 2px; }
.version-badge {
  font-size: 10px;
  font-weight: 800;
  color: #38bdf8;
  background: rgba(56,189,248,0.12);
  border: 1px solid rgba(56,189,248,0.4);
  border-radius: 20px;
  padding: 2px 9px;
  letter-spacing: 0.5px;
}
.clock { font-size: 24px; font-weight: 800; color: #e2e8f0; }

.nav {
  display: flex;
  gap: 10px;
  padding: 16px 20px 0 20px;
  flex-wrap: wrap;
}

.nav button {
  background: rgba(30,41,59,0.85);
  border: 1px solid rgba(148,163,184,0.22);
  transition: background .15s, transform .1s;
}

.nav button:active { transform: scale(0.97); }

.nav button.active {
  background: #0284c7;
  box-shadow: 0 4px 16px rgba(2,132,199,0.45);
}

.page { display: none; }
.page.active { display: block; }

.main {
  padding: 20px;
  display: grid;
  grid-template-columns: 1.75fr 0.85fr;
  gap: 20px;
}

.card {
  background: rgba(15, 23, 42, 0.78);
  border: 1px solid rgba(148,163,184,0.22);
  border-radius: 22px;
  padding: 18px;
  box-shadow: 0 18px 45px rgba(0,0,0,0.38);
}

.race-panel {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 14px;
  margin-bottom: 12px;
}

.metric {
  background: linear-gradient(180deg, rgba(30,41,59,0.95), rgba(15,23,42,0.95));
  border: 1px solid rgba(148,163,184,0.18);
  border-radius: 18px;
  padding: 16px;
}

.metric-label {
  font-size: 12px;
  color: #94a3b8;
  text-transform: uppercase;
  letter-spacing: 1.4px;
}

.metric-value {
  font-size: 34px;
  font-weight: 900;
  margin-top: 8px;
  /* v4.3: tabular-nums bikin tiap digit angka punya lebar sama persis --
     sebelumnya font default proporsional bikin lebar teks goyang tiap kali
     angka berubah (mis. "1" lebih kurus dari "0"), dan karena raceTimer
     di-update tiap ~200ms, ini kelihatan seperti "kedip2"/jitter padahal
     bukan animasi apapun -- cuma teks yang lebarnya berubah-ubah terus. */
  font-variant-numeric: tabular-nums;
  font-feature-settings: "tnum";
}

/* Badge status besar (Race/Qualify) -- warna + titik berdenyut pas RUNNING
   supaya kelihatan jelas dari jarak agak jauh, tidak cuma teks polos. */
.status-badge {
  display: inline-flex;
  align-items: center;
  gap: 9px;
  font-size: 26px;
  font-weight: 1000;
  padding: 4px 0;
}

.status-dot {
  width: 12px;
  height: 12px;
  border-radius: 50%;
  background: currentColor;
  flex-shrink: 0;
}

.status-badge.is-live .status-dot {
  animation: statusPulse 1.1s ease-in-out infinite;
}

@keyframes statusPulse {
  0%   { box-shadow: 0 0 0 0 currentColor; opacity: 1; }
  70%  { box-shadow: 0 0 0 9px transparent; opacity: 0.55; }
  100% { box-shadow: 0 0 0 0 transparent; opacity: 1; }
}

.status-running { color: #22c55e; }
.status-countdown { color: #facc15; }
.status-stopped { color: #f97316; }
.status-idle { color: #94a3b8; }

/* Kotak ringkasan pengaturan aktif (Lap Target / ID Cooldown) -- sengaja
   ditaruh nempel di bawah panel Race Status/Timer, BUKAN di kartu
   terpisah jauh di samping, supaya operator gampang cek nilai yang lagi
   aktif tanpa geser pandangan. */
.quick-settings {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 12px;
  margin-bottom: 18px;
}

.quick-settings .qs-item {
  background: rgba(2,6,23,0.55);
  border: 1px dashed rgba(56,189,248,0.35);
  border-radius: 14px;
  padding: 10px 14px;
  display: flex;
  align-items: center;
  justify-content: space-between;
}

.quick-settings .qs-label {
  font-size: 12px;
  color: #94a3b8;
  text-transform: uppercase;
  letter-spacing: 1px;
}

.quick-settings .qs-value {
  font-size: 20px;
  font-weight: 900;
  color: #38bdf8;
}

.settings-hint {
  font-size: 12px;
  color: #64748b;
  margin: -6px 0 14px 2px;
}

.countdown-overlay {
  position: fixed;
  inset: 0;
  background: rgba(2,6,23,0.82);
  display: none;
  align-items: center;
  justify-content: center;
  z-index: 99;
}

.countdown-number {
  font-size: 150px;
  font-weight: 1000;
  color: #38bdf8;
  text-shadow: 0 0 36px rgba(56,189,248,0.7);
}

.controls {
  display: grid;
  grid-template-columns: 1fr 1fr 100px 100px 100px 100px;
  gap: 12px;
  align-items: end;
  margin-bottom: 8px;
}

.driver-form {
  display: grid;
  grid-template-columns: 1fr 1fr 120px;
  gap: 12px;
  align-items: end;
  margin-bottom: 18px;
}

label { font-size: 13px; color: #94a3b8; }

input, select {
  width: 100%;
  padding: 13px;
  margin-top: 6px;
  border: 1px solid rgba(148,163,184,0.28);
  border-radius: 12px;
  background: rgba(2,6,23,0.78);
  color: white;
  font-size: 18px;
  outline: none;
}

button {
  border: none;
  padding: 14px;
  border-radius: 12px;
  font-size: 15px;
  font-weight: 800;
  cursor: pointer;
  color: white;
  transition: filter .12s, transform .08s;
}

button:hover { filter: brightness(1.12); }
button:active { transform: scale(0.96); }

.btn-save { background: #0284c7; }
.btn-settings { background: #7c3aed; }
.btn-start { background: #16a34a; }
.btn-stop { background: #ea580c; }
.btn-reset { background: #dc2626; }
.btn-delete { background: #dc2626; padding: 9px 12px; }
.btn-check { width: 22px; height: 22px; }

h2 {
  margin: 0 0 14px 0;
  font-size: 22px;
  color: #facc15;
}

table {
  width: 100%;
  border-collapse: collapse;
  overflow: hidden;
  border-radius: 16px;
  /* v4.3: dulu tabel tidak punya background sendiri (transparan penuh,
     nunjukin warna .card di belakangnya yang juga transparan ~78%) --
     jadi kelihatan "tembus pandang". Sekarang tabel punya warna solid
     sendiri yang senada dengan tema gelap, baris genap/ganjil dibedakan
     tipis di atas warna solid ini (bukan lagi transparan terhadap
     background di belakangnya). */
  background: #111827;
}

th {
  background: linear-gradient(90deg, #0369a1, #0284c7);
  color: white;
  font-size: 13px;
  padding: 14px;
  text-transform: uppercase;
}

td {
  padding: 14px;
  text-align: center;
  border-bottom: 1px solid rgba(148,163,184,0.14);
  font-size: 17px;
}

tr:nth-child(odd) {
  background: #111827;
}

tr:nth-child(even) {
  background: #1a2540;
}

tr.leaderboard-row { transition: background .3s; }

/* Efek "baru nyatat lap" -- kilat biru singkat di baris racer/qualifier
   yang barusan nambah lap, supaya kelihatan jelas siapa yang baru saja
   melintas tanpa harus scan angka satu-satu. */
@keyframes lapFlash {
  0%   { background: rgba(56,189,248,0.55); }
  100% { background: transparent; }
}
tr.lap-flash { animation: lapFlash 1.4s ease-out; }

.rank1 {
  color: #22c55e;
  font-weight: 1000;
  font-size: 23px;
}

.rank-cell {
  display: inline-flex;
  align-items: center;
  gap: 6px;
  justify-content: center;
  font-weight: 900;
}

.medal { font-size: 22px; line-height: 1; }

.finish { color: #22c55e; font-weight: 1000; }
.inrace { color: #38bdf8; font-weight: 800; }
.dnf { color: #ef4444; font-weight: 900; }
.dns { color: #64748b; font-weight: 900; }
.ready { color: #facc15; font-weight: 800; }
.bestlap-cell {
  color: #22c55e;
  font-weight: 1000;
  text-shadow: 0 0 12px rgba(34,197,94,0.55);
}

.lap-chip {
  display: inline-block;
  background: #020617;
  border: 1px solid rgba(56,189,248,0.35);
  border-radius: 10px;
  padding: 7px 11px;
  margin: 3px;
  color: #e2e8f0;
  font-size: 14px;
}

.lap-chip-best {
  border-color: rgba(34,197,94,0.8);
  color: #22c55e;
  font-weight: 900;
  box-shadow: 0 0 12px rgba(34,197,94,0.25);
}
.side {
  display: grid;
  gap: 20px;
}

.terminal {
  background: #020617;
  border-radius: 18px;
  padding: 18px;
  font-family: monospace;
  font-size: 21px;
  line-height: 1.9;
  color: #22c55e;
  border: 1px solid rgba(34,197,94,0.25);
}

.terminal-title {
  color: #38bdf8;
  font-size: 15px;
  margin-bottom: 10px;
  letter-spacing: 1.5px;
}

.terminal-scroll {
  background: #020617;
  border-radius: 18px;
  padding: 8px 16px;
  font-family: monospace;
  font-size: 14px;
  color: #22c55e;
  border: 1px solid rgba(34,197,94,0.25);
  max-height: 340px;
  overflow-y: auto;
}

.terminal-row {
  display: grid;
  grid-template-columns: 90px 90px 1fr 60px 70px;
  gap: 10px;
  padding: 7px 2px;
  border-bottom: 1px solid rgba(34,197,94,0.10);
}

.terminal-row.terminal-header {
  color: #38bdf8;
  font-size: 11px;
  text-transform: uppercase;
  letter-spacing: 1px;
  border-bottom: 1px solid rgba(56,189,248,0.3);
  position: sticky;
  top: 0;
  background: #020617;
  padding-top: 4px;
}

.terminal-empty {
  color: #64748b;
  padding: 14px 0;
  text-align: center;
  font-family: Arial, sans-serif;
}

.info-grid {
  display: grid;
  gap: 14px;
}

.info {
  background: rgba(2,6,23,0.65);
  border-radius: 16px;
  padding: 16px;
  border: 1px solid rgba(148,163,184,0.18);
}

.info-label {
  color: #94a3b8;
  font-size: 13px;
  text-transform: uppercase;
  letter-spacing: 1px;
}

.info-value {
  font-size: 28px;
  font-weight: 900;
  margin-top: 7px;
}

.driver-page { padding: 20px; }

/* Grafik posisi per lap -- dipakai di Race (live), detail hasil race
   tersimpan, dan link ke data-nya ikut di CSV export. */
.chart-wrap {
  background: rgba(2,6,23,0.55);
  border: 1px solid rgba(148,163,184,0.18);
  border-radius: 16px;
  padding: 14px;
  margin-top: 14px;
  overflow-x: auto;
}
.chart-legend { display: flex; flex-wrap: wrap; gap: 10px 16px; margin-top: 10px; font-size: 12px; color: #cbd5e1; }
.chart-legend-item { display: flex; align-items: center; gap: 6px; }
.chart-legend-dot { width: 10px; height: 10px; border-radius: 50%; flex-shrink: 0; }
.chart-empty { color: #64748b; text-align: center; padding: 20px 0; }

/* Notifikasi kecil (toast) -- dipakai antara lain buat kasih tahu kalau
   Race/Qualifying ditolak start karena satunya lagi jalan. */
.toast-wrap {
  position: fixed;
  top: 18px;
  left: 50%;
  transform: translateX(-50%);
  z-index: 200;
  display: flex;
  flex-direction: column;
  gap: 10px;
  align-items: center;
  pointer-events: none;
}

.toast {
  background: #1e293b;
  border: 1px solid rgba(248,113,113,0.55);
  color: #fecaca;
  padding: 13px 22px;
  border-radius: 14px;
  font-weight: 700;
  font-size: 15px;
  box-shadow: 0 14px 34px rgba(0,0,0,0.5);
  animation: toastIn .18s ease-out;
}

@keyframes toastIn {
  from { opacity: 0; transform: translateY(-10px); }
  to   { opacity: 1; transform: translateY(0); }
}

/* --- Announcement modular (template kalimat custom per jenis) --- */
.ann-row {
  border: 1px solid rgba(148,163,184,0.18);
  border-radius: 12px;
  padding: 10px 12px;
  margin-bottom: 10px;
  background: rgba(2,6,23,0.4);
}

.ann-row-head label {
  font-size: 14px;
  font-weight: 700;
  color: #e2e8f0;
  display: flex;
  align-items: center;
  gap: 8px;
}

.ann-template-row {
  display: flex;
  gap: 8px;
  margin-top: 8px;
  align-items: center;
}

.ann-template-row input[type=text] {
  flex: 1;
  margin-top: 0;
  padding: 9px 12px;
  font-size: 13px;
}

.ann-btn-test {
  padding: 9px 12px;
  font-size: 12px;
  white-space: nowrap;
  background: #16a34a;
}

.ann-btn-reset {
  padding: 9px 10px;
  font-size: 12px;
  white-space: nowrap;
  background: #475569;
}

.ann-hint {
  font-size: 11px;
  color: #64748b;
  margin-top: 6px;
}

.ann-hint code {
  background: rgba(56,189,248,0.14);
  color: #38bdf8;
  padding: 1px 5px;
  border-radius: 5px;
  font-size: 10.5px;
}

/* --- v4.7: redesign kartu announcement, gaya "iOS Settings" --- */
.ios-toggle {
  position: relative;
  display: inline-block;
  width: 46px;
  height: 27px;
  flex-shrink: 0;
}

.ios-toggle input {
  opacity: 0;
  width: 0;
  height: 0;
  position: absolute;
}

.ios-toggle-track {
  position: absolute;
  inset: 0;
  cursor: pointer;
  background: #3f4a5e;
  transition: background .2s ease;
  border-radius: 27px;
}

.ios-toggle-track::before {
  content: "";
  position: absolute;
  height: 23px;
  width: 23px;
  left: 2px;
  top: 2px;
  background: #f8fafc;
  transition: transform .2s ease;
  border-radius: 50%;
  box-shadow: 0 2px 5px rgba(0,0,0,0.35);
}

.ios-toggle input:checked + .ios-toggle-track { background: #22c55e; }
.ios-toggle input:checked + .ios-toggle-track::before { transform: translateX(19px); }

.ann-card {
  background: linear-gradient(180deg, rgba(30,41,59,0.6), rgba(15,23,42,0.6));
  border: 1px solid rgba(148,163,184,0.14);
  border-radius: 16px;
  padding: 14px 16px;
  margin-bottom: 12px;
  box-shadow: 0 6px 18px rgba(0,0,0,0.18);
}

.ann-card-head {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 10px;
}

.ann-card-title {
  font-size: 15px;
  font-weight: 800;
  color: #f1f5f9;
  letter-spacing: 0.2px;
}

.ann-card-row {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 10px;
  margin-top: 12px;
  padding-top: 12px;
  border-top: 1px solid rgba(148,163,184,0.12);
}

.ann-card-row-label {
  font-size: 12.5px;
  color: #94a3b8;
  font-weight: 600;
}

.ann-card-row select {
  width: auto;
  min-width: 150px;
  margin-top: 0;
  padding: 7px 10px;
  font-size: 12.5px;
  border-radius: 9px;
}

.ann-card .ann-template-row {
  margin-top: 12px;
  padding-top: 12px;
  border-top: 1px solid rgba(148,163,184,0.12);
}

.ann-card .ann-hint { margin-top: 8px; }

.ann-section-label {
  font-size: 11px;
  font-weight: 800;
  color: #64748b;
  text-transform: uppercase;
  letter-spacing: 1.4px;
  margin: 18px 0 10px 2px;
}
.ann-section-label:first-child { margin-top: 4px; }

@media (max-width: 1050px) {
  .main { grid-template-columns: 1fr; }
  .controls { grid-template-columns: 1fr 1fr 1fr; }
  .race-panel { grid-template-columns: 1fr 1fr; }
}

@media (max-width: 700px) {
  .header { height: auto; min-height: 68px; padding: 10px 16px; flex-wrap: wrap; }
  .brand-logo { height: 34px; }
  .logo { font-size: 22px; }
  .clock { font-size: 17px; }
  .main, .driver-page { padding: 12px; }
  .race-panel { grid-template-columns: 1fr; }
  .quick-settings { grid-template-columns: 1fr; }
  .controls, .driver-form { grid-template-columns: 1fr; }
  th, td { font-size: 13px; padding: 9px; }
  .countdown-number { font-size: 100px; }
  .terminal-row { grid-template-columns: 62px 60px 1fr 40px 50px; font-size: 11px; gap: 4px; }
  .terminal-scroll { max-height: 260px; padding: 6px 10px; }
}

/* --- Overlay Race (blue screen) ---
   Jendela terpisah (dibuka lewat window.open ke URL yang sama + ?overlay=1)
   khusus buat di-capture OBS/software recording lain sebagai overlay video
   race -- background biru solid (gampang di-chroma-key, sengaja BUKAN
   hijau supaya tidak sama dengan warna panah "naik posisi"), isinya cuma
   timer + leaderboard ringkas (posisi, nama, lap, panah naik/turun +
   selisih posisi, last lap, gap). Semua elemen normal (header, nav,
   halaman biasa) disembunyikan total lewat class `overlay-mode` di
   <body>. */
body.overlay-mode > .header,
body.overlay-mode > .nav,
body.overlay-mode > .page,
body.overlay-mode > .toast-wrap,
body.overlay-mode > .countdown-overlay {
  display: none !important;
}

body.overlay-mode {
  background: #0000ff !important;
}

.overlay-view { display: none; }

body.overlay-mode .overlay-view {
  display: block;
  padding: 14px;
  max-width: 380px;
  margin: 0 auto;
}

.overlay-timer {
  font-family: 'Courier New', monospace;
  font-size: 26px;
  font-weight: 1000;
  color: #f8fafc;
  text-align: center;
  letter-spacing: 1px;
  background: rgba(15,23,42,0.55);
  border-radius: 10px;
  padding: 6px 18px;
  width: fit-content;
  margin: 0 auto 12px auto;
  box-shadow: 0 3px 10px rgba(0,0,0,0.25);
}

.overlay-row {
  display: grid;
  grid-template-columns: 18px 1fr 42px 30px 50px 48px;
  align-items: center;
  gap: 6px;
  background: rgba(15,23,42,0.48);
  border-radius: 8px;
  padding: 5px 8px;
  margin-bottom: 4px;
  color: #f8fafc;
  box-shadow: 0 3px 10px rgba(0,0,0,0.25);
  will-change: transform;
}

/* Efek kilat singkat saat baris ini BARU SAJA nambah lap (lewat garis
   finish) -- lepas dari animasi pindah posisi (FLIP) yang jalan
   bersamaan lewat inline style transform, lihat renderOverlayBoard(). */
@keyframes overlayRowFlash {
  0%   { background: rgba(56,189,248,0.55); box-shadow: 0 0 0 2px rgba(56,189,248,0.9); }
  100% { background: rgba(15,23,42,0.48); box-shadow: 0 3px 10px rgba(0,0,0,0.25); }
}

.overlay-row-flash { animation: overlayRowFlash 1s ease-out; }

.overlay-pos {
  font-size: 13px;
  font-weight: 1000;
  color: #38bdf8;
  text-align: center;
}

.overlay-name {
  font-size: 12px;
  font-weight: 800;
  text-align: left;
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
}

.overlay-laps {
  font-size: 9.5px;
  font-weight: 800;
  color: #e2e8f0;
  text-align: center;
  white-space: nowrap;
}

.overlay-laps.finished { color: #22c55e; }

.overlay-arrow {
  text-align: center;
  font-size: 11px;
  font-weight: 900;
  white-space: nowrap;
}
.overlay-arrow.up { color: #22c55e; }
.overlay-arrow.down { color: #ef4444; }
.overlay-arrow.same { color: #94a3b8; }

.overlay-lastlap {
  font-size: 10px;
  font-weight: 700;
  color: #cbd5e1;
  text-align: right;
  white-space: nowrap;
}

.overlay-gap {
  font-size: 10px;
  font-weight: 800;
  color: #facc15;
  text-align: right;
  white-space: nowrap;
}

.overlay-empty {
  color: #ffffff;
  text-align: center;
  font-weight: 800;
  font-size: 15px;
  margin-top: 24px;
  text-shadow: 0 2px 8px rgba(0,0,0,0.6);
}

.overlay-replay-btn {
  display: none;
  margin: 0 auto 10px auto;
  padding: 7px 14px;
  font-size: 12px;
}

/* v4.3: tombol "Mulai Replay" -- ditampilkan dulu begitu overlay replay
   dibuka, SEBELUM timeline benar-benar jalan, supaya sempat siapkan
   software recording (OBS dkk) dulu. Timeline (+ countdown & horn) baru
   mulai begitu tombol ini diklik. */
.overlay-start-btn {
  display: block;
  margin: 30px auto;
  padding: 14px 22px;
  font-size: 15px;
  font-weight: 900;
}

/* v4.3: angka countdown & "GO" khusus di jendela overlay -- BUKAN pakai
   #countdownOverlay yang dipakai race live, karena elemen itu sengaja
   disembunyikan total (display:none!important) di body.overlay-mode
   (lihat blok CSS overlay race di atas). Dibikin lebih kecil dari versi
   live karena jendela overlay-nya sendiri kecil (max-width 380px). */
.overlay-countdown {
  display: none;
  text-align: center;
  font-size: 90px;
  font-weight: 1000;
  color: #ffffff;
  text-shadow: 0 2px 14px rgba(0,0,0,0.6);
  margin: 30px 0;
}
</style>
</head>


<body>
<script>
  // Dijalankan sesegera mungkin (sebelum header/nav sempat dirender)
  // supaya jendela overlay tidak sempat "kelip" nampilin UI normal dulu.
  if (new URLSearchParams(location.search).get('overlay') === '1') {
    document.body.classList.add('overlay-mode');
  }
</script>

<div class="countdown-overlay" id="countdownOverlay">
  <div class="countdown-number" id="countdownNumber">5</div>
</div>

<div class="toast-wrap" id="toastWrap"></div>

<!-- Overlay Race (green screen) -- lihat catatan CSS di atas. Konten
     diisi oleh renderOverlay(), cuma aktif kalau <body> punya class
     "overlay-mode" (dicek lewat ?overlay=1 di URL, lihat akhir script). -->
<div class="overlay-view" id="overlayView">
  <button class="btn-settings overlay-replay-btn" id="overlayReplayRestart" onclick="restartOverlayReplay()">🔁 Ulangi Replay</button>
  <button class="btn-start overlay-start-btn" id="overlayStartBtn" style="display:none;" onclick="beginOverlayReplayCountdown()">▶ Mulai Replay</button>
  <div class="overlay-countdown" id="overlayCountdownDisplay"></div>
  <div class="overlay-timer" id="overlayRaceTimer">00:00:000</div>
  <div id="overlayBoard"></div>
</div>

<div class="header">
  <div class="header-left">
    <svg class="brand-logo" viewBox="0 0 120 100" xmlns="http://www.w3.org/2000/svg" aria-hidden="true">
      <defs>
        <linearGradient id="spdGrad" x1="0" y1="0" x2="1" y2="1">
          <stop offset="0" stop-color="#7dd3fc"/>
          <stop offset="1" stop-color="#0284c7"/>
        </linearGradient>
      </defs>
      <!-- bendera kotak-kotak -->
      <g transform="translate(2,10) rotate(-6)">
        <path d="M0 0 C14 -6 28 4 42 -2 L42 34 C28 40 14 30 0 36 Z" fill="#0f172a" stroke="#e2e8f0" stroke-width="2"/>
        <g fill="#e2e8f0">
          <rect x="3" y="1" width="7" height="7"/>
          <rect x="17" y="1" width="7" height="7"/>
          <rect x="31" y="1" width="7" height="7"/>
          <rect x="10" y="8" width="7" height="7"/>
          <rect x="24" y="8" width="7" height="7"/>
          <rect x="3" y="15" width="7" height="7"/>
          <rect x="17" y="15" width="7" height="7"/>
          <rect x="31" y="15" width="7" height="7"/>
          <rect x="10" y="22" width="7" height="7"/>
          <rect x="24" y="22" width="7" height="7"/>
        </g>
        <line x1="0" y1="-2" x2="0" y2="46" stroke="#e2e8f0" stroke-width="3"/>
      </g>
      <!-- speedometer -->
      <g transform="translate(74,52)">
        <circle cx="0" cy="0" r="30" fill="#0f172a" stroke="url(#spdGrad)" stroke-width="4"/>
        <path d="M -21 12 A 24 24 0 1 1 21 12" fill="none" stroke="url(#spdGrad)" stroke-width="6" stroke-linecap="round"/>
        <line x1="0" y1="0" x2="14" y2="-16" stroke="#f8fafc" stroke-width="4" stroke-linecap="round"/>
        <circle cx="0" cy="0" r="4" fill="#f8fafc"/>
      </g>
    </svg>
    <div class="brand-text">
      <div class="logo">YurLaps</div>
      <div class="subtitle-row">
        <div class="subtitle">PRO RACE TIMING SYSTEM</div>
        <div class="version-badge" id="fwVersionBadge">v4.2</div>
      </div>
    </div>
  </div>
  <div class="header-right">
    <div class="clock" id="clock">--:--:--</div>
    <select id="headerUiLangSelect" class="lang-select" onchange="setUILanguage(this.value)">
      <option value="id">🇮🇩 Indonesia</option>
      <option value="en">🇬🇧 English</option>
    </select>
  </div>
</div>

<div class="nav">
  <button id="tabRace" class="active"
      onclick="showPage('race')">
      Race
  </button>

  <button id="tabQualify"
    onclick="showPage('qualify')">
    Qualify
</button>

  <button id="tabDriver"
      onclick="showPage('driver')">
      Driver
  </button>

  <button id="tabResult"
      onclick="showPage('result')">
      Race Result
  </button>

  <button id="tabSetting"
    onclick="showPage('setting')">
    Setting
  </button>

</div>

<div id="racePage" class="page active">
  <div class="main" style="grid-template-columns: 1fr;">

    <div class="card">

      <div class="race-panel">
        <div class="metric">
          <div class="metric-label">Race Status</div>
          <div class="metric-value">
            <span class="status-badge" id="raceStatusBadge">
              <span class="status-dot"></span>
              <span id="raceStatus">IDLE</span>
            </span>
          </div>
        </div>

        <div class="metric">
          <div class="metric-label">Race Timer</div>
          <div class="metric-value" id="raceTimer">00:00:000</div>
        </div>
      </div>

      <!-- Ringkasan pengaturan yang LAGI AKTIF -- sengaja nempel persis di
           bawah Race Status/Timer (bukan kartu terpisah jauh di samping)
           supaya gampang dicek sekilas dari jarak, dan langsung terlihat
           jelas beda visualnya dengan input pengaturan di bawahnya. -->
      <div class="quick-settings">
        <div class="qs-item">
          <span class="qs-label" id="lblTargetView">Lap Target Aktif</span>
          <span class="qs-value" id="targetView">-</span>
        </div>
        <div class="qs-item">
          <span class="qs-label" id="lblCooldownView">ID Cooldown Aktif</span>
          <span class="qs-value"><span id="cooldownView">-</span>s</span>
        </div>
      </div>

      <div class="controls">
        <div>
          <label id="lblLapTarget">Lap Target</label>
          <input type="number" id="lapTarget" min="1">
        </div>

        <div>
          <label id="lblCooldown">ID Cooldown / Per ID (Second)</label>
          <input type="number" id="cooldown" min="1" step="1">
        </div>

        <button class="btn-settings" onclick="saveSettings()" id="btnSaveRaceSettings">Save Settings</button>

        <button id="startBtn" class="btn-start" onclick="startRace()">Start</button>

        <button id="stopBtn" class="btn-stop" onclick="stopRace()" style="display:none;">Stop</button>

        <button id="resetBtn" class="btn-reset" onclick="resetRace()" style="display:none;">Reset</button>
      </div>
      <div class="settings-hint" id="raceSettingsHint">
        "Save Settings" cuma menyimpan Lap Target &amp; ID Cooldown di atas.
        Hasil race tersimpan otomatis begitu race di-Reset -- lihat tab
        "Race Result".
      </div>

      <div style="display:flex;align-items:center;justify-content:space-between;flex-wrap:wrap;gap:10px;">
        <h2 id="h2LeaderboardRace" style="margin:0;">Leaderboard</h2>
        <button class="btn-settings" style="padding:9px 16px;font-size:13px;" onclick="openRaceOverlay()" id="btnOpenOverlay">
          🖥️ Buka Overlay Race (Green Screen)
        </button>
      </div>

      <table>
        <thead>
          <tr>
            <th id="thRaceRank">Rank</th>
            <th id="thRaceDriver">Driver</th>
            <th id="thRaceId">ID</th>
            <th id="thRaceLap">Lap</th>
            <th id="thRaceBest">Best Lap</th>
            <th id="thRaceLast">Last Lap</th>
            <th id="thRaceAvg">Average</th>
            <th id="thRaceStatus">Status</th>
          </tr>
        </thead>
        <tbody id="leaderboardBody"></tbody>
      </table>

      <h2 id="h2PositionChartRace" style="margin-top:22px;">Grafik Posisi per Lap</h2>
      <div id="racePositionChart"></div>
    </div>

  </div>

  <div class="main" style="grid-template-columns: 1fr; margin-top:0;">
    <div class="card">
      <h2 id="h2TerminalRace">Terminal</h2>
      <div class="terminal-title">LAST 50 TRANSPONDER READS</div>
      <div class="terminal-scroll" id="raceTerminalBody">
        <div class="terminal-empty">No transponder detected yet</div>
      </div>
    </div>
  </div>
</div>
<div id="qualifyPage" class="page">
  <div class="main" style="grid-template-columns: 1fr;">

    <div class="card">
      <div class="race-panel">
        <div class="metric">
          <div class="metric-label">Qualify Status</div>
          <div class="metric-value">
            <span class="status-badge" id="qualifyStatusBadge">
              <span class="status-dot"></span>
              <span id="qualifyStatus">STOPPED</span>
            </span>
          </div>
        </div>

        <div class="metric">
          <div class="metric-label">Max Lap</div>
          <div class="metric-value" id="qualifyMaxLapView">-</div>
        </div>

        <div class="metric">
          <div class="metric-label">Sort By</div>
          <div class="metric-value" id="qualifySortView">-</div>
        </div>
      </div>

      <div class="controls">
        <div>
          <label id="lblMaxLap">Max Lap / Driver</label>
          <input type="number" id="qualifyMaxLapInput" min="1" value="10" onchange="saveQualifySettings()">
        </div>

        <div>
          <label id="lblQualifyCooldown">ID Cooldown / Second</label>
          <input type="number" id="qualifyCooldownInput" min="1" value="3" onchange="saveQualifySettings()">
        </div>

        <div>
          <label id="lblSortPosition">Sort Position</label>
          <select id="qualifySortInput"
            onchange="saveQualifySettings()">
             <option value="best">Best Lap</option>
             <option value="lap">Lap Count</option>
          </select>
        </div>

        <button class="btn-settings" onclick="saveQualifySettings()" id="btnSaveQualifySettings">Save Settings</button>
        <button id="qualifyStartBtn" class="btn-start" onclick="startQualify()">Start</button>
        <button id="qualifyStopBtn" class="btn-stop" onclick="stopQualify()" style="display:none;">Stop</button>
        <button id="qualifyResetBtn" class="btn-reset" onclick="resetQualify()" style="display:none;">Reset</button>
      </div>
      <div class="settings-hint" id="qualifySettingsHint">
        "Save Settings" cuma menyimpan Max Lap, ID Cooldown &amp; Sort
        Position di atas. Hasil qualifying tersimpan otomatis begitu
        di-Reset -- lihat tab "Race Result".
      </div>

      <h2 id="h2LeaderboardQualify">Qualifying Leaderboard</h2>

      <table>
        <thead>
          <tr>
            <th id="thQualPos">Pos</th>
            <th id="thQualDriver">Driver</th>
            <th id="thQualId">ID</th>
            <th id="thQualLap">Lap</th>
            <th id="thQualBest">Best Lap</th>
            <th id="thQualLast">Last Lap</th>
            <th id="thQualAvg">Average</th>
            <th id="thQualDelete">Delete</th>
          </tr>
        </thead>
        <tbody id="qualifyBody"></tbody>
      </table>
    </div>

  </div>

  <div class="main" style="grid-template-columns: 1fr; margin-top:0;">
    <div class="card">
      <h2 id="h2TerminalQualify">Terminal</h2>
      <div class="terminal-title">LAST 50 TRANSPONDER READS</div>
      <div class="terminal-scroll" id="qualifyTerminalBody">
        <div class="terminal-empty">No transponder detected yet</div>
      </div>
    </div>
  </div>
</div>

<div id="driverPage" class="page">
  <div class="driver-page">
    <div class="card">
      <h2 id="h2DriverReg">Driver Registration</h2>

      <div class="driver-form">
        <div>
          <label id="lblDriverName">Driver Name</label>
          <input type="text" id="driverName" placeholder="Example: Rizky">
        </div>

        <div>
          <label id="lblTransponderId">Transponder ID</label>
          <input type="number" id="driverId" placeholder="Decimal ID">
        </div>

        <button class="btn-save" onclick="saveDriver()">Save</button>
      </div>
    </div>

    <div class="card">
      <h2 id="h2LastDetectedId">Last Detected ID</h2>
      <div style="color:#94a3b8;font-size:13px;margin-bottom:12px;">
        Otomatis mengikuti transponder terakhir yang kebaca sensor (Race
        atau Qualifying) -- tidak perlu input manual.
      </div>

      <div id="driverLastIdResult">
        <div class="info">
          <div class="info-label">Menunggu transponder terdeteksi...</div>
        </div>
      </div>
    </div>

    <div class="card">
      <h2 id="h2DriverList">Driver List</h2>

      <div style="margin-bottom:16px;">
        <label id="lblSearchDriver">Search Driver</label>
        <input type="text" id="driverSearchInput" placeholder="Cari nama driver..."
          oninput="filterDriverList()">
      </div>

      <table>
        <thead>
          <tr>
            <th id="thDrvName">Name</th>
            <th id="thDrvId">Transponder ID</th>
            <th id="thDrvJoin">Join Race</th>
            <th id="thDrvDelete">Delete</th>
          </tr>
        </thead>
        <tbody id="driverBody"></tbody>
      </table>
    </div>
  </div>
</div>

<div id="resultPage" class="page">

  <div class="driver-page">

    <div class="card">

<h2 id="h2RaceResultTitle">Race Result</h2>

<div style="display:flex;gap:10px;margin-bottom:16px;">
  <button id="btnResultRace" class="btn-save" onclick="showResultTab('race')">
    Result Race
  </button>

  <button id="btnResultQualify" class="btn-start" onclick="showResultTab('qualify')">
    Result Qualify
  </button>
</div>

<div id="raceResultBox">
  <div id="historyList"></div>
</div>

<div id="qualifyResultBox" style="display:none;">
  <div id="qualifyHistoryList"></div>
</div>

<div id="raceDetailBox" style="display:none; margin-top:18px;">

  <button
    id="btnBackRaceResult"
    class="btn-stop"
    onclick="closeRaceDetail()"
    style="margin-bottom:14px;">
    Back to Result List
  </button>

  <div id="raceDetailContent"></div>

</div>

    </div>

  </div>

</div>

<div id="settingPage" class="page">
  <div class="driver-page">
    <div class="card">
      <h2 id="h2VoiceSetting">Voice Setting</h2>

      <div class="info" style="margin-bottom:14px;">
        <div class="info-label" id="lblVoiceLanguage">Language &amp; Voice</div>

        <div class="select-row">
          <label id="lblUiLanguage">Bahasa Tampilan (UI)</label>
          <select id="uiLangSelect" onchange="setUILanguage(this.value)">
            <option value="id">🇮🇩 Indonesia</option>
            <option value="en">🇬🇧 English</option>
          </select>
        </div>

        <div class="select-row">
          <label id="lblAnnounceLanguage">Bahasa Pengumuman (Suara)</label>
          <select id="announceLangSelect" onchange="setAnnounceLanguage(this.value)">
            <option value="id">🇮🇩 Indonesia</option>
            <option value="en">🇬🇧 English</option>
          </select>
        </div>

        <div class="select-row">
          <label id="lblVoiceRate">Kecepatan Suara</label>
          <select id="voiceRateSelect" onchange="setVoiceRate(this.value)">
            <option value="0.75">0.75x -- Pelan</option>
            <option value="1">1x -- Normal</option>
            <option value="1.25">1.25x -- Cepat</option>
            <option value="1.5">1.5x -- Sangat Cepat</option>
          </select>
        </div>

        <div class="select-row">
          <label id="lblVoiceGender">Jenis Suara</label>
          <select id="voiceGenderSelect" onchange="setVoiceGender(this.value)">
            <option value="auto">Bawaan Perangkat</option>
            <option value="female">Wanita</option>
            <option value="male">Pria</option>
          </select>
        </div>

        <div class="settings-hint" id="voiceGenderHint" style="margin-top:2px;">
          Pilihan Wanita/Pria tergantung suara yang tersedia di perangkat/browser
          -- kalau cuma ada satu suara terpasang, pilihan ini mungkin tidak
          berpengaruh. Coba tombol "Uji" di salah satu pengumuman di bawah
          buat dengar hasilnya.
        </div>
      </div>

      <div class="info" style="margin-bottom:14px;">
        <div class="info-label">Race Start Sound / Horn MP3</div>

        <input
          type="file"
          id="hornFile"
          accept=".mp3,audio/mpeg">

        <br><br>

        <button id="btnUploadHorn" class="btn-save" onclick="uploadHornSound()">
          Upload Horn
        </button>

        <button id="btnTestHorn" class="btn-start" onclick="testHornSound()">
          Test Horn
        </button>

        <div id="hornStatus" style="color:#94a3b8;margin-top:10px;">
          No file uploaded yet
        </div>
      </div>

      <div class="info">
        <div class="info-label" id="lblAnnouncementsTitle">Announcements</div>
        <div class="settings-hint" id="annModularHint" style="margin-top:6px;">
          Tiap pengumuman bisa diatur sendiri kalimatnya pakai placeholder
          seperti {name} atau {position} -- klik "Uji" buat dengar
          hasilnya kapan saja, tidak perlu race beneran jalan.
        </div>

        <div class="ann-section-label" id="lblAnnSectionGeneral">Umum</div>

        <div class="ann-card">
          <div class="ann-card-head">
            <span class="ann-card-title" id="lblAnnCountdown">Countdown (3-2-1)</span>
            <label class="ios-toggle">
              <input type="checkbox" id="annCountdown" checked>
              <span class="ios-toggle-track"></span>
            </label>
          </div>
        </div>

        <div class="ann-card">
          <div class="ann-card-head">
            <span class="ann-card-title" id="lblAnnHorn">Horn (bunyi start)</span>
            <label class="ios-toggle">
              <input type="checkbox" id="annHorn" checked>
              <span class="ios-toggle-track"></span>
            </label>
          </div>
        </div>

        <div class="ann-card" data-ann="raceStart">
          <div class="ann-card-head">
            <span class="ann-card-title" id="lblAnnRaceStart">Race Start</span>
            <label class="ios-toggle">
              <input type="checkbox" id="annRaceStart" checked>
              <span class="ios-toggle-track"></span>
            </label>
          </div>
          <div class="ann-template-row">
            <input type="text" id="annTemplate_raceStart" oninput="onAnnouncementTemplateInput('raceStart')">
            <button class="ann-btn-test" onclick="testAnnouncementTemplate('raceStart')">🔊 Uji</button>
            <button class="ann-btn-reset" onclick="resetAnnouncementTemplate('raceStart')" title="Reset ke default">↺</button>
          </div>
          <div class="ann-hint" id="annHint_raceStart"></div>
        </div>

        <div class="ann-card" data-ann="raceFinish">
          <div class="ann-card-head">
            <span class="ann-card-title" id="lblAnnRaceFinish">Race Finish</span>
            <label class="ios-toggle">
              <input type="checkbox" id="annRaceFinish" checked>
              <span class="ios-toggle-track"></span>
            </label>
          </div>
          <div class="ann-template-row">
            <input type="text" id="annTemplate_raceFinish" oninput="onAnnouncementTemplateInput('raceFinish')">
            <button class="ann-btn-test" onclick="testAnnouncementTemplate('raceFinish')">🔊 Uji</button>
            <button class="ann-btn-reset" onclick="resetAnnouncementTemplate('raceFinish')" title="Reset ke default">↺</button>
          </div>
          <div class="ann-hint" id="annHint_raceFinish"></div>
        </div>

        <div class="ann-card" data-ann="raceStopped">
          <div class="ann-card-head">
            <span class="ann-card-title" id="lblAnnRaceStopped">Race Stopped (ikut toggle "Race Finish")</span>
          </div>
          <div class="ann-template-row">
            <input type="text" id="annTemplate_raceStopped" oninput="onAnnouncementTemplateInput('raceStopped')">
            <button class="ann-btn-test" onclick="testAnnouncementTemplate('raceStopped')">🔊 Uji</button>
            <button class="ann-btn-reset" onclick="resetAnnouncementTemplate('raceStopped')" title="Reset ke default">↺</button>
          </div>
          <div class="ann-hint" id="annHint_raceStopped"></div>
        </div>

        <div class="ann-section-label" id="lblAnnSectionPerDriver">Per Driver (bisa dipilih Semua / Terdepan / Top 3)</div>

        <div class="ann-card" data-ann="lapCount">
          <div class="ann-card-head">
            <span class="ann-card-title" id="lblAnnLapCount">Lap Count</span>
            <label class="ios-toggle">
              <input type="checkbox" id="annLapCount" checked>
              <span class="ios-toggle-track"></span>
            </label>
          </div>
          <div class="ann-card-row">
            <span class="ann-card-row-label" id="lblAnnScopeLabel1">Umumkan untuk</span>
            <select id="annScope_lapCount" onchange="onAnnouncementScopeChange('lapCount')">
              <option value="all">Semua Driver</option>
              <option value="leader">Driver Terdepan (P1)</option>
              <option value="top3">Top 3</option>
            </select>
          </div>
          <div class="ann-template-row">
            <input type="text" id="annTemplate_lapCount" oninput="onAnnouncementTemplateInput('lapCount')">
            <button class="ann-btn-test" onclick="testAnnouncementTemplate('lapCount')">🔊 Uji</button>
            <button class="ann-btn-reset" onclick="resetAnnouncementTemplate('lapCount')" title="Reset ke default">↺</button>
          </div>
          <div class="ann-hint" id="annHint_lapCount"></div>
        </div>

        <div class="ann-card" data-ann="lastLap">
          <div class="ann-card-head">
            <span class="ann-card-title" id="lblAnnLastLap">Last Lap</span>
            <label class="ios-toggle">
              <input type="checkbox" id="annLastLap" checked>
              <span class="ios-toggle-track"></span>
            </label>
          </div>
          <div class="ann-card-row">
            <span class="ann-card-row-label" id="lblAnnScopeLabel2">Umumkan untuk</span>
            <select id="annScope_lastLap" onchange="onAnnouncementScopeChange('lastLap')">
              <option value="all">Semua Driver</option>
              <option value="leader">Driver Terdepan (P1)</option>
              <option value="top3">Top 3</option>
            </select>
          </div>
          <div class="ann-template-row">
            <input type="text" id="annTemplate_lastLap" oninput="onAnnouncementTemplateInput('lastLap')">
            <button class="ann-btn-test" onclick="testAnnouncementTemplate('lastLap')">🔊 Uji</button>
            <button class="ann-btn-reset" onclick="resetAnnouncementTemplate('lastLap')" title="Reset ke default">↺</button>
          </div>
          <div class="ann-hint" id="annHint_lastLap"></div>
        </div>

        <div class="ann-card" data-ann="position">
          <div class="ann-card-head">
            <span class="ann-card-title" id="lblAnnPosition">Position Change</span>
            <label class="ios-toggle">
              <input type="checkbox" id="annPosition" checked>
              <span class="ios-toggle-track"></span>
            </label>
          </div>
          <div class="ann-card-row">
            <span class="ann-card-row-label" id="lblAnnScopeLabel3">Umumkan untuk</span>
            <select id="annScope_position" onchange="onAnnouncementScopeChange('position')">
              <option value="all">Semua Driver</option>
              <option value="leader">Driver Terdepan (P1)</option>
              <option value="top3">Top 3</option>
            </select>
          </div>
          <div class="ann-template-row">
            <input type="text" id="annTemplate_position" oninput="onAnnouncementTemplateInput('position')">
            <button class="ann-btn-test" onclick="testAnnouncementTemplate('position')">🔊 Uji</button>
            <button class="ann-btn-reset" onclick="resetAnnouncementTemplate('position')" title="Reset ke default">↺</button>
          </div>
          <div class="ann-hint" id="annHint_position"></div>
        </div>

        <div class="ann-card" data-ann="finishPosition">
          <div class="ann-card-head">
            <span class="ann-card-title" id="lblAnnFinishPos">Finish Position</span>
            <label class="ios-toggle">
              <input type="checkbox" id="annFinishPosition" checked>
              <span class="ios-toggle-track"></span>
            </label>
          </div>
          <div class="ann-card-row">
            <span class="ann-card-row-label" id="lblAnnScopeLabel4">Umumkan untuk</span>
            <select id="annScope_finishPosition" onchange="onAnnouncementScopeChange('finishPosition')">
              <option value="all">Semua Driver</option>
              <option value="leader">Driver Terdepan (P1)</option>
              <option value="top3">Top 3</option>
            </select>
          </div>
          <div class="ann-template-row">
            <input type="text" id="annTemplate_finishPosition" oninput="onAnnouncementTemplateInput('finishPosition')">
            <button class="ann-btn-test" onclick="testAnnouncementTemplate('finishPosition')">🔊 Uji</button>
            <button class="ann-btn-reset" onclick="resetAnnouncementTemplate('finishPosition')" title="Reset ke default">↺</button>
          </div>
          <div class="ann-hint" id="annHint_finishPosition"></div>
        </div>

        <div class="ann-card" data-ann="bestLap">
          <div class="ann-card-head">
            <span class="ann-card-title" id="lblAnnBestLap">Best Lap</span>
            <label class="ios-toggle">
              <input type="checkbox" id="annBestLap" checked>
              <span class="ios-toggle-track"></span>
            </label>
          </div>
          <div class="ann-card-row">
            <span class="ann-card-row-label" id="lblAnnScopeLabel5">Umumkan untuk</span>
            <select id="annScope_bestLap" onchange="onAnnouncementScopeChange('bestLap')">
              <option value="all">Semua Driver</option>
              <option value="leader">Driver Terdepan (P1)</option>
              <option value="top3">Top 3</option>
            </select>
          </div>
          <div class="ann-template-row">
            <input type="text" id="annTemplate_bestLap" oninput="onAnnouncementTemplateInput('bestLap')">
            <button class="ann-btn-test" onclick="testAnnouncementTemplate('bestLap')">🔊 Uji</button>
            <button class="ann-btn-reset" onclick="resetAnnouncementTemplate('bestLap')" title="Reset ke default">↺</button>
          </div>
          <div class="ann-hint" id="annHint_bestLap"></div>
        </div>

        <div class="ann-card" data-ann="qualifyLapTime">
          <div class="ann-card-head">
            <span class="ann-card-title" id="lblAnnQualifyLapTime">Qualifying Lap Time</span>
            <label class="ios-toggle">
              <input type="checkbox" id="annQualifyLapTime" checked>
              <span class="ios-toggle-track"></span>
            </label>
          </div>
          <div class="ann-card-row">
            <span class="ann-card-row-label" id="lblAnnScopeLabel6">Umumkan untuk</span>
            <select id="annScope_qualifyLapTime" onchange="onAnnouncementScopeChange('qualifyLapTime')">
              <option value="all">Semua Driver</option>
              <option value="leader">Driver Terdepan (P1)</option>
              <option value="top3">Top 3</option>
            </select>
          </div>
          <div class="ann-template-row">
            <input type="text" id="annTemplate_qualifyLapTime" oninput="onAnnouncementTemplateInput('qualifyLapTime')">
            <button class="ann-btn-test" onclick="testAnnouncementTemplate('qualifyLapTime')">🔊 Uji</button>
            <button class="ann-btn-reset" onclick="resetAnnouncementTemplate('qualifyLapTime')" title="Reset ke default">↺</button>
          </div>
          <div class="ann-hint" id="annHint_qualifyLapTime"></div>
        </div>

        <br>

        <button id="btnSaveVoiceSetting" class="btn-save" onclick="saveVoiceSetting()">
          Save Voice Setting
        </button>

        <button class="btn-delete" onclick="resetAllAnnouncementTemplates()" style="margin-left:8px;">
          Reset Semua Kalimat
        </button>
      </div>

    </div>

    <div class="card">
      <h2 id="h2CountdownSetting">Race Countdown</h2>

      <div class="info">
        <div class="select-row">
          <label id="lblCountdownStart">Mulai Hitungan Dari</label>
          <input type="number" id="countdownStartInput" min="1" max="60" value="5" onchange="saveCountdownSettings()" style="width:90px;">
        </div>

        <div class="select-row">
          <label id="lblHornDelayMode">Horn Setelah Hitungan Habis</label>
          <select id="hornDelayModeSelect" onchange="onHornDelayModeChange()">
            <option value="fixed">Tetap (Fixed)</option>
            <option value="random">Acak (Random)</option>
          </select>
        </div>

        <div class="select-row" id="hornFixedRow">
          <label id="lblHornFixedDelay">Delay Tetap (detik)</label>
          <input type="number" id="hornFixedInput" min="0" step="0.5" value="0" onchange="saveCountdownSettings()" style="width:90px;">
        </div>

        <div class="select-row" id="hornRandomRow" style="display:none;">
          <label id="lblHornRandomRange">Rentang Acak (detik)</label>
          <div style="display:flex;gap:8px;align-items:center;">
            <input type="number" id="hornRandomMinInput" min="0" step="0.5" value="5" onchange="saveCountdownSettings()" style="width:70px;">
            <span style="color:#94a3b8;">--</span>
            <input type="number" id="hornRandomMaxInput" min="0" step="0.5" value="10" onchange="saveCountdownSettings()" style="width:70px;">
          </div>
        </div>

        <div class="settings-hint" id="hornDelayHint" style="margin-top:6px;">
          Setelah angka countdown habis (layar menunjukkan "GO"), horn akan
          bunyi sesuai pengaturan ini. Mode Acak berguna supaya driver tidak
          bisa menebak persis kapan start (anti false-start) -- rentangnya
          diundi ulang setiap kali tombol Start ditekan.
        </div>
      </div>
    </div>

    <div class="card">
      <h2 id="h2SystemSettings">System Settings</h2>

      <div class="info" style="margin-bottom:14px;">
        <div class="info-label" id="h2SystemDiagnostics">Device Diagnostics</div>
        <div style="color:#94a3b8;margin-bottom:10px;" id="lblDiagnosticsHint">
          Memory info -- useful to check if the device is running low on
          memory over long uptime (this is what causes the web UI to freeze).
        </div>

        <table style="width:100%;color:#e2e8f0;border-collapse:collapse;">
          <tr><td id="lblFreeHeap">Free Heap</td><td id="diagFreeHeap" style="text-align:right;">-</td></tr>
          <tr><td id="lblMinFreeHeap">Lowest Free Heap Ever</td><td id="diagMinFreeHeap" style="text-align:right;">-</td></tr>
          <tr><td id="lblLargestBlock">Largest Free Block</td><td id="diagLargestBlock" style="text-align:right;">-</td></tr>
          <tr><td id="lblWsClients">WebSocket Clients</td><td id="diagWsClients" style="text-align:right;">-</td></tr>
          <tr><td id="lblUptime">Uptime</td><td id="diagUptime" style="text-align:right;">-</td></tr>
          <tr><td id="lblFwVersion">Firmware Version</td><td id="settingFwVersion" style="text-align:right;color:#38bdf8;font-weight:800;">v4.2</td></tr>
        </table>

        <br>

        <button class="btn-save" id="btnRefreshDiagnostics" onclick="loadDiagnostics()">
          Refresh
        </button>
      </div>

      <div class="info" style="margin-bottom:14px;">
        <div class="info-label">WiFi Network</div>
        <div id="wifiSsidDisplay" style="color:#94a3b8;margin-bottom:10px;">
          SSID: -
        </div>

        <label id="lblNewPassword">New Password (min. 8 characters)</label><br>
        <input type="password" id="wifiPasswordInput" minlength="8" placeholder="New WiFi password">
        <br><br>

        <button class="btn-save" onclick="changeWifiPassword()">
          Change Password &amp; Restart
        </button>

        <div id="wifiPasswordStatus" style="color:#94a3b8;margin-top:10px;"></div>
      </div>

      <div class="info" style="margin-bottom:14px;">
        <div class="info-label">Firmware Update (OTA)</div>
        <div style="color:#94a3b8;margin-bottom:10px;">
          Upload a compiled .bin firmware file. Device will restart automatically
          once the update finishes. Do not power off during upload.
        </div>

        <input type="file" id="otaFile" accept=".bin">
        <br><br>

        <button class="btn-save" onclick="uploadOta()">
          Upload &amp; Update Firmware
        </button>

        <div id="otaProgressWrap" style="display:none;margin-top:12px;">
          <div style="background:#1e293b;border-radius:8px;overflow:hidden;height:18px;">
            <div id="otaProgressBar" style="background:#22c55e;height:100%;width:0%;transition:width .2s;"></div>
          </div>
        </div>

        <div id="otaStatus" style="color:#94a3b8;margin-top:10px;"></div>
      </div>

      <div class="info">
        <div class="info-label" style="color:#f87171;">Danger Zone</div>
        <div style="color:#94a3b8;margin-bottom:10px;">
          This wipes ALL data on the device (drivers, race history, qualify
          history, settings, WiFi password) back to factory defaults. This
          cannot be undone.
        </div>

        <button class="btn-stop" onclick="factoryReset()">
          Reset to Factory Default
        </button>

        <div id="factoryResetStatus" style="color:#94a3b8;margin-top:10px;"></div>
      </div>
    </div>
  </div>
</div>

<script>
let firstLoad = true;
let lastRaceState = "";
let lastCountdownVoice = null;

let previousRacers = {};
let lastLapAnnounced = {};
let finishAnnounced = {};
let raceFinishAnnounced = false;
let bestLapRecord = {};
let lastLapCountRecord = {};

let terminalEntries = []; // {time, idDecimal, hits, quality} -- terbaru duluan
let audioUnlocked = false;
let currentDrivers = []; // cache driver list terbaru, dipakai fitur Last Detected ID & Search

// Satu Audio object yang dipakai ULANG terus (bukan `new Audio()` tiap
// mau bunyi). Ini kunci fix horn tidak keluar di HP: browser mobile
// (terutama iOS Safari) cuma meng-unlock instance Audio yang SAMA yang
// disentuh saat user gesture -- kalau tiap play bikin objek Audio baru,
// unlock sebelumnya jadi tidak berlaku buat objek baru itu.
let hornAudio = new Audio();
hornAudio.preload = 'auto';

// src pakai cache-buster tiap play (bukan cuma sekali di awal), supaya
// kalau user upload horn.mp3 baru lewat menu Settings, yang kebunyi
// tetap file terbaru, bukan cache lama -- walau objek Audio-nya sama.
function playHorn(onCantPlay) {
  try {
    hornAudio.src = '/start_horn.mp3?t=' + Date.now();
    hornAudio.currentTime = 0;
    hornAudio.volume = 1;

    const p = hornAudio.play();

    if (p && p.catch) {
      p.catch(() => {
        if (onCantPlay) onCantPlay();
      });
    }
  } catch (e) {
    if (onCantPlay) onCantPlay();
  }
}


function showPage(page) {
  document.getElementById('qualifyPage').classList.remove('active');
  document.getElementById('tabQualify').classList.remove('active');
  document.getElementById('racePage').classList.remove('active');
  document.getElementById('driverPage').classList.remove('active');
  document.getElementById('resultPage').classList.remove('active');
  document.getElementById('settingPage').classList.remove('active');

  document.getElementById('tabRace').classList.remove('active');
  document.getElementById('tabDriver').classList.remove('active');
  document.getElementById('tabResult').classList.remove('active');
  document.getElementById('tabSetting').classList.remove('active');

  if (page === 'race') {

    document.getElementById('racePage').classList.add('active');
    document.getElementById('tabRace').classList.add('active');

  }

  else if (page === 'qualify') {

  document.getElementById('qualifyPage').classList.add('active');
  document.getElementById('tabQualify').classList.add('active');

  }
  else if (page === 'driver') {

    document.getElementById('driverPage').classList.add('active');
    document.getElementById('tabDriver').classList.add('active');

  }
  else if (page === 'result') {

    document.getElementById('resultPage').classList.add('active');
    document.getElementById('tabResult').classList.add('active');
    showResultTab('race');

  }
  else if (page === 'setting') {

  document.getElementById('settingPage').classList.add('active');
  document.getElementById('tabSetting').classList.add('active');

  }
}

function updateClock() {
  const now = new Date();
  document.getElementById('clock').innerHTML = now.toLocaleTimeString();
}

function formatMs(ms) {
  if (!ms || ms <= 0) return "-";

  let minutes = Math.floor(ms / 60000);
  let seconds = Math.floor((ms % 60000) / 1000);
  let millis = ms % 1000;

  return String(minutes).padStart(2, '0') + ":" +
         String(seconds).padStart(2, '0') + "." +
         String(millis).padStart(3, '0');
}

// v4.3: khusus buat JAM RACE yang berjalan (raceTimer, overlayRaceTimer) --
// BEDA dari formatMs() di atas. formatMs() sengaja tampil "-" kalau
// ms<=0 karena dipakai buat Best Lap/Last Lap/Avg Lap yang MEMANG belum
// ada datanya saat racer belum menyelesaikan lap (0 di situ artinya
// "belum ada data"). Tapi untuk jam race yang berjalan, 0 itu BUKAN
// "belum ada data" -- itu memang race yang baru mulai di detik ke-0, dan
// harus tetap tampil sebagai jam (00:00:000), bukan mendadak ganti jadi
// tanda "-" lalu balik lagi ke angka begitu heartbeat berikutnya datang
// (ini salah satu penyebab jam kelihatan "kedip2").
function formatRaceClock(ms) {
  if (typeof ms !== 'number' || isNaN(ms) || ms < 0) ms = 0;

  let minutes = Math.floor(ms / 60000);
  let seconds = Math.floor((ms % 60000) / 1000);
  let millisPart = Math.floor(ms % 1000);

  return String(minutes).padStart(2, '0') + ":" +
         String(seconds).padStart(2, '0') + ":" +
         String(millisPart).padStart(3, '0');
}

// --- Indikator performa (Consistency, Gap to Leader, Trend) ---
// Semua dihitung MURNI di browser dari lapLogs yang sudah ada (tidak
// perlu data tambahan dari device) -- dipakai bareng di leaderboard Race,
// leaderboard Qualifying, dan halaman History (race/qualify detail).

// Consistency = standar deviasi lap time. Ditampilkan dalam bentuk PERSEN
// (relatif terhadap rata-rata lap) -- lebih gampang dibaca sekilas
// dibanding angka detik mentah, dan konsisten di berbagai kelas/track
// (mobil lambat vs cepat sama-sama bisa dibandingkan "seberapa stabil").
// 100% = sempurna konsisten, makin kecil = makin naik-turun lap time-nya.
function calcConsistency(lapLogs) {
  if (!lapLogs || lapLogs.length < 2) return null;

  const mean = lapLogs.reduce((a, b) => a + b, 0) / lapLogs.length;
  const variance = lapLogs.reduce((a, b) => a + Math.pow(b - mean, 2), 0) / lapLogs.length;

  return Math.sqrt(variance); // ms
}

function consistencyPercent(lapLogs) {
  const sd = calcConsistency(lapLogs);
  if (sd === null || !lapLogs || lapLogs.length === 0) return null;

  const mean = lapLogs.reduce((a, b) => a + b, 0) / lapLogs.length;
  if (mean <= 0) return null;

  const pct = 100 - (sd / mean * 100);
  return Math.max(0, Math.min(100, pct));
}

function consistencyText(lapLogs) {
  const pct = consistencyPercent(lapLogs);
  if (pct === null) return '-';
  return pct.toFixed(1) + '%';
}

// Gap to Leader untuk RACE: kalau lap-nya sama dengan leader, bandingkan
// total waktu tempuh sejauh lap itu. Kalau lap-nya lebih sedikit dari
// leader, tampilkan selisih lap ("+1 Lap") -- konvensi umum di race
// timing. Catatan: ini pendekatan dari total lapLogs, bukan dari crossing
// timestamp presisi, karena device tidak menyimpan timestamp per-crossing
// -- cukup akurat untuk gambaran gap, bukan buat keperluan protes resmi.
function raceGapToLeaderText(racer, leader) {
  if (!racer || !leader) return '-';
  if (racer.idDecimal === leader.idDecimal) return 'Leader';
  if (!racer.laps || racer.laps === 0) return '-';

  if (racer.laps < leader.laps) {
    const lapDiff = leader.laps - racer.laps;
    return '+' + lapDiff + ' Lap' + (lapDiff > 1 ? 's' : '');
  }

  const selfTotal = (racer.lapLogs || []).reduce((a, b) => a + b, 0);
  const leaderTotal = (leader.lapLogs || []).reduce((a, b) => a + b, 0);
  const diff = selfTotal - leaderTotal;

  if (diff <= 0) return 'Leader';
  return '+' + (diff / 1000).toFixed(2) + 's';
}

// Gap to Leader untuk QUALIFYING: bukan cumulative time (karena qualifying
// tidak start bareng-bareng), tapi selisih BEST LAP ke best lap milik P1
// -- ini istilah standar "gap to pole" di motorsport.
function qualifyGapToLeaderText(entry, leader) {
  if (!entry || !leader) return '-';
  if (entry.idDecimal === leader.idDecimal) return 'Leader';
  if (!entry.bestLapTime || entry.bestLapTime <= 0) return '-';
  if (!leader.bestLapTime || leader.bestLapTime <= 0) return '-';

  const diff = entry.bestLapTime - leader.bestLapTime;
  if (diff <= 0) return 'Leader';
  return '+' + (diff / 1000).toFixed(2) + 's';
}

// Trend = rata-rata paruh kedua lap dibanding paruh pertama, dalam PERSEN
// (konsisten dengan Consistency yang juga persen -- dulu salah satu detik
// satu lagi tidak ada satuan jelas, bikin bingung). Negatif = makin cepat
// (membaik), positif = makin lambat (menurun). Butuh minimal 4 lap.
function trendPlainText(lapLogs) {
  if (!lapLogs || lapLogs.length < 4) return '-';

  const mid = Math.floor(lapLogs.length / 2);
  const firstHalf = lapLogs.slice(0, mid);
  const secondHalf = lapLogs.slice(mid);
  const avg = arr => arr.reduce((a, b) => a + b, 0) / arr.length;

  const firstAvg = avg(firstHalf);
  const diff = avg(secondHalf) - firstAvg;
  if (firstAvg <= 0) return '-';

  const pct = (diff / firstAvg) * 100;
  const pctAbs = Math.abs(pct).toFixed(1);

  if (pct < -1) return 'Membaik (-' + pctAbs + '%)';
  if (pct > 1) return 'Menurun (+' + pctAbs + '%)';
  return 'Stabil';
}

function trendText(lapLogs) {
  const plain = trendPlainText(lapLogs);

  if (plain === '-') return '-';
  if (plain.startsWith('Membaik')) return `<span style="color:#22c55e;">▲ ${plain}</span>`;
  if (plain.startsWith('Menurun')) return `<span style="color:#f87171;">▼ ${plain}</span>`;
  return `<span style="color:#94a3b8;">● ${plain}</span>`;
}

// Bikin blok kecil 3 indikator, dipasang di atas daftar lap-log di detail
// row yang bisa di-expand (racer-laplogs-*) -- supaya tabel utama tetap
// ringkas/rapi terutama di HP, indikator baru muncul kalau di-expand.
// --- Grafik Posisi per Lap ---
// Dipakai di halaman Race (live), Qualifying (live), detail hasil race
// tersimpan, dan detail hasil qualifying tersimpan. Posisi di tiap lap
// dihitung dari total waktu tempuh KUMULATIF sampai lap itu (ranking
// makin kecil kumulatifnya = makin depan) -- entry yang belum nyampe
// lap tsb otomatis tidak ikut dihitung (garis chart-nya berhenti).
const CHART_COLORS = ['#38bdf8', '#facc15', '#22c55e', '#f472b6', '#f97316',
  '#a78bfa', '#f87171', '#2dd4bf', '#fb923c', '#c084fc'];

function computePositionsPerLap(entries) {
  const list = (entries || []).filter(e => e.lapLogs && e.lapLogs.length > 0);
  let maxLap = 0;
  list.forEach(e => { if (e.lapLogs.length > maxLap) maxLap = e.lapLogs.length; });

  const positions = {}; // idDecimal -> [pos di lap1, pos di lap2, ...] (null kalau belum sampai)
  list.forEach(e => { positions[e.idDecimal] = []; });

  for (let lap = 1; lap <= maxLap; lap++) {
    const standings = list
      .filter(e => e.lapLogs.length >= lap)
      .map(e => ({
        idDecimal: e.idDecimal,
        cumulative: e.lapLogs.slice(0, lap).reduce((a, b) => a + b, 0)
      }))
      .sort((a, b) => a.cumulative - b.cumulative);

    standings.forEach((s, i) => { positions[s.idDecimal].push(i + 1); });

    list.forEach(e => {
      if (e.lapLogs.length < lap) positions[e.idDecimal].push(null);
    });
  }

  return { maxLap, positions, list };
}

// Render grafik SVG polyline posisi-per-lap ke dalam elemen dengan id
// `containerId`. `entries` = array racer/qualifier (perlu .name,
// .idDecimal, .lapLogs). Kalau belum ada lap sama sekali, tampilkan
// pesan kosong drpd chart kosong yang membingungkan.
function renderPositionChart(containerId, entries) {
  const container = document.getElementById(containerId);
  if (!container) return;

  const { maxLap, positions, list } = computePositionsPerLap(entries);

  if (!list.length || maxLap === 0) {
    container.innerHTML = `<div class="chart-wrap"><div class="chart-empty">Grafik posisi muncul begitu ada lap tercatat</div></div>`;
    return;
  }

  const totalEntries = list.length;
  const W = Math.max(360, maxLap * 60);
  const H = 60 + totalEntries * 4 + 160;
  const padL = 44, padR = 20, padT = 20, padB = 34;
  const plotW = W - padL - padR;
  const plotH = H - padT - padB;

  const xFor = lap => padL + (maxLap <= 1 ? 0 : (plotW * (lap - 1) / (maxLap - 1)));
  const yFor = pos => padT + (plotH * (pos - 1) / Math.max(1, totalEntries - 1));

  let svg = `<svg viewBox="0 0 ${W} ${H}" xmlns="http://www.w3.org/2000/svg" style="width:100%;height:auto;max-width:${W}px;">`;

  // grid horizontal per posisi
  for (let p = 1; p <= totalEntries; p++) {
    const y = yFor(p);
    svg += `<line x1="${padL}" y1="${y}" x2="${W - padR}" y2="${y}" stroke="rgba(148,163,184,0.14)" stroke-width="1"/>`;
    svg += `<text x="${padL - 10}" y="${y + 4}" text-anchor="end" font-size="11" fill="#94a3b8">P${p}</text>`;
  }

  // grid vertikal + label lap
  for (let lap = 1; lap <= maxLap; lap++) {
    const x = xFor(lap);
    svg += `<text x="${x}" y="${H - 10}" text-anchor="middle" font-size="11" fill="#94a3b8">${lap}</text>`;
  }

  list.forEach((e, i) => {
    const color = CHART_COLORS[i % CHART_COLORS.length];
    const pts = positions[e.idDecimal];
    let pathD = '';
    let started = false;

    pts.forEach((pos, idx) => {
      if (pos === null) { started = false; return; }
      const x = xFor(idx + 1), y = yFor(pos);
      pathD += (started ? ' L ' : ' M ') + x + ' ' + y;
      started = true;
    });

    if (pathD) {
      svg += `<path d="${pathD.trim()}" fill="none" stroke="${color}" stroke-width="2.5" stroke-linejoin="round" stroke-linecap="round"/>`;
    }

    pts.forEach((pos, idx) => {
      if (pos === null) return;
      svg += `<circle cx="${xFor(idx + 1)}" cy="${yFor(pos)}" r="3.5" fill="${color}"/>`;
    });
  });

  svg += `</svg>`;

  let legend = `<div class="chart-legend">`;
  list.forEach((e, i) => {
    const color = CHART_COLORS[i % CHART_COLORS.length];
    legend += `<div class="chart-legend-item"><span class="chart-legend-dot" style="background:${color}"></span>${e.name}</div>`;
  });
  legend += `</div>`;

  container.innerHTML = `<div class="chart-wrap">${svg}${legend}</div>`;
}

function indicatorStatsHtml(entry, leader, gapFn, lapLogs) {
  return `
    <div style="display:flex;flex-wrap:wrap;gap:10px;margin-bottom:10px;">
      <div style="background:#0f172a;border:1px solid rgba(56,189,248,0.25);border-radius:10px;padding:6px 12px;">
        <span style="color:#94a3b8;font-size:12px;">Consistency</span><br>
        <b>${consistencyText(lapLogs)}</b>
      </div>
      <div style="background:#0f172a;border:1px solid rgba(56,189,248,0.25);border-radius:10px;padding:6px 12px;">
        <span style="color:#94a3b8;font-size:12px;">Gap to Leader</span><br>
        <b>${gapFn(entry, leader)}</b>
      </div>
      <div style="background:#0f172a;border:1px solid rgba(56,189,248,0.25);border-radius:10px;padding:6px 12px;">
        <span style="color:#94a3b8;font-size:12px;">Trend</span><br>
        <b>${trendText(lapLogs)}</b>
      </div>
    </div>
  `;
}

// --- Export CSV (race & qualify) ---
// Sengaja CSV, bukan .xlsx asli -- generate Excel binary butuh library
// tambahan yang berat buat ESP32, sedangkan CSV simpel di-generate device
// dan tetap kebuka mulus di Excel/Google Sheets. Semua dilakukan
// CLIENT-SIDE dari data yang sudah di-fetch (tidak nambah beban device).

function csvEscape(val) {
  val = val === null || val === undefined ? '' : String(val);
  if (val.indexOf(',') !== -1 || val.indexOf('"') !== -1 || val.indexOf('\n') !== -1) {
    return '"' + val.replace(/"/g, '""') + '"';
  }
  return val;
}

function secText(ms) {
  return (ms && ms > 0) ? (ms / 1000).toFixed(3) : '';
}

function triggerCsvDownload(filename, csvContent) {
  // Prefix BOM (\ufeff) supaya Excel otomatis kebaca sebagai UTF-8 --
  // tanpa ini, karakter seperti "±" atau nama dengan huruf non-ASCII bisa
  // muncul berantakan di Excel (walau tetap benar di text editor biasa).
  const blob = new Blob(["\ufeff" + csvContent], { type: 'text/csv;charset=utf-8;' });
  const url = URL.createObjectURL(blob);

  const a = document.createElement('a');
  a.href = url;
  a.download = filename;
  document.body.appendChild(a);
  a.click();
  document.body.removeChild(a);
  URL.revokeObjectURL(url);
}

function buildRaceCsv(data) {
  const leader = data.racers[0];
  let maxLaps = 0;

  data.racers.forEach(r => {
    if (r.lapLogs && r.lapLogs.length > maxLaps) maxLaps = r.lapLogs.length;
  });

  const { positions } = computePositionsPerLap(data.racers);

  let headers = [
    'Rank', 'Name', 'Transponder ID', 'Laps', 'Best Lap (s)', 'Average Lap (s)',
    'Status', 'Consistency (%)', 'Gap to Leader', 'Trend'
  ];

  for (let i = 1; i <= maxLaps; i++) headers.push('Lap ' + i + ' (s)');
  for (let i = 1; i <= maxLaps; i++) headers.push('Pos Lap ' + i);

  let rows = [headers];

  data.racers.forEach(r => {
    const pct = consistencyPercent(r.lapLogs);

    let row = [
      r.rank, r.name, r.idDecimal, r.laps,
      secText(r.bestLapTime), secText(r.averageLapTime),
      r.status,
      pct !== null ? pct.toFixed(1) : '',
      raceGapToLeaderText(r, leader),
      trendPlainText(r.lapLogs)
    ];

    for (let i = 0; i < maxLaps; i++) {
      const lap = (r.lapLogs || [])[i];
      row.push(lap ? secText(lap) : '');
    }

    const posArr = positions[r.idDecimal] || [];
    for (let i = 0; i < maxLaps; i++) {
      row.push(posArr[i] ? posArr[i] : '');
    }

    rows.push(row);
  });

  return rows.map(r => r.map(csvEscape).join(',')).join('\r\n');
}

function buildQualifyCsv(data) {
  const leader = data.qualifiers[0];
  let maxLaps = 0;

  data.qualifiers.forEach(q => {
    if (q.lapLogs && q.lapLogs.length > maxLaps) maxLaps = q.lapLogs.length;
  });

  let headers = [
    'Rank', 'Name', 'Transponder ID', 'Laps', 'Best Lap (s)', 'Average Lap (s)',
    'Consistency (%)', 'Gap to Leader', 'Trend'
  ];

  for (let i = 1; i <= maxLaps; i++) headers.push('Lap ' + i + ' (s)');

  let rows = [headers];

  data.qualifiers.forEach(q => {
    const pct = consistencyPercent(q.lapLogs);

    let row = [
      q.rank, q.name, q.idDecimal, q.laps,
      secText(q.bestLapTime), secText(q.averageLapTime),
      pct !== null ? pct.toFixed(1) : '',
      qualifyGapToLeaderText(q, leader),
      trendPlainText(q.lapLogs)
    ];

    for (let i = 0; i < maxLaps; i++) {
      const lap = (q.lapLogs || [])[i];
      row.push(lap ? secText(lap) : '');
    }

    rows.push(row);
  });

  return rows.map(r => r.map(csvEscape).join(',')).join('\r\n');
}

// Cache data detail yang lagi ditampilkan, supaya tombol download tinggal
// pakai ulang tanpa fetch lagi.
let currentRaceDetailData = null;
let currentQualifyDetailData = null;

function downloadRaceCsv() {
  if (!currentRaceDetailData) return;
  const csv = buildRaceCsv(currentRaceDetailData);
  const safeName = currentRaceDetailData.raceName.replace(/[^a-z0-9]+/gi, '_');
  triggerCsvDownload(safeName + '.csv', csv);
}

function downloadQualifyCsv() {
  if (!currentQualifyDetailData) return;
  const csv = buildQualifyCsv(currentQualifyDetailData);
  const safeName = currentQualifyDetailData.qualifyName.replace(/[^a-z0-9]+/gi, '_');
  triggerCsvDownload(safeName + '.csv', csv);
}

function setStatusStyle(status) {
  const badge = document.getElementById('raceStatusBadge');
  if (!badge) return;

  badge.className = "status-badge";

  if (status === "RUNNING") badge.classList.add("status-running", "is-live");
  else if (status === "COUNTDOWN") badge.classList.add("status-countdown", "is-live");
  else if (status === "STOPPED") badge.classList.add("status-stopped");
  else badge.classList.add("status-idle");
}

// Sama seperti setStatusStyle() tapi buat badge Qualify Status -- dipisah
// fungsinya (bukan status race) tapi visualnya konsisten: warna + titik
// berdenyut pas RUNNING, supaya operator tidak perlu baca teks buat tahu
// qualifying lagi jalan atau tidak.
function setQualifyStatusStyle(isRunning) {
  const badge = document.getElementById('qualifyStatusBadge');
  const text = document.getElementById('qualifyStatus');
  if (text) text.innerHTML = isRunning ? "RUNNING" : "STOPPED";
  if (!badge) return;

  badge.className = "status-badge";
  badge.classList.add(isRunning ? "status-running" : "status-stopped");
  if (isRunning) badge.classList.add("is-live");
}

function racerStatusClass(status) {
  if (status === "Finish") return "finish";
  if (status === "In Race") return "inrace";
  if (status === "DNF") return "dnf";
  if (status === "DNS") return "dns";
  return "ready";
}

// Notifikasi singkat di atas layar -- dipakai buat kasih tahu operator
// kalau Race/Qualifying ditolak start (satunya lagi jalan), atau pesan
// singkat lain yang perlu kelihatan sekilas tanpa mengganggu alur.
function showToast(message, durationMs = 3200) {
  const wrap = document.getElementById('toastWrap');
  if (!wrap) { alert(message); return; }

  const el = document.createElement('div');
  el.className = 'toast';
  el.textContent = message;
  wrap.appendChild(el);

  setTimeout(() => { el.remove(); }, durationMs);
}

// --- Overlay Race (green screen untuk OBS / software recording lain) ---
function isOverlayMode() {
  return new URLSearchParams(location.search).get('overlay') === '1';
}

function openRaceOverlay() {
  const url = location.origin + location.pathname + '?overlay=1';
  window.open(url, 'yurlapsOverlay', 'width=380,height=640,noopener');
}

// Dipanggil dari tombol "Buka Overlay Replay" di detail Hasil Race
// tersimpan. BUKAN rekaman video/mp4 -- ini cuma jendela overlay yang
// sama, tapi datanya "diputar ulang" dari file race tersimpan dengan
// jarak waktu antar lap PERSIS seperti aslinya (dihitung dari lapLogs),
// jadi kalau ditumpuk manual dengan rekaman video race yang sudah ada,
// alurnya cocok.
function openReplayOverlay(file) {
  const url = location.origin + location.pathname +
    '?overlay=1&replay=' + encodeURIComponent(file);
  window.open(url, 'yurlapsOverlayReplay', 'width=380,height=640,noopener');
}

let overlayReplayActive = false;

// v4.5 FIX: dulu SEMUA baris ikut dihitung ulang arah panahnya setiap
// renderOverlayBoard() dipanggil -- padahal fungsi ini dipanggil ulang
// tiap ADA SAJA satu racer yang nge-lap (bukan cuma racer yang
// bersangkutan). Akibatnya driver yang diam di posisinya bisa ikut
// "ketimpa" hitungan ulang gara-gara racer LAIN lewat garis finish.
// Sekarang dilacak per-racer: `overlayLastKnownLaps` buat tahu apakah
// racer ini SENDIRI yang baru saja nambah lap, dan kalau ya baru
// panahnya (arah + selisih posisi) dihitung ulang dari posisi dia pas
// lap SEBELUMNYA (`overlayLastLapPosition`) -- lalu disimpan beku di
// `overlayFrozenArrow` dan dipakai apa adanya selama racer itu belum
// nge-lap lagi, walau racer lain lewat finish berkali-kali di antaranya.
let overlayLastKnownLaps = {};
let overlayLastLapPosition = {};
let overlayFrozenArrow = {};

// Timer overlay diupdate dari renderData() DAN renderLiveData() (heartbeat
// tiap ~200ms) supaya jalannya mulus. Leaderboard overlay (nama, posisi,
// gap) cuma bisa diupdate dari renderData() (full state, tiap ada lap
// beneran tercatat) karena payload heartbeat tidak membawa nama driver
// atau lapLogs -- tapi itu memang cukup, karena posisi & gap di race cuma
// benar-benar berubah persis di momen lap tercatat, bukan tiap detik.
function renderOverlayTimer(raceTime) {
  if (!document.body.classList.contains('overlay-mode')) return;
  const el = document.getElementById('overlayRaceTimer');
  if (el && typeof raceTime === 'number') el.innerHTML = formatRaceClock(raceTime);
}

// Render tabel overlay dari array racer yang SUDAH TERURUT (posisi 1 di
// index 0) -- dipakai baik oleh renderOverlay() (data live dari
// WebSocket) maupun replayTick() (data hasil simulasi dari file
// tersimpan), supaya tampilan & logika panah/gap-nya identik persis di
// kedua mode. `lapTarget` dipakai buat kolom Lap (mis. "6/10") dan buat
// nentuin kapan tampil "FINISH". Nomor posisi (kolom Pos) SELALU
// dihitung ulang tiap render supaya akurat buat semua orang -- yang
// "dibekukan" cuma indikator panahnya, sesuai catatan di atas.
function renderOverlayBoard(racersSorted, lapTarget) {
  const board = document.getElementById('overlayBoard');
  if (!board) return;

  if (!racersSorted || racersSorted.length === 0) {
    board.innerHTML = '<div class="overlay-empty">Menunggu race dimulai...</div>';
    return;
  }

  // --- FLIP animation (First-Last-Invert-Play), langkah "First" ---
  // Catat posisi Y tiap baris SEBELUM DOM diganti, supaya begitu urutan
  // berubah, baris yang pindah kelihatan BERGESER halus ke tempat
  // barunya -- bukan langsung "loncat" begitu saja kayak sebelumnya.
  const firstRects = {};
  racersSorted.forEach(r => {
    const el = document.getElementById('overlay-row-' + r.idDecimal);
    if (el) firstRects[r.idDecimal] = el.getBoundingClientRect().top;
  });

  const leader = racersSorted[0];
  let html = '';
  const justLappedIds = [];

  racersSorted.forEach((racer, index) => {
    const pos = index + 1;
    const id = racer.idDecimal;
    const lastKnownLaps = overlayLastKnownLaps[id];

    // Racer ini dianggap "baru saja lewat garis finish" kalau jumlah
    // lap-nya SEKARANG lebih banyak dari terakhir kali kita cek KHUSUS
    // buat racer ini (bukan dibandingkan racer lain).
    const justLapped = lastKnownLaps === undefined
      ? racer.laps > 0
      : racer.laps > lastKnownLaps;

    if (justLapped) {
      justLappedIds.push(id);

      const prevPos = overlayLastLapPosition[id];
      let arrowClass = 'same', arrowChar = '▬';

      if (prevPos !== undefined && prevPos !== pos) {
        const delta = prevPos - pos;
        if (delta > 0) { arrowClass = 'up'; arrowChar = '▲' + delta; }
        else { arrowClass = 'down'; arrowChar = '▼' + Math.abs(delta); }
      }

      overlayFrozenArrow[id] = { arrowClass, arrowChar };
      overlayLastLapPosition[id] = pos;
    }

    overlayLastKnownLaps[id] = racer.laps;

    const frozen = overlayFrozenArrow[id] || { arrowClass: 'same', arrowChar: '▬' };

    const isFinished = racer.status === 'Finish' ||
      (lapTarget && racer.laps >= lapTarget);

    const lapsText = isFinished ? 'FINISH' : (racer.laps + '/' + (lapTarget || '-'));

    html += `
      <div class="overlay-row" id="overlay-row-${id}">
        <div class="overlay-pos">${pos}</div>
        <div class="overlay-name">${racer.name}</div>
        <div class="overlay-laps${isFinished ? ' finished' : ''}">${lapsText}</div>
        <div class="overlay-arrow ${frozen.arrowClass}">${frozen.arrowChar}</div>
        <div class="overlay-lastlap">${formatMs(racer.lastLapTime)}</div>
        <div class="overlay-gap">${raceGapToLeaderText(racer, leader)}</div>
      </div>
    `;
  });

  board.innerHTML = html;

  // --- FLIP, langkah "Last, Invert, Play" ---
  // Bandingkan posisi Y baris SEKARANG (setelah DOM diganti) dengan
  // posisi SEBELUMNYA yang dicatat di atas. Kalau beda, "putar balik"
  // baris itu secara visual ke posisi lama pakai transform (tanpa
  // transisi, jadi tidak kelihatan), lalu di frame berikutnya lepas
  // transform-nya DENGAN transisi aktif -- browser otomatis
  // menganimasikan pergeserannya dengan mulus ke posisi barunya.
  racersSorted.forEach(r => {
    const el = document.getElementById('overlay-row-' + r.idDecimal);
    if (!el) return;

    const first = firstRects[r.idDecimal];
    if (first === undefined) return; // baris baru muncul, tidak usah digeser dari mana-mana

    const delta = first - el.getBoundingClientRect().top;
    if (Math.abs(delta) < 1) return;

    el.style.transition = 'none';
    el.style.transform = `translateY(${delta}px)`;

    requestAnimationFrame(() => {
      el.style.transition = 'transform 0.45s cubic-bezier(0.22, 1, 0.36, 1)';
      el.style.transform = '';
    });
  });

  // Efek kilat singkat di baris yang BARU SAJA nambah lap -- supaya
  // kelihatan jelas siapa yang barusan melintas garis finish, terlepas
  // dari animasi pindah posisi di atas (dua-duanya bisa jalan bersamaan).
  justLappedIds.forEach(id => {
    const el = document.getElementById('overlay-row-' + id);
    if (!el) return;

    el.classList.remove('overlay-row-flash');
    void el.offsetWidth; // paksa reflow biar animasi restart tiap lap
    el.classList.add('overlay-row-flash');
  });
}

function renderOverlay(data) {
  // Kalau lagi replay, data live dari WebSocket diabaikan total -- biar
  // tidak numpang nimpa tampilan yang lagi diputar dari file tersimpan.
  if (!document.body.classList.contains('overlay-mode') || overlayReplayActive) return;

  renderOverlayTimer(data.raceTime);
  renderOverlayBoard(data.racers, data.lapTarget);
}

// --- Replay overlay dari file race tersimpan ---
// v4.3: dulu timeline langsung jalan begitu data selesai di-fetch --
// sekarang cuma DISIAPKAN (data diambil, ditampung di
// pendingReplayRacers), lalu tombol "Mulai Replay" ditampilkan dan
// MENUNGGU diklik. Ini kasih waktu buka software recording (OBS dkk)
// dan pastikan semuanya siap SEBELUM timeline/countdown/horn jalan --
// dulu tidak ada jeda sama sekali begitu jendela overlay dibuka.
let pendingReplayRacers = null;
let pendingReplayLapTarget = null;

function startOverlayReplay(file) {
  overlayReplayActive = true;

  document.getElementById('overlayBoard').innerHTML =
    '<div class="overlay-empty">Memuat data race...</div>';

  fetch('/raceDetail?file=' + encodeURIComponent(file))
    .then(res => res.json())
    .then(data => {
      const racers = (data.racers || []).filter(r => r.lapLogs && r.lapLogs.length > 0);

      if (!racers.length) {
        document.getElementById('overlayBoard').innerHTML =
          '<div class="overlay-empty">Race ini belum ada lap tercatat</div>';
        return;
      }

      pendingReplayRacers = racers;
      pendingReplayLapTarget = data.lapTarget || null;

      document.getElementById('overlayBoard').innerHTML =
        '<div class="overlay-empty">Siap direplay -- klik Mulai kalau recording sudah jalan</div>';

      const startBtn = document.getElementById('overlayStartBtn');
      if (startBtn) startBtn.style.display = '';
    })
    .catch(() => {
      document.getElementById('overlayBoard').innerHTML =
        '<div class="overlay-empty">Gagal memuat data race ini</div>';
    });
}

// Dipanggil dari klik tombol "Mulai Replay". Jalankan countdown 3-2-1 +
// horn PERSIS seperti race live (playCountdownVoice/playHorn -- reuse
// fungsi yang sama, tidak ada logika duplikat), baru setelah horn bunyi
// timeline replay yang sebenarnya mulai jalan dari t=0.
function beginOverlayReplayCountdown() {
  if (!pendingReplayRacers) return;

  const startBtn = document.getElementById('overlayStartBtn');
  if (startBtn) startBtn.style.display = 'none';

  const cd = document.getElementById('overlayCountdownDisplay');
  const board = document.getElementById('overlayBoard');
  board.innerHTML = '';
  cd.style.display = 'block';

  let n = 3;
  cd.textContent = n;
  playCountdownVoice(n);

  const timer = setInterval(() => {
    n--;

    if (n > 0) {
      cd.textContent = n;
      playCountdownVoice(n);
      return;
    }

    // n === 0 -- tampilkan "GO", bunyikan horn, timeline mulai jalan
    clearInterval(timer);
    cd.textContent = "GO";
    playHorn();

    setTimeout(() => {
      cd.style.display = 'none';
      runOverlayReplayTimeline(pendingReplayRacers, pendingReplayLapTarget);
    }, 700);
  }, 1000);
}

// Dipanggil dari tombol "Ulangi Replay" setelah timeline selesai --
// balik ke state "siap Mulai" pakai data yang SAMA (tanpa fetch ulang ke
// server), bukan location.reload() seperti sebelumnya.
function restartOverlayReplay() {
  const restartBtn = document.getElementById('overlayReplayRestart');
  if (restartBtn) restartBtn.style.display = 'none';

  document.getElementById('overlayBoard').innerHTML =
    '<div class="overlay-empty">Siap direplay -- klik Mulai kalau recording sudah jalan</div>';
  document.getElementById('overlayRaceTimer').innerHTML = formatRaceClock(0);
  overlayLastKnownLaps = {};
  overlayLastLapPosition = {};
  overlayFrozenArrow = {};

  const startBtn = document.getElementById('overlayStartBtn');
  if (startBtn) startBtn.style.display = '';
}

// Rekonstruksi urutan posisi berdasarkan lapLogs (durasi tiap lap) --
// pada waktu t (ms sejak race mulai), racer dianggap sudah menyelesaikan
// lap ke-N kalau jumlah kumulatif durasi lap 1..N <= t. Racer diurutkan
// sama persis seperti leaderboard race asli: lap TERBANYAK dulu, kalau
// sama, waktu tempuh kumulatif TERKECIL menang. `lastLapTime` ikut
// dihitung (durasi lap terakhir yang sudah kelar) buat kolom Last Lap.
function computeReplayStateAt(racers, t) {
  const state = racers.map(r => {
    let laps = 0, cum = 0;
    for (let i = 0; i < r.lapLogs.length; i++) {
      const next = cum + r.lapLogs[i];
      if (next <= t) { cum = next; laps++; } else break;
    }

    const completed = r.lapLogs.slice(0, laps);

    return {
      idDecimal: r.idDecimal,
      name: r.name,
      laps,
      lapLogs: completed,
      lastLapTime: laps > 0 ? completed[completed.length - 1] : 0
    };
  });

  state.sort((a, b) => (b.laps - a.laps) || (a.laps === 0 ? 0 :
    (a.lapLogs.reduce((x, y) => x + y, 0) - b.lapLogs.reduce((x, y) => x + y, 0))));

  return state;
}

function runOverlayReplayTimeline(racers, lapTarget) {
  let maxTime = 0;
  racers.forEach(r => {
    const total = r.lapLogs.reduce((a, b) => a + b, 0);
    if (total > maxTime) maxTime = total;
  });

  const startWall = performance.now();
  let lastStateKey = '';

  function tick() {
    const elapsed = performance.now() - startWall;

    renderOverlayTimer(elapsed);

    // Cuma render ulang tabel kalau susunan lap-nya BENERAN berubah
    // dibanding frame sebelumnya -- kalau dipanggil di tiap animation
    // frame (~60x/detik) walau datanya sama persis, browser jadi terus
    // bongkar-pasang DOM tabel dan kelihatan seperti berkedip.
    const state = computeReplayStateAt(racers, elapsed);
    const stateKey = state.map(s => s.idDecimal + ':' + s.laps).join(',');

    if (stateKey !== lastStateKey) {
      renderOverlayBoard(state, lapTarget);
      lastStateKey = stateKey;
    }

    if (elapsed < maxTime + 800) {
      requestAnimationFrame(tick);
      return;
    }

    // Timeline selesai -- tampilan terakhir dibiarkan diam di layar,
    // munculkan tombol "Ulangi Replay".
    const restartBtn = document.getElementById('overlayReplayRestart');
    if (restartBtn) restartBtn.style.display = '';
  }

  requestAnimationFrame(tick);
}

// --- Medali posisi 1/2/3 -- dipakai di leaderboard Race, Qualifying, dan
// halaman detail hasil tersimpan, supaya konsisten di semua tempat. ---
function medalIcon(rank) {
  if (rank === 1) return '🥇';
  if (rank === 2) return '🥈';
  if (rank === 3) return '🥉';
  return '';
}

function rankCellHtml(rank) {
  const medal = medalIcon(rank);
  return `<span class="rank-cell">${medal ? `<span class="medal">${medal}</span>` : ''}${rank}</span>`;
}

// Efek kilat biru singkat di baris tabel begitu ada lap baru tercatat --
// dipanggil dari processImportantAnnouncements() (Race) dan renderQualify()
// (Qualifying). Aman dipanggil berkali-kali; `void row.offsetWidth` maksa
// browser reflow supaya animasi restart walau class-nya sama persis
// dengan panggilan sebelumnya (kalau tidak, CSS animation tidak akan
// re-trigger untuk lap kedua dst).
function flashRow(rowId) {
  const row = document.getElementById(rowId);
  if (!row) return;
  row.classList.remove('lap-flash');
  void row.offsetWidth;
  row.classList.add('lap-flash');
}

// --- Terminal (50 pembacaan sensor terakhir) ---
// Device tidak punya RTC, jadi "jam terdeteksi" dihitung dari jam lokal
// BROWSER saat pesan diterima lewat WebSocket (praktis real-time karena
// jaringannya lokal). Entri yang di-load dari snapshot awal (belum pernah
// "live" di browser ini) waktunya ditandai "-" -- lebih jujur daripada
// pura-pura tahu jam pastinya.
//
// Nama driver dicari ulang tiap render (bukan disimpan di entri), supaya
// kalau driver baru saja didaftarkan lewat "Add Driver", entri lama yang
// sebelumnya "Unknown" otomatis ikut kebaruan begitu currentDrivers
// ke-update (renderData memanggil renderTerminal() lagi tiap data baru).
function driverNameForId(idDecimal) {
  const found = currentDrivers.find(d => String(d.idDecimal) === String(idDecimal));
  return found ? found.name : null;
}

function renderTerminal() {
  let rowsHtml = '';

  if (terminalEntries.length === 0) {
    rowsHtml = '<div class="terminal-empty">No transponder detected yet</div>';
  } else {
    terminalEntries.forEach(e => {
      const name = driverNameForId(e.idDecimal);
      const nameHtml = name
        ? `<span style="color:#22c55e;">${name}</span>`
        : `<span style="color:#facc15;">Unknown</span>`;

      rowsHtml += `<div class="terminal-row">
        <span>${e.time}</span>
        <span>${e.idDecimal}</span>
        ${nameHtml}
        <span>${e.hits}</span>
        <span>${e.quality}%</span>
      </div>`;
    });
  }

  const header = `<div class="terminal-row terminal-header">
    <span>Time</span><span>ID</span><span>Name</span><span>Hits</span><span>Quality</span>
  </div>`;

  const html = header + rowsHtml;

  const raceEl = document.getElementById('raceTerminalBody');
  if (raceEl) raceEl.innerHTML = html;

  const qualEl = document.getElementById('qualifyTerminalBody');
  if (qualEl) qualEl.innerHTML = html;
}

function addTerminalEntry(idDecimal, hits, quality) {
  const time = new Date().toLocaleTimeString('id-ID', { hour12: false });

  terminalEntries.unshift({ time, idDecimal, hits, quality });

  if (terminalEntries.length > 50) {
    terminalEntries.length = 50;
  }

  renderTerminal();
  updateDriverLastIdCheck(idDecimal);
}

// Cuma dipanggil sekali di load pertama (lihat renderData) -- supaya
// tidak menimpa entri live yang sudah punya jam asli dengan snapshot
// server yang jamnya ditandai "-".
function setTerminalEntries(list) {
  if (!list) return;

  terminalEntries = list.map(e => ({
    time: '-',
    idDecimal: e.idDecimal,
    hits: e.hits,
    quality: e.quality
  }));

  renderTerminal();

  if (terminalEntries.length > 0) {
    updateDriverLastIdCheck(terminalEntries[0].idDecimal);
  }
}

// --- Unlock audio untuk mobile ---
// Browser mobile (terutama iOS Safari, juga Chrome Android) memblokir
// audio.play() dan speechSynthesis.speak() kalau tidak dipicu LANGSUNG
// dari user gesture (tap/klik). Karena suara di app ini dipicu dari pesan
// WebSocket (bukan tap langsung), browser mobile diam-diam blokir semua
// suara. Solusinya: fungsi ini dipanggil di dalam tombol Start (dan
// dimana pun user tap duluan, lihat listener di bawah) -- dan di dalam
// gesture itu kita "unlock" AUDIO OBJECT YANG SAMA (hornAudio) yang bakal
// dipakai lagi nanti dari WebSocket, plus speech synthesis, supaya
// pemanggilan berikutnya (tanpa gesture) tetap diizinkan browser.
function unlockAudio() {
  if (audioUnlocked) return;
  audioUnlocked = true;

  try {
    hornAudio.src = '/start_horn.mp3';
    hornAudio.volume = 0;
    hornAudio.play().then(() => {
      hornAudio.pause();
      hornAudio.currentTime = 0;
      hornAudio.volume = 1;
    }).catch(() => {});
  } catch (e) {}

  try {
    if ('speechSynthesis' in window) {
      const u = new SpeechSynthesisUtterance(' ');
      u.volume = 0;
      speechSynthesis.speak(u);
    }
  } catch (e) {}
}

// Safety net: tap DIMANAPUN di halaman dihitung sebagai gesture yang sah
// buat unlock audio -- jadi walau user tidak sengaja pencet Start duluan
// (misal buka halaman Driver dulu), tap pertama di manapun tetap
// meng-unlock suara untuk sisa sesi.
document.addEventListener('click', unlockAudio, { once: true });

function loadData() {
  fetch('/data')
    .then(response => response.json())
    .then(renderData);
}

// Dipanggil tiap heartbeat 200ms (payload ringan, tanpa lapLogs/nama).
// Cuma update elemen yang benar-benar butuh terasa "hidup" tiap saat: jam
// race, countdown, ID terakhir, dan ringkasan angka per-racer. Detail lap
// log & pengumuman suara ditangani broadcast LENGKAP yang tetap dikirim
// instan tiap ada lap beneran tercatat (lihat addLap()/addQualifyLap()),
// jadi tidak ada yang "telat" -- cuma dipisah biar heartbeat-nya ringan.
function renderLiveData(data) {
  document.getElementById('raceStatus').innerHTML = data.raceState;
  document.getElementById('raceTimer').innerHTML = formatRaceClock(data.raceTime);
  renderOverlayTimer(data.raceTime);
  setStatusStyle(data.raceState);
  updateRaceButtons(data.raceState);

  if (lastRaceState !== data.raceState) {
    if (data.raceState === "RUNNING") {
      playRaceStartSound();
    }
    lastRaceState = data.raceState;
  }

  if (data.raceState === "COUNTDOWN") {
    let text = data.countdown > 0 ? data.countdown : "GO";
    document.getElementById('countdownOverlay').style.display = "flex";
    document.getElementById('countdownNumber').innerHTML = text;

    if (lastCountdownVoice !== data.countdown) {
      playCountdownVoice(data.countdown);
      lastCountdownVoice = data.countdown;
    }
  } else {
    lastCountdownVoice = null;
    document.getElementById('countdownOverlay').style.display = "none";
  }

  if (data.racersLive) {
    data.racersLive.forEach(r => {
      const lapCell = document.getElementById('racer-laps-' + r.idDecimal);
      if (lapCell) {
        lapCell.innerHTML = (r.laps >= data.lapTarget)
          ? `<span class="finish">${r.laps} / FINISH</span>`
          : r.laps;
      }

      const bestCell = document.getElementById('racer-best-' + r.idDecimal);
      if (bestCell) bestCell.innerHTML = formatMs(r.bestLapTime);

      const lastCell = document.getElementById('racer-last-' + r.idDecimal);
      if (lastCell) lastCell.innerHTML = formatMs(r.lastLapTime);

      const avgCell = document.getElementById('racer-avg-' + r.idDecimal);
      if (avgCell) avgCell.innerHTML = formatMs(r.averageLapTime);

      const statusCell = document.getElementById('racer-status-' + r.idDecimal);
      if (statusCell) {
        statusCell.innerHTML = translateStatus(r.status);
        statusCell.className = racerStatusClass(r.status);
      }
    });
  }
}

function renderData(data) {
      // Cuma populate dari snapshot server kalau browser ini belum pernah
      // dapat entri live sama sekali -- biar tidak menimpa entri yang
      // sudah punya jam asli dengan snapshot yang jamnya ditandai "-".
      if (terminalEntries.length === 0 && data.terminalLog) {
        setTerminalEntries(data.terminalLog);
      }

      if (data.firmwareVersion) {
        const vb = document.getElementById('fwVersionBadge');
        if (vb) vb.innerHTML = data.firmwareVersion;

        const vs = document.getElementById('settingFwVersion');
        if (vs) vs.innerHTML = data.firmwareVersion;
      }

      document.getElementById('targetView').innerHTML = data.lapTarget;
      applyCountdownSettingsFromData(data);
      document.getElementById('cooldownView').innerHTML = data.minLapInterval;

      document.getElementById('raceStatus').innerHTML = data.raceState;
      document.getElementById('raceTimer').innerHTML = formatRaceClock(data.raceTime);
      renderOverlay(data);
      setStatusStyle(data.raceState);
      updateRaceButtons(data.raceState);
      if (lastRaceState !== data.raceState) {
  if (data.raceState === "RUNNING") {
    playRaceStartSound();
  }

  lastRaceState = data.raceState;
}

      if (data.raceState === "COUNTDOWN") {
      let text = data.countdown > 0 ? data.countdown : "GO";
      document.getElementById('countdownOverlay').style.display = "flex";
      document.getElementById('countdownNumber').innerHTML = text;

      if (lastCountdownVoice !== data.countdown) {
      playCountdownVoice(data.countdown);
      lastCountdownVoice = data.countdown;
  }

} else {
  lastCountdownVoice = null;
        document.getElementById('countdownOverlay').style.display = "none";
      }

      if (firstLoad) {
        document.getElementById('lapTarget').value = data.lapTarget;
        document.getElementById('cooldown').value = data.minLapInterval;
        firstLoad = false;
      }

      if (data.wifiSsid) {
        document.getElementById('wifiSsidDisplay').innerHTML = 'SSID: ' + data.wifiSsid;
      }

      let rows = '';

      data.racers.forEach((racer, index) => {
        let rankClass = index === 0 ? 'rank1' : '';
        let lapText = racer.laps;

        if (racer.laps >= data.lapTarget) {
          lapText = `<span class="finish">${racer.laps} / FINISH</span>`;
        }

        let lapLogHtml = "";

if (racer.lapLogs && racer.lapLogs.length > 0) {
  racer.lapLogs.forEach((lapTime, lapIndex) => {
    lapLogHtml += `
      <span style="
        display:inline-block;
        background:#020617;
        border:1px solid rgba(56,189,248,0.35);
        border-radius:10px;
        padding:8px 12px;
        margin:4px;
        color:#e2e8f0;
      ">
        Lap ${lapIndex + 1}: <b style="color:#22c55e">${formatMs(lapTime)}</b>
      </span>
    `;
  });
} else {
  lapLogHtml = `<span style="color:#94a3b8">No lap recorded yet</span>`;
}

rows += `
  <tr id="racer-row-${racer.idDecimal}" class="leaderboard-row">
    <td class="${rankClass}">${rankCellHtml(index + 1)}</td>
    <td>${racer.name}</td>
    <td>${racer.idDecimal}</td>
    <td id="racer-laps-${racer.idDecimal}">${lapText}</td>
    <td id="racer-best-${racer.idDecimal}">${formatMs(racer.bestLapTime)}</td>
    <td id="racer-last-${racer.idDecimal}">${formatMs(racer.lastLapTime)}</td>
    <td id="racer-avg-${racer.idDecimal}">${formatMs(racer.averageLapTime)}</td>
    <td id="racer-status-${racer.idDecimal}" class="${racerStatusClass(racer.status)}">${translateStatus(racer.status)}</td>
  </tr>

  <tr>
    <td colspan="8" style="text-align:left; background:rgba(2,6,23,0.45);">
      <div style="margin-top:8px;" id="racer-laplogs-${racer.idDecimal}">
        ${indicatorStatsHtml(racer, data.racers[0], raceGapToLeaderText, racer.lapLogs)}
        ${lapLogHtml}
      </div>
    </td>
  </tr>
`;
      });

      renderPositionChart('racePositionChart', data.racers);

      document.getElementById('leaderboardBody').innerHTML = rows;
      processImportantAnnouncements(data);

      let driverRows = '';

      currentDrivers = data.drivers;

      renderDriverList(document.getElementById('driverSearchInput')
        ? document.getElementById('driverSearchInput').value
        : '');

      renderQualify(data);
}

// Render tabel driver, opsional difilter berdasarkan nama (dipakai fitur
// search). Dipisah dari renderData supaya bisa dipanggil ulang cuma buat
// filter tanpa nunggu data baru dari server.
function renderDriverList(filterText = '') {
  const keyword = (filterText || '').trim().toLowerCase();

  let driverRows = '';

  currentDrivers.forEach((driver) => {
    if (keyword && !driver.name.toLowerCase().includes(keyword)) return;

    // Cari index asli di currentDrivers (bukan index hasil filter) supaya
    // toggle/delete tetap kena driver yang benar.
    const realIndex = currentDrivers.indexOf(driver);

    driverRows += `
      <tr>
        <td>${driver.name}</td>
        <td>${driver.idDecimal}</td>
        <td>
          <input class="btn-check" type="checkbox" ${driver.active ? "checked" : ""}
          onchange="toggleDriver(${realIndex})">
        </td>
        <td>
          <button class="btn-delete" onclick="deleteDriver(${realIndex})">Delete</button>
        </td>
      </tr>
    `;
  });

  if (currentDrivers.length > 0 && driverRows === '') {
    driverRows = `<tr><td colspan="4" style="color:#94a3b8;">Tidak ada driver dengan nama itu.</td></tr>`;
  }

  document.getElementById('driverBody').innerHTML = driverRows;
}

function filterDriverList() {
  const keyword = document.getElementById('driverSearchInput').value;
  renderDriverList(keyword);
}

// --- Last Detected ID (otomatis, dipicu tiap ada pembacaan sensor baru) ---
// Dilakukan client-side (bukan hit endpoint baru) karena browser sudah
// punya daftar driver lengkap ter-sinkron real-time lewat WebSocket --
// jadi hasilnya instan tanpa perlu round-trip ke device. Dipanggil dari
// addTerminalEntry()/setTerminalEntries() -- otomatis ikut ID terakhir
// dari Race ATAU Qualifying, tidak peduli lagi di halaman mana.
function updateDriverLastIdCheck(idDecimal) {
  const resultEl = document.getElementById('driverLastIdResult');
  if (!resultEl) return;

  const found = currentDrivers.find(d => String(d.idDecimal) === String(idDecimal));

  if (found) {
    resultEl.innerHTML = `
      <div class="info" style="border-color:rgba(34,197,94,0.4);">
        <div class="info-label" style="color:#22c55e;">Terdaftar</div>
        <div class="info-value" style="font-size:22px;">${found.name}</div>
        <div style="color:#94a3b8;font-size:13px;margin-top:4px;">ID: ${idDecimal}</div>
      </div>
    `;
    return;
  }

  resultEl.innerHTML = `
    <div class="info" style="border-color:rgba(250,204,21,0.4);">
      <div class="info-label" style="color:#facc15;">Unknown</div>
      <div style="color:#94a3b8;font-size:13px;margin:6px 0 12px 0;">
        Transponder ID ${idDecimal} belum terdaftar.
      </div>

      <div class="driver-form" style="grid-template-columns: 1fr 120px;margin-bottom:0;">
        <div>
          <label id="lblQuickAddName">Nama Driver</label>
          <input type="text" id="quickAddName" placeholder="Isi nama untuk daftarkan">
        </div>
        <button class="btn-save" onclick="quickAddTransponder('${idDecimal}')">Add</button>
      </div>
    </div>
  `;
}

// Add cepat dari Last Detected ID -- ID sudah otomatis kepakai, user
// tinggal isi nama lalu klik Add, tanpa perlu pindah ke form Driver
// Registration dan ketik ulang ID-nya.
function quickAddTransponder(idValue) {
  const nameInput = document.getElementById('quickAddName');
  const name = nameInput.value.trim();

  if (!name) {
    alert('Isi nama driver dulu');
    return;
  }

  fetch(`/addDriver?name=${encodeURIComponent(name)}&id=${idValue}`)
    .then(() => {
      document.getElementById('driverLastIdResult').innerHTML =
        `<div class="info" style="border-color:rgba(34,197,94,0.4);">
          <div class="info-label" style="color:#22c55e;">Berhasil ditambahkan</div>
          <div class="info-value" style="font-size:22px;">${name}</div>
        </div>`;
      loadData();
    });
}

function saveSettings() {
  let target = document.getElementById('lapTarget').value;
  let cooldownSecond = document.getElementById('cooldown').value;

  fetch(`/settings?target=${target}&cooldown=${cooldownSecond}`)
    .then(() => loadData());
}

function saveDriver() {
  let name = document.getElementById('driverName').value;
  let id = document.getElementById('driverId').value;

  if (!name || !id) return;

  fetch(`/addDriver?name=${encodeURIComponent(name)}&id=${id}`)
    .then(() => {
      document.getElementById('driverName').value = "";
      document.getElementById('driverId').value = "";
      loadData();
    });
}

function toggleDriver(index) {
  fetch(`/toggleDriver?index=${index}`)
    .then(() => loadData());
}

function deleteDriver(index) {
  fetch(`/deleteDriver?index=${index}`)
    .then(() => loadData());
}

function startRace() {
  // Dipanggil di sini (bukan cuma banner terpisah) supaya suara otomatis
  // ter-unlock begitu user pencet Start -- ini genuine user gesture jadi
  // browser HP mengizinkan audio.play()/speechSynthesis setelah ini.
  unlockAudio();

  previousRacers = {};
  lastLapAnnounced = {};
  finishAnnounced = {};
  raceFinishAnnounced = false;
  lastCountdownVoice = null;
  bestLapRecord = {}; // fix: dulu tidak pernah di-reset, jadi "best lap"
                       // race baru dibandingkan ke rekor race lama

  fetch('/start')
    .then(res => res.text())
    .then(text => {
      if (text === 'QUALIFY_RUNNING') {
        showToast('Tidak bisa Start Race -- Qualifying sedang berjalan. Stop Qualifying dulu.');
      }
      loadData();
    });
}

function stopRace() {
  fetch('/stop').then(() => loadData());
}

function resetRace() {

  document
    .getElementById(
      'saveRaceModal'
    )
    .style.display = 'flex';
}

function closeSaveModal() {

  document

    .getElementById(

      'saveRaceModal'

    )

    .style.display = 'none';

}

function updateRaceButtons(state) {
  document.getElementById('startBtn').style.display = 'none';
  document.getElementById('stopBtn').style.display = 'none';
  document.getElementById('resetBtn').style.display = 'none';

  if (state === "IDLE") {
    document.getElementById('startBtn').style.display = '';
  }

  if (state === "COUNTDOWN" || state === "RUNNING") {
    document.getElementById('stopBtn').style.display = '';
  }

  if (state === "STOPPED") {
    document.getElementById('resetBtn').style.display = '';
  }
}

// Sama seperti updateRaceButtons() -- Start cuma muncul kalau lagi
// tidak jalan, Stop cuma muncul kalau lagi jalan, Reset muncul kalau
// tidak jalan (biar operator bisa bersihkan leaderboard sebelum sesi
// berikutnya).
function updateQualifyButtons(isRunning) {
  const startBtn = document.getElementById('qualifyStartBtn');
  const stopBtn = document.getElementById('qualifyStopBtn');
  const resetBtn = document.getElementById('qualifyResetBtn');
  if (!startBtn || !stopBtn || !resetBtn) return;

  startBtn.style.display = isRunning ? 'none' : '';
  stopBtn.style.display = isRunning ? '' : 'none';
  resetBtn.style.display = isRunning ? 'none' : '';
}

function saveRaceResult() {
  const raceName = document.getElementById('raceNameInput').value;

  if (!raceName) return;

  const dateTime = new Date().toLocaleString();

  fetch(
    '/saveRace?name=' +
    encodeURIComponent(raceName) +
    '&datetime=' +
    encodeURIComponent(dateTime)
  )
  .then(() => {
    closeSaveModal();

    fetch('/reset')
      .then(() => {
        document.getElementById('raceNameInput').value = "";
        // Fix: sebelumnya kelewat di jalur "Save" (baru ada di
        // startRace()/discardRace()) -- bestLapRecord dari race yang baru
        // disimpan ini harus dikosongkan juga, biar race berikutnya tidak
        // dibandingkan ke rekor race yang sudah lewat.
        previousRacers = {};
        lastLapAnnounced = {};
        finishAnnounced = {};
        raceFinishAnnounced = false;
        bestLapRecord = {};
        loadData();
        loadHistory();
      });
  });
}

function loadHistory() {
  document.getElementById('raceDetailBox').style.display = 'none';
  document.getElementById('historyList').style.display = '';

  fetch('/history')
    .then(response => response.json())
    .then(data => {
      let html = '';

      data.history.forEach((race) => {
        html += `
          <div
            onclick="openRaceDetail('${encodeURIComponent(race.file)}')"
            style="
              background:#020617;
              border:1px solid rgba(56,189,248,0.35);
              border-radius:14px;
              padding:16px;
              margin-bottom:12px;
              cursor:pointer;
            ">
            <div style="font-size:20px;font-weight:900;color:#38bdf8;">
              ${race.name}
            </div>

            <div style="color:#94a3b8;margin-top:6px;">
              ${race.datetime}
            </div>
          </div>
        `;
      });

      if (html === '') {
        html = `<div style="color:#94a3b8;">No race result saved yet</div>`;
      }

      document.getElementById('historyList').innerHTML = html;
    });
}

function discardRace() {
  closeSaveModal();

  previousRacers = {};
  lastLapAnnounced = {};
  finishAnnounced = {};
  raceFinishAnnounced = false;
  bestLapRecord = {};

  fetch('/reset')
    .then(() => loadData());
}

function openRaceDetail(file) {

  fetch('/raceDetail?file=' + decodeURIComponent(file))
    .then(response => response.json())
    .then(data => {

      currentRaceDetailData = data;

      document.getElementById('historyList').style.display = 'none';
      document.getElementById('raceDetailBox').style.display = '';

      let html = `
        <h2>${data.raceName}</h2>

        <div style="
          color:#94a3b8;
          margin-bottom:15px;
        ">
          ${data.startDateTime}
        </div>

        <button class="btn-save" onclick="downloadRaceCsv()" style="margin-bottom:15px;">
          Download CSV
        </button>

        <button class="btn-settings" onclick="openReplayOverlay('${decodeURIComponent(file).replace(/'/g, "\\'")}')" style="margin-bottom:15px;margin-left:8px;">
          🖥️ Buka Overlay Replay (Green Screen)
        </button>

        <table>
          <thead>
            <tr>
              <th>${UI_TEXT[getUILanguage()].thRaceRank}</th>
              <th>${UI_TEXT[getUILanguage()].thRaceDriver}</th>
              <th>${UI_TEXT[getUILanguage()].thRaceLap}</th>
              <th>${UI_TEXT[getUILanguage()].thRaceBest}</th>
              <th>${UI_TEXT[getUILanguage()].thRaceAvg}</th>
              <th>${UI_TEXT[getUILanguage()].thRaceStatus}</th>
            </tr>
          </thead>
          <tbody>
      `;

      data.racers.forEach(r => {

        html += `
          <tr>
            <td>${rankCellHtml(r.rank)}</td>
            <td>${r.name}</td>
            <td>${r.laps}</td>
            <td>${formatMs(r.bestLapTime)}</td>
            <td>${formatMs(r.averageLapTime)}</td>
            <td>${translateStatus(r.status)}</td>
          </tr>
        `;

        if (r.lapLogs && r.lapLogs.length) {

          html += `
            <tr>
              <td colspan="6"
                  style="text-align:left;">
                ${indicatorStatsHtml(r, data.racers[0], raceGapToLeaderText, r.lapLogs)}
          `;

          r.lapLogs.forEach((lap,i) => {

            html += `
              <span class="lap-chip">
                Lap ${i+1} :
                ${formatMs(lap)}
              </span>
            `;

          });

          html += `
              </td>
            </tr>
          `;
        }

      });

      html += `
          </tbody>
        </table>

        <h2 style="margin-top:22px;">Grafik Posisi per Lap</h2>
        <div id="raceDetailPositionChart"></div>
      `;

      document.getElementById(
        'raceDetailContent'
      ).innerHTML = html;

      renderPositionChart('raceDetailPositionChart', data.racers);

    });
}

function closeRaceDetail() {
  document.getElementById('raceDetailBox').style.display = 'none';
  document.getElementById('historyList').style.display = '';
}

function uploadHornSound() {
  const input = document.getElementById('hornFile');

  if (!input.files || input.files.length === 0) {
    alert('Pilih file MP3 dulu');
    return;
  }

  const file = input.files[0];

  if (!file.name.toLowerCase().endsWith('.mp3')) {
    alert('File harus MP3');
    return;
  }

  const formData = new FormData();
  formData.append('horn', file);

  document.getElementById('hornStatus').innerHTML = 'Uploading...';

  fetch('/uploadHorn', {
    method: 'POST',
    body: formData
  })
  .then(response => response.text())
  .then(text => {
    document.getElementById('hornStatus').innerHTML = text;
  });
}

function testHornSound() {
  playHorn();
}

function changeWifiPassword() {
  const input = document.getElementById('wifiPasswordInput');
  const password = input.value;
  const statusEl = document.getElementById('wifiPasswordStatus');

  if (!password || password.length < 8) {
    statusEl.style.color = '#f87171';
    statusEl.innerHTML = 'Password minimal 8 karakter.';
    return;
  }

  if (!confirm('Device akan restart dan semua koneksi WiFi terputus sementara. Lanjutkan?')) {
    return;
  }

  statusEl.style.color = '#94a3b8';
  statusEl.innerHTML = 'Menyimpan & restart device...';

  fetch('/wifiPassword', {
    method: 'POST',
    headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
    body: 'password=' + encodeURIComponent(password)
  })
  .then(response => response.text())
  .then(text => {
    if (text === 'PASSWORD_CHANGED_REBOOTING') {
      statusEl.style.color = '#4ade80';
      statusEl.innerHTML = 'Password diganti. Device restart -- reconnect ke WiFi pakai password baru.';
      input.value = '';
    } else {
      statusEl.style.color = '#f87171';
      statusEl.innerHTML = 'Gagal: ' + text;
    }
  })
  .catch(() => {
    // Device kemungkinan sudah restart duluan sebelum sempat balas -- ini normal.
    statusEl.style.color = '#4ade80';
    statusEl.innerHTML = 'Password diganti. Device sedang restart -- reconnect ke WiFi pakai password baru.';
  });
}

function uploadOta() {
  const input = document.getElementById('otaFile');
  const statusEl = document.getElementById('otaStatus');
  const progressWrap = document.getElementById('otaProgressWrap');
  const progressBar = document.getElementById('otaProgressBar');

  if (!input.files || input.files.length === 0) {
    alert('Pilih file firmware .bin dulu');
    return;
  }

  const file = input.files[0];

  if (!file.name.toLowerCase().endsWith('.bin')) {
    alert('File harus .bin');
    return;
  }

  if (!confirm('Update firmware akan me-restart device setelah selesai. Pastikan file .bin benar untuk board ini. Lanjutkan?')) {
    return;
  }

  const formData = new FormData();
  formData.append('firmware', file);

  progressWrap.style.display = 'block';
  progressBar.style.width = '0%';
  statusEl.style.color = '#94a3b8';
  statusEl.innerHTML = 'Uploading...';

  const xhr = new XMLHttpRequest();

  xhr.upload.addEventListener('progress', (e) => {
    if (e.lengthComputable) {
      const pct = Math.round((e.loaded / e.total) * 100);
      progressBar.style.width = pct + '%';
      statusEl.innerHTML = 'Uploading... ' + pct + '%';
    }
  });

  xhr.onload = () => {
    if (xhr.responseText === 'OTA_OK_REBOOTING') {
      progressBar.style.width = '100%';
      statusEl.style.color = '#4ade80';
      statusEl.innerHTML = 'Update berhasil. Device sedang restart...';
    } else {
      statusEl.style.color = '#f87171';
      statusEl.innerHTML = 'Update gagal: ' + xhr.responseText;
    }
  };

  xhr.onerror = () => {
    // Device mungkin sudah restart sebelum sempat balas response -- kalau
    // progress sempat sampai 100%, anggap sukses.
    if (parseInt(progressBar.style.width) >= 100) {
      statusEl.style.color = '#4ade80';
      statusEl.innerHTML = 'Update kemungkinan berhasil. Device sedang restart...';
    } else {
      statusEl.style.color = '#f87171';
      statusEl.innerHTML = 'Upload gagal, coba lagi.';
    }
  };

  xhr.open('POST', '/otaUpdate');
  xhr.send(formData);
}

function factoryReset() {
  const statusEl = document.getElementById('factoryResetStatus');

  if (!confirm('Ini akan MENGHAPUS SEMUA DATA (driver, history race, history qualifying, setting, password WiFi) dan tidak bisa dibatalkan. Lanjutkan?')) {
    return;
  }

  if (!confirm('Konfirmasi sekali lagi: benar-benar reset device ke kondisi pabrik?')) {
    return;
  }

  statusEl.style.color = '#94a3b8';
  statusEl.innerHTML = 'Menghapus data & restart device...';

  fetch('/factoryReset', {
    method: 'POST',
    headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
    body: 'confirm=YES'
  })
  .then(response => response.text())
  .then(text => {
    statusEl.style.color = '#4ade80';
    statusEl.innerHTML = 'Device sedang di-reset & restart. Halaman ini tidak akan update lagi -- muat ulang browser setelah device menyala kembali.';
  })
  .catch(() => {
    statusEl.style.color = '#4ade80';
    statusEl.innerHTML = 'Device sedang di-reset & restart. Muat ulang browser setelah device menyala kembali.';
  });
}

function playRaceStartSound() {
  if (document.getElementById('annHorn').checked) {
    playHorn(() => {
      if (document.getElementById('annRaceStart').checked) {
        speakAnnouncement('raceStart');
      }
    });

    return;
  }

  if (document.getElementById('annRaceStart').checked) {
    speakAnnouncement('raceStart');
  }
}

// =============================================================
// Bahasa UI vs Bahasa Pengumuman -- SENGAJA DIPISAH (v4.7)
// =============================================================
// Dulu satu radio "voiceLang" ngendaliin DUA hal sekaligus (bahasa
// tampilan UI DAN bahasa yang dibacakan suara) -- sekarang keduanya
// independen, masing-masing punya pilihan (select box) & penyimpanan
// sendiri, supaya mis. UI bisa Indonesia tapi pengumuman tetap Inggris
// (atau sebaliknya) kalau operatornya begitu.
function getUILanguage() {
  try {
    const saved = localStorage.getItem('yurlaps_ui_lang');
    if (saved === 'id' || saved === 'en') return saved;
  } catch (e) {}

  const sel = document.getElementById('uiLangSelect');
  return (sel && sel.value) || 'id';
}

function setUILanguage(lang) {
  try { localStorage.setItem('yurlaps_ui_lang', lang); } catch (e) {}

  const sel1 = document.getElementById('uiLangSelect');
  const sel2 = document.getElementById('headerUiLangSelect');
  if (sel1) sel1.value = lang;
  if (sel2) sel2.value = lang;

  applyUILanguage();
}

function getAnnounceLanguage() {
  try {
    const saved = localStorage.getItem('yurlaps_announce_lang');
    if (saved === 'id' || saved === 'en') return saved;
  } catch (e) {}

  const sel = document.getElementById('announceLangSelect');
  return (sel && sel.value) || 'id';
}

function setAnnounceLanguage(lang) {
  try { localStorage.setItem('yurlaps_announce_lang', lang); } catch (e) {}

  const sel = document.getElementById('announceLangSelect');
  if (sel) sel.value = lang;

  // Kalimat announcement & hint placeholder-nya ikut bahasa pengumuman,
  // BUKAN bahasa UI -- jadi di-refresh di sini, bukan di setUILanguage().
  refreshAnnouncementTemplateInputs();
}

// --- Kecepatan & jenis suara TTS ---
function getVoiceRate() {
  try {
    const saved = parseFloat(localStorage.getItem('yurlaps_voice_rate'));
    if (saved && saved > 0) return saved;
  } catch (e) {}
  return 1;
}

function setVoiceRate(rate) {
  try { localStorage.setItem('yurlaps_voice_rate', rate); } catch (e) {}
}

function getVoiceGenderPref() {
  try {
    const saved = localStorage.getItem('yurlaps_voice_gender');
    if (saved === 'female' || saved === 'male' || saved === 'auto') return saved;
  } catch (e) {}
  return 'auto';
}

function setVoiceGender(gender) {
  try { localStorage.setItem('yurlaps_voice_gender', gender); } catch (e) {}
}

// Web Speech API tidak punya properti "gender" resmi di objek voice --
// jadi ditebak dari NAMA suara yang tersedia di perangkat/browser
// (bervariasi banget antar OS/browser, ini best-effort). Kalau tidak
// ketemu yang cocok, jatuh ke suara pertama yang bahasanya cocok, atau
// null (biar browser pakai default) kalau memang tidak ada satupun.
const VOICE_FEMALE_HINTS = ['female', 'woman', 'perempuan', 'wanita', 'zira', 'samantha', 'victoria', 'susan', 'karen', 'moira', 'tessa', 'fiona', 'siri'];
const VOICE_MALE_HINTS = ['male', 'man', 'pria', 'laki', 'david', 'daniel', 'alex', 'fred', 'tom', 'george', 'james'];

function getVoicesForLang(lang) {
  const prefix = lang === 'id' ? 'id' : 'en';
  const all = (typeof speechSynthesis !== 'undefined' && speechSynthesis.getVoices) ? speechSynthesis.getVoices() : [];
  return all.filter(v => v.lang && v.lang.toLowerCase().startsWith(prefix));
}

function pickVoiceForGender(lang, gender) {
  if (gender === 'auto') return null;

  const voices = getVoicesForLang(lang);
  if (!voices.length) return null;

  const hints = gender === 'female' ? VOICE_FEMALE_HINTS : VOICE_MALE_HINTS;
  const match = voices.find(v => hints.some(h => v.name.toLowerCase().includes(h)));

  return match || voices[0];
}

// Tombol bahasa di header -- alternatif cepat dari select box di
// Settings > Voice Setting (dua-duanya tetap saling sinkron karena
// sama-sama baca/set lewat setUILanguage()).

// --- Terjemahan seluruh UI (bukan cuma suara) ---
// Satu toggle bahasa (radio Indonesian/English di Settings) sekarang
// ngendaliin DUA hal: suara (getVoiceText, sudah ada sebelumnya) DAN
// semua tulisan statis di UI (label, tombol, header tabel, status badge)
// KECUALI nama driver dan brand "YurLaps" -- itu selalu apa adanya.
const UI_TEXT = {
  id: {
    // Nav
    tabRace: 'Balapan', tabQualify: 'Kualifikasi', tabDriver: 'Pembalap',
    tabResult: 'Hasil Balapan', tabSetting: 'Pengaturan',

    // Headers
    h2LeaderboardRace: 'Klasemen', h2TerminalRace: 'Terminal',
    h2LeaderboardQualify: 'Klasemen Kualifikasi', h2TerminalQualify: 'Terminal',
    h2DriverReg: 'Pendaftaran Pembalap', h2LastDetectedId: 'ID Terakhir Terdeteksi',
    h2DriverList: 'Daftar Pembalap', h2RaceResultTitle: 'Hasil Balapan',
    h2VoiceSetting: 'Pengaturan Suara', h2SystemSettings: 'Pengaturan Sistem',
    h2CountdownSetting: 'Countdown Balapan', lblCountdownStart: 'Mulai Hitungan Dari',
    lblHornDelayMode: 'Horn Setelah Hitungan Habis', lblHornFixedDelay: 'Delay Tetap (detik)',
    lblHornRandomRange: 'Rentang Acak (detik)',
    hornDelayHint: 'Setelah angka countdown habis (layar menunjukkan "GO"), horn akan bunyi sesuai pengaturan ini. Mode Acak berguna supaya driver tidak bisa menebak persis kapan start (anti false-start) -- rentangnya diundi ulang setiap kali tombol Start ditekan.',
    h2SaveRaceModal: 'Simpan Hasil Balapan', h2SaveQualifyModal: 'Simpan Hasil Kualifikasi',
    h2SystemDiagnostics: 'Diagnostik Device',
    lblDiagnosticsHint: 'Info memori -- berguna untuk cek apakah device kehabisan memori setelah nyala lama (ini penyebab web UI freeze).',
    lblFreeHeap: 'Sisa Heap', lblMinFreeHeap: 'Heap Terendah Sejak Nyala',
    lblLargestBlock: 'Blok Kosong Terbesar', lblWsClients: 'Client WebSocket',
    lblUptime: 'Lama Nyala', btnRefreshDiagnostics: 'Refresh',

    // Labels
    lblLapTarget: 'Target Lap', lblCooldown: 'Jeda ID / Per ID (Detik)',
    lblMaxLap: 'Maks Lap / Pembalap', lblQualifyCooldown: 'Jeda ID / Detik',
    lblSortPosition: 'Urutkan Posisi', lblDriverName: 'Nama Pembalap',
    lblTransponderId: 'ID Transponder', lblSearchDriver: 'Cari Pembalap',
    lblAnnCountdown: 'Hitung Mundur', lblAnnHorn: 'Klakson',
    lblAnnRaceStart: 'Mulai Balapan', lblAnnLapCount: 'Jumlah Lap',
    lblAnnLastLap: 'Lap Terakhir', lblAnnPosition: 'Perubahan Posisi',
    lblAnnFinishPos: 'Posisi Finish', lblAnnRaceFinish: 'Balapan Selesai',
    lblAnnBestLap: 'Lap Tercepat', lblNewPassword: 'Password Baru (min. 8 karakter)',
    lblAnnouncementsTitle: 'Pengumuman', lblAnnRaceStopped: 'Balapan Dihentikan (ikut toggle "Balapan Selesai" di atas)',
    lblAnnQualifyLapTime: 'Waktu Lap Kualifikasi',
    annModularHint: 'Tiap pengumuman bisa diatur sendiri kalimatnya pakai placeholder seperti {name} atau {position} -- klik "Uji" buat dengar hasilnya kapan saja, tidak perlu race beneran jalan.',
    lblQuickAddName: 'Nama Pembalap', lblVoiceLanguage: 'Bahasa &amp; Suara',
    lblUiLanguage: 'Bahasa Tampilan (UI)', lblAnnounceLanguage: 'Bahasa Pengumuman (Suara)',
    lblVoiceRate: 'Kecepatan Suara', lblVoiceGender: 'Jenis Suara',
    voiceGenderHint: 'Pilihan Wanita/Pria tergantung suara yang tersedia di perangkat/browser -- kalau cuma ada satu suara terpasang, pilihan ini mungkin tidak berpengaruh. Coba tombol "Uji" di salah satu pengumuman di bawah buat dengar hasilnya.',
    lblAnnSectionGeneral: 'Umum', lblAnnSectionPerDriver: 'Per Driver (bisa dipilih Semua / Terdepan / Top 3)',
    lblAnnScopeLabel1: 'Umumkan untuk', lblAnnScopeLabel2: 'Umumkan untuk',
    lblAnnScopeLabel3: 'Umumkan untuk', lblAnnScopeLabel4: 'Umumkan untuk',
    lblAnnScopeLabel5: 'Umumkan untuk', lblAnnScopeLabel6: 'Umumkan untuk',
    lblLangId: 'Indonesia', lblLangEn: 'Inggris',

    // Table headers
    thRaceRank: 'Posisi', thRaceDriver: 'Nama', thRaceId: 'ID', thRaceLap: 'Lap',
    thRaceBest: 'Lap Tercepat', thRaceLast: 'Lap Terakhir', thRaceAvg: 'Rata-rata',
    thRaceStatus: 'Status',
    thQualPos: 'Posisi', thQualDriver: 'Nama', thQualId: 'ID', thQualLap: 'Lap',
    thQualBest: 'Lap Tercepat', thQualLast: 'Lap Terakhir', thQualAvg: 'Rata-rata',
    thQualDelete: 'Hapus',
    thDrvName: 'Nama', thDrvId: 'ID Transponder', thDrvJoin: 'Ikut Balapan',
    thDrvDelete: 'Hapus',

    // Buttons
    btnResultRace: 'Hasil Balapan', btnResultQualify: 'Hasil Kualifikasi',
    btnBackRaceResult: 'Kembali ke Daftar Hasil',
    btnUploadHorn: 'Upload Klakson', btnTestHorn: 'Coba Klakson',
    btnSaveVoiceSetting: 'Simpan Pengaturan Suara',
    btnSaveRace: 'Simpan Balapan', btnDiscardRace: 'Jangan Simpan',
    btnCancelSaveRace: 'Batal',
    btnSaveQualify: 'Simpan Kualifikasi', btnDiscardQualify: 'Jangan Simpan',
    btnCancelSaveQualify: 'Batal',

    // Status badge (racer.status dari server selalu Inggris, ditranslate
    // saat render)
    statusReady: 'Siap', statusInRace: 'Sedang Balapan', statusFinish: 'Selesai',
    statusDNF: 'DNF', statusDNS: 'Tidak Start',
  },
  en: {
    tabRace: 'Race', tabQualify: 'Qualify', tabDriver: 'Driver',
    tabResult: 'Race Result', tabSetting: 'Setting',

    h2LeaderboardRace: 'Leaderboard', h2TerminalRace: 'Terminal',
    h2LeaderboardQualify: 'Qualifying Leaderboard', h2TerminalQualify: 'Terminal',
    h2DriverReg: 'Driver Registration', h2LastDetectedId: 'Last Detected ID',
    h2DriverList: 'Driver List', h2RaceResultTitle: 'Race Result',
    h2VoiceSetting: 'Voice Setting', h2SystemSettings: 'System Settings',
    h2CountdownSetting: 'Race Countdown', lblCountdownStart: 'Start Counting From',
    lblHornDelayMode: 'Horn After Countdown Ends', lblHornFixedDelay: 'Fixed Delay (seconds)',
    lblHornRandomRange: 'Random Range (seconds)',
    hornDelayHint: 'After the countdown number ends (screen shows "GO"), the horn will sound according to this setting. Random mode is useful so drivers can\'t guess exactly when the start happens (anti false-start) -- the range is re-rolled every time Start is pressed.',
    h2SaveRaceModal: 'Save Race Result', h2SaveQualifyModal: 'Save Qualify Result',
    h2SystemDiagnostics: 'Device Diagnostics',
    lblDiagnosticsHint: 'Memory info -- useful to check if the device is running low on memory over long uptime (this is what causes the web UI to freeze).',
    lblFreeHeap: 'Free Heap', lblMinFreeHeap: 'Lowest Free Heap Ever',
    lblLargestBlock: 'Largest Free Block', lblWsClients: 'WebSocket Clients',
    lblUptime: 'Uptime', btnRefreshDiagnostics: 'Refresh',

    lblLapTarget: 'Lap Target', lblCooldown: 'ID Cooldown / Per ID (Second)',
    lblMaxLap: 'Max Lap / Driver', lblQualifyCooldown: 'ID Cooldown / Second',
    lblSortPosition: 'Sort Position', lblDriverName: 'Driver Name',
    lblTransponderId: 'Transponder ID', lblSearchDriver: 'Search Driver',
    lblAnnCountdown: 'Countdown', lblAnnHorn: 'Horn',
    lblAnnRaceStart: 'Race Start', lblAnnLapCount: 'Lap Count',
    lblAnnLastLap: 'Last Lap', lblAnnPosition: 'Position Change',
    lblAnnFinishPos: 'Finish Position', lblAnnRaceFinish: 'Race Finish',
    lblAnnBestLap: 'Best Lap', lblNewPassword: 'New Password (min. 8 characters)',
    lblAnnouncementsTitle: 'Announcements', lblAnnRaceStopped: 'Race Stopped (follows "Race Finish" toggle above)',
    lblAnnQualifyLapTime: 'Qualifying Lap Time',
    annModularHint: 'Each announcement can have its own custom sentence using placeholders like {name} or {position} -- click "Test" to hear the result anytime, no need for an actual race.',
    lblQuickAddName: 'Driver Name', lblVoiceLanguage: 'Language &amp; Voice',
    lblUiLanguage: 'Display Language (UI)', lblAnnounceLanguage: 'Announcement Language (Voice)',
    lblVoiceRate: 'Speech Rate', lblVoiceGender: 'Voice Type',
    voiceGenderHint: 'Female/Male options depend on the voices available on your device/browser -- if only one voice is installed, this choice may have no effect. Try the "Test" button on any announcement below to hear the result.',
    lblAnnSectionGeneral: 'General', lblAnnSectionPerDriver: 'Per Driver (choose All / Leader / Top 3)',
    lblAnnScopeLabel1: 'Announce for', lblAnnScopeLabel2: 'Announce for',
    lblAnnScopeLabel3: 'Announce for', lblAnnScopeLabel4: 'Announce for',
    lblAnnScopeLabel5: 'Announce for', lblAnnScopeLabel6: 'Announce for',
    lblLangId: 'Indonesian', lblLangEn: 'English',

    thRaceRank: 'Rank', thRaceDriver: 'Driver', thRaceId: 'ID', thRaceLap: 'Lap',
    thRaceBest: 'Best Lap', thRaceLast: 'Last Lap', thRaceAvg: 'Average',
    thRaceStatus: 'Status',
    thQualPos: 'Pos', thQualDriver: 'Driver', thQualId: 'ID', thQualLap: 'Lap',
    thQualBest: 'Best Lap', thQualLast: 'Last Lap', thQualAvg: 'Average',
    thQualDelete: 'Delete',
    thDrvName: 'Name', thDrvId: 'Transponder ID', thDrvJoin: 'Join Race',
    thDrvDelete: 'Delete',

    btnResultRace: 'Result Race', btnResultQualify: 'Result Qualify',
    btnBackRaceResult: 'Back to Result List',
    btnUploadHorn: 'Upload Horn', btnTestHorn: 'Test Horn',
    btnSaveVoiceSetting: 'Save Voice Setting',
    btnSaveRace: 'Save Race', btnDiscardRace: "Don't Save",
    btnCancelSaveRace: 'Cancel',
    btnSaveQualify: 'Save Qualify', btnDiscardQualify: "Don't Save",
    btnCancelSaveQualify: 'Cancel',

    statusReady: 'Ready', statusInRace: 'In Race', statusFinish: 'Finish',
    statusDNF: 'DNF', statusDNS: 'DNS',
  }
};

// Terjemahkan status dari server (selalu Inggris: Ready/In Race/Finish/
// DNF/DNS) ke bahasa UI yang aktif. Dipakai di semua tempat yang nampilin
// racer.status / q.status.
function translateStatus(status) {
  const lang = getUILanguage();
  const map = {
    'Ready': 'statusReady', 'In Race': 'statusInRace', 'Finish': 'statusFinish',
    'DNF': 'statusDNF', 'DNS': 'statusDNS'
  };
  const key = map[status];
  return key ? UI_TEXT[lang][key] : status;
}

// v4.7: dulu label lblAnn* membungkus <input> checkbox di dalamnya, jadi
// butuh penanganan khusus (CHECKBOX_LABEL_IDS) supaya textContent tidak
// ikut menghapus checkbox-nya. Sekarang toggle-nya sudah dipisah (ios-
// toggle terpisah dari <span class="ann-card-title">), jadi semua label
// aman ditranslate lewat jalur generik biasa -- tidak perlu penanganan
// khusus lagi.
function applyUILanguage() {
  const lang = getUILanguage();
  const dict = UI_TEXT[lang];

  Object.keys(dict).forEach(key => {
    const el = document.getElementById(key);
    if (el) el.textContent = dict[key];
  });

  // Re-render bagian yang teksnya di-generate lewat JS (bukan HTML statis)
  // supaya status badge & leaderboard ikut kebaruan bahasa instan, tanpa
  // nunggu update data berikutnya.
  loadData();
}

// =============================================================
// Sistem Announcement Modular -- kalimat custom per jenis announcement
// =============================================================
// Tiap jenis announcement (Position, Lap Count, Best Lap, dst) punya
// kalimat DEFAULT bawaan (mengikuti bahasa ID/EN yang aktif), tapi bisa
// ditimpa BEBAS oleh user lewat kotak teks di Settings > Voice Setting,
// pakai placeholder seperti {name}, {position}, {lap}. Placeholder yang
// tidak dikenali/tidak tersedia buat jenis tsb otomatis dikosongkan
// (bukan dibacakan mentah-mentah sebagai teks "{typo}").
//
// `placeholders` di bawah ini cuma dipakai buat teks bantuan & data
// contoh tombol "Uji" -- applyAnnouncementTemplate() sendiri tidak
// membatasi placeholder mana yang "boleh" dipakai di jenis mana (kalau
// user coba pakai placeholder dari jenis lain, cukup akan tampil kosong
// karena datanya memang tidak dikirim ke jenis itu).
const ANNOUNCEMENT_DEFS = {
  raceStart:      { placeholders: [], sample: {}, hasScope: false },
  raceFinish:     { placeholders: [], sample: {}, hasScope: false },
  raceStopped:    { placeholders: [], sample: {}, hasScope: false },
  lapCount:       { placeholders: ['name', 'position', 'lap', 'target'], sample: { name: 'Budi', position: '2', lap: '5', target: '10' }, hasScope: true },
  lastLap:        { placeholders: ['name', 'lap', 'target'], sample: { name: 'Budi', lap: '9', target: '10' }, hasScope: true },
  position:       { placeholders: ['name', 'position'], sample: { name: 'Budi', position: 'dua' }, hasScope: true },
  finishPosition: { placeholders: ['name', 'position', 'lap'], sample: { name: 'Budi', position: 'satu', lap: '10' }, hasScope: true },
  bestLap:        { placeholders: ['name', 'bestlap'], sample: { name: 'Budi', bestlap: '11.980' }, hasScope: true },
  qualifyLapTime: { placeholders: ['name', 'lastlap', 'lap'], sample: { name: 'Budi', lastlap: '11 koma 980', lap: '5' }, hasScope: true }
};

// --- Scope: siapa saja yang diumumkan buat tiap jenis (v4.7) ---
// 'all'    = semua driver, tiap kali kejadiannya terjadi (perilaku lama)
// 'leader' = cuma driver yang LAGI di posisi 1 saat kejadian itu terjadi
// 'top3'   = cuma driver yang lagi di posisi 1-3
// Tidak berlaku buat raceStart/raceFinish/raceStopped -- itu kejadian
// GLOBAL (satu per race), bukan per-driver, jadi tidak ada konsep
// "posisi siapa" buat disaring.
let announcementScopes = {};

function loadAnnouncementScopes() {
  try {
    const raw = localStorage.getItem('yurlaps_ann_scopes');
    if (raw) announcementScopes = JSON.parse(raw);
  } catch (e) {}
  if (!announcementScopes) announcementScopes = {};

  Object.keys(ANNOUNCEMENT_DEFS).forEach(key => {
    const sel = document.getElementById('annScope_' + key);
    if (sel) sel.value = announcementScopes[key] || 'all';
  });
}

function onAnnouncementScopeChange(key) {
  const sel = document.getElementById('annScope_' + key);
  if (!sel) return;

  announcementScopes[key] = sel.value;
  try { localStorage.setItem('yurlaps_ann_scopes', JSON.stringify(announcementScopes)); } catch (e) {}
}

// `position` = posisi/rank driver SAAT kejadian ini terjadi (1 = terdepan).
function passesAnnouncementScope(key, position) {
  const scope = announcementScopes[key] || 'all';
  if (scope === 'leader') return position === 1;
  if (scope === 'top3') return position <= 3;
  return true;
}

// v4.6: "Lap Count" dulu KEBALIK -- checkbox-nya bernama "Lap Count" tapi
// kalimat yang dibacakan sebenarnya isinya POSISI ("Budi posisi 2"), bukan
// jumlah lap ("Budi lap 5"), dan cuma beda kondisi guard dengan Position
// Change (biar tidak dobel bicara di lap yang sama). Sekarang benar-benar
// mengumumkan LAP dengan default template `{name} lap {lap}` -- kalau
// masih mau dengar posisi tiap lap, tinggal ganti templatenya sendiri
// jadi `{name} posisi {position}`.
const ANNOUNCEMENT_DEFAULT_TEMPLATES = {
  id: {
    raceStart: 'Balapan dimulai',
    raceFinish: 'Balapan selesai',
    raceStopped: 'Balapan dihentikan',
    lapCount: '{name} lap {lap}',
    lastLap: 'Lap terakhir untuk {name}',
    position: '{name} sekarang posisi {position}',
    finishPosition: '{name} finis posisi {position}',
    bestLap: 'Best lap baru oleh {name}',
    qualifyLapTime: '{name} waktu lap {lastlap}'
  },
  en: {
    raceStart: 'Race started',
    raceFinish: 'Race finished',
    raceStopped: 'Race stopped',
    lapCount: '{name} lap {lap}',
    lastLap: 'Final lap for {name}',
    position: '{name} is now in {position} position',
    finishPosition: '{name} finishes in {position} position',
    bestLap: 'New best lap by {name}',
    qualifyLapTime: '{name} lap time {lastlap}'
  }
};

// Ditampung di memori & localStorage, terpisah per bahasa (ID/EN) --
// cuma menyimpan kalimat yang MEMANG sudah diubah user; yang belum
// disentuh tetap ambil dari ANNOUNCEMENT_DEFAULT_TEMPLATES di atas.
let announcementTemplates = { id: {}, en: {} };

function loadAnnouncementTemplates() {
  try {
    const raw = localStorage.getItem('yurlaps_ann_templates');
    if (raw) announcementTemplates = JSON.parse(raw);
  } catch (e) {}

  if (!announcementTemplates.id) announcementTemplates.id = {};
  if (!announcementTemplates.en) announcementTemplates.en = {};

  refreshAnnouncementTemplateInputs();
}

function saveAnnouncementTemplatesToStorage() {
  try {
    localStorage.setItem('yurlaps_ann_templates', JSON.stringify(announcementTemplates));
  } catch (e) {}
}

// Ambil kalimat AKTIF buat satu jenis announcement, di bahasa yang lagi
// dipilih -- balik ke default bawaan kalau user belum pernah nulis
// kalimat sendiri (atau kotaknya sengaja dikosongkan).
function getAnnouncementTemplate(key) {
  const lang = getAnnounceLanguage();
  const custom = announcementTemplates[lang] && announcementTemplates[lang][key];
  if (custom && custom.trim()) return custom;
  return (ANNOUNCEMENT_DEFAULT_TEMPLATES[lang] && ANNOUNCEMENT_DEFAULT_TEMPLATES[lang][key]) || '';
}

// Ganti semua {placeholder} di template dengan nilai dari `vars`.
// Placeholder yang tidak ada di `vars` dikosongkan total (bukan
// dibiarkan literal "{xxx}"), lalu spasi ganda hasil pengosongan itu
// dirapikan supaya kalimatnya tidak kedengaran aneh ada jeda kosong.
function applyAnnouncementTemplate(template, vars) {
  if (!template) return '';

  const out = template.replace(/\{(\w+)\}/g, (match, key) => {
    const v = vars[key];
    return (v === undefined || v === null) ? '' : String(v);
  });

  return out.replace(/\s+/g, ' ').trim();
}

// Titik panggil TUNGGAL buat semua announcement berbasis kalimat --
// dulu tersebar sebagai speakText(getVoiceText(...)) atau string manual
// digabung langsung di banyak tempat (termasuk pengecekan bahasa
// berulang-ulang). Sekarang semua lewat sini, otomatis pakai template
// custom user kalau ada.
function speakAnnouncement(key, vars = {}) {
  const template = getAnnouncementTemplate(key);
  if (!template) return;

  speakText(applyAnnouncementTemplate(template, vars));
}

// Dipanggil tiap user mengetik di kotak template -- simpan ke memori +
// localStorage langsung saat itu juga (tidak perlu tombol Save terpisah
// buat bagian ini, biar terasa instan/live). Tombol "Save Voice Setting"
// tetap ada buat checkbox on/off & bahasa.
function onAnnouncementTemplateInput(key) {
  const el = document.getElementById('annTemplate_' + key);
  if (!el) return;

  const lang = getAnnounceLanguage();
  if (!announcementTemplates[lang]) announcementTemplates[lang] = {};
  announcementTemplates[lang][key] = el.value;

  saveAnnouncementTemplatesToStorage();
}

// Tombol "🔊 Uji" -- dengar hasilnya SEKARANG JUGA pakai data contoh,
// tanpa perlu race beneran jalan. Pakai isi kotak teks APA ADANYA (toh
// sudah otomatis kesimpan tiap ketik lewat onAnnouncementTemplateInput).
function testAnnouncementTemplate(key) {
  unlockAudio();

  const el = document.getElementById('annTemplate_' + key);
  const template = (el && el.value.trim()) ? el.value : getAnnouncementTemplate(key);
  const sample = (ANNOUNCEMENT_DEFS[key] && ANNOUNCEMENT_DEFS[key].sample) || {};

  const text = applyAnnouncementTemplate(template, sample);
  speakText(text || (getAnnounceLanguage() === 'id' ? 'Kalimat kosong' : 'Empty sentence'));
}

// Tombol "↺" per baris -- balikin SATU jenis ke kalimat default bawaan
// (mengikuti bahasa aktif), langsung tersimpan & kotak teksnya di-refresh.
function resetAnnouncementTemplate(key) {
  const lang = getAnnounceLanguage();
  if (announcementTemplates[lang]) delete announcementTemplates[lang][key];

  saveAnnouncementTemplatesToStorage();
  refreshAnnouncementTemplateInputs();
}

// Tombol besar "Reset Semua Kalimat" -- balikin SEMUA jenis, KEDUA
// bahasa, ke default bawaan.
function resetAllAnnouncementTemplates() {
  announcementTemplates = { id: {}, en: {} };
  saveAnnouncementTemplatesToStorage();
  refreshAnnouncementTemplateInputs();
}

// Isi ulang semua kotak teks template sesuai bahasa aktif + tampilkan
// hint placeholder yang valid buat tiap jenis -- dipanggil saat load
// awal DAN tiap ganti bahasa (ID/EN), supaya kotak teksnya selalu
// mencerminkan kalimat yang benar-benar aktif dipakai saat itu.
function refreshAnnouncementTemplateInputs() {
  const emptyHint = getAnnounceLanguage() === 'id'
    ? '(tanpa placeholder -- kalimat tetap)'
    : '(no placeholders -- fixed sentence)';

  Object.keys(ANNOUNCEMENT_DEFS).forEach(key => {
    const input = document.getElementById('annTemplate_' + key);
    if (input) input.value = getAnnouncementTemplate(key);

    const hint = document.getElementById('annHint_' + key);
    if (hint) {
      const ph = ANNOUNCEMENT_DEFS[key].placeholders;
      hint.innerHTML = ph.length
        ? 'Placeholder: ' + ph.map(p => `<code>{${p}}</code>`).join(' ')
        : emptyHint;
    }
  });
}

function getVoiceText(key, name = '', value = '') {
  const lang = getAnnounceLanguage();

  const text = {
    id: {
      raceStart: 'Balapan dimulai',
      raceFinish: 'Balapan selesai',
      raceStopped: 'Balapan dihentikan',
      lapCount: `${name} lap ${value}`,
      lastLap: `Lap terakhir untuk ${name}`,
      position: `${name} sekarang posisi ${value}`,
      finish: `${name} finis posisi ${value}`,
      bestLap: `Best lap baru oleh ${name}`
    },
    en: {
      raceStart: 'Race started',
      raceFinish: 'Race finished',
      raceStopped: 'Race stopped',
      lapCount: `${name} lap ${value}`,
      lastLap: `Final lap for ${name}`,
      position: `${name} is now in ${value} position`,
      finish: `${name} finishes in ${value} position`,
      bestLap: `New best lap by ${name}`
    }
  };

  return text[lang][key] || '';
}


function positionText(position) {
  const lang = getAnnounceLanguage();

  if (lang === "id") {
    if (position === 1) return "satu";
    if (position === 2) return "dua";
    if (position === 3) return "tiga";
    return String(position);
  }

  if (position === 1) return "first";
  if (position === 2) return "second";
  if (position === 3) return "third";
  return String(position);
}


function speakText(text) {
  if (!text) return;

  const utterance = new SpeechSynthesisUtterance(text);

  const lang = getAnnounceLanguage();
  utterance.lang = lang === 'id' ? 'id-ID' : 'en-US';
  utterance.rate = getVoiceRate();
  utterance.pitch = 1;

  const voice = pickVoiceForGender(lang, getVoiceGenderPref());
  if (voice) utterance.voice = voice;

  speechSynthesis.speak(utterance);
}

// v4.7: dulu cuma hardcode kata buat angka 1-5 (pas banget sama
// countdown bawaan yang emang 5 detik). Sekarang countdown bisa dimulai
// dari angka berapa saja (lihat pengaturan Countdown Race) -- kata
// Indonesia/Inggris cuma disediakan sampai sepuluh; di atas itu (jarang
// dipakai buat countdown balapan) TTS tetap membacakan angkanya sebagai
// digit ("11", "12", dst), tetap benar didengar walau bukan kata utuh.
const COUNTDOWN_WORDS = {
  id: ['', 'Satu', 'Dua', 'Tiga', 'Empat', 'Lima', 'Enam', 'Tujuh', 'Delapan', 'Sembilan', 'Sepuluh'],
  en: ['', 'One', 'Two', 'Three', 'Four', 'Five', 'Six', 'Seven', 'Eight', 'Nine', 'Ten']
};

function playCountdownVoice(value) {
  if (!document.getElementById('annCountdown').checked) return;

  // angka 0 tidak dibacakan -- nanti horn yang bunyi saat race start
  if (value === 0) return;

  const lang = getAnnounceLanguage();
  const words = COUNTDOWN_WORDS[lang] || COUNTDOWN_WORDS.en;
  const text = words[value] || String(value);

  speakText(text);
}


function processImportantAnnouncements(data) {
  if (data.raceState !== "RUNNING" && data.raceState !== "STOPPED") return;

  let finishedCount = 0;

  data.racers.forEach((racer, index) => {
    const id = racer.idDecimal;
    const old = previousRacers[id];
    const position = index + 1;
    const positionChanged = !!(old && old.position !== position);

    // Efek visual "lap baru" -- lepas dari checkbox pengumuman suara,
    // selalu jalan biar operator lihat langsung siapa yang barusan lewat.
    if (old && racer.laps > old.laps) {
      flashRow('racer-row-' + id);
    }

    if (racer.status === "Finish") {
      finishedCount++;
    }

if (
   document.getElementById('annLapCount').checked &&
   old &&
   racer.laps > old.laps &&
   racer.status !== "Finish" &&
   passesAnnouncementScope('lapCount', position) &&
   // Kalau posisi JUGA berubah di lap yang sama, biarkan annPosition di
   // bawah yang announce (lebih spesifik) -- supaya tidak dobel bicara
   // dua kali beruntun buat kejadian yang sama.
   !(document.getElementById('annPosition').checked && positionChanged)
) {
  // v4.6: dulu di sini kebalik -- checkbox "Lap Count" tapi kalimatnya
  // isi POSISI ("Budi posisi 2"), bukan lap ("Budi lap 5"). Sekarang
  // benar-benar mengumumkan lap sesuai nama fiturnya (default template
  // "{name} lap {lap}"); kalau tetap mau dengar posisi tiap lap, tinggal
  // ganti templatenya sendiri di Settings jadi "{name} posisi {position}".
  speakAnnouncement('lapCount', {
    name: racer.name,
    position: position,
    lap: racer.laps,
    target: data.lapTarget
  });
}

    if (
      document.getElementById('annLastLap').checked &&
      racer.laps === data.lapTarget - 1 &&
      racer.status !== "Finish" &&
      !lastLapAnnounced[id] &&
      passesAnnouncementScope('lastLap', position)
    ) {
      speakAnnouncement('lastLap', { name: racer.name, lap: racer.laps, target: data.lapTarget });
      lastLapAnnounced[id] = true;
    }

    if (
      document.getElementById('annPosition').checked &&
      positionChanged &&
      old &&
      // v4.8 FIX: dulu tidak ada syarat "racer.laps > old.laps" di sini --
      // akibatnya driver yang cuma KEGESER posisinya (gara-gara driver
      // LAIN nyalip, bukan dia sendiri yang nge-lap) ikut diumumkan juga.
      // Sekarang cuma driver yang BENERAN baru saja melintas garis finish
      // (nge-lap) DAN posisinya berubah yang diumumkan -- driver lain yang
      // kena dampak (posisinya ikut turun/naik tanpa berbuat apa-apa)
      // tidak lagi disebut.
      racer.laps > old.laps &&
      racer.status !== "Finish" &&
      passesAnnouncementScope('position', position)
    ) {
      speakAnnouncement('position', { name: racer.name, position: positionText(position) });
    }

    if (
      document.getElementById('annBestLap').checked &&
      racer.bestLapTime > 0 &&
      (!bestLapRecord[id] || racer.bestLapTime < bestLapRecord[id]) &&
      passesAnnouncementScope('bestLap', position)
    ) {
      if (bestLapRecord[id]) {
        speakAnnouncement('bestLap', { name: racer.name, bestlap: formatMs(racer.bestLapTime) });
      }

      bestLapRecord[id] = racer.bestLapTime;
    }

    if (
      document.getElementById('annFinishPosition').checked &&
      racer.status === "Finish" &&
      !finishAnnounced[id] &&
      passesAnnouncementScope('finishPosition', position)
    ) {
      speakAnnouncement('finishPosition', { name: racer.name, position: positionText(position), lap: racer.laps });
      finishAnnounced[id] = true;
    }

    previousRacers[id] = {
      laps: racer.laps,
      status: racer.status,
      position: position,
      bestLapTime: racer.bestLapTime
    };
  });

  if (
    data.raceState === "RUNNING" &&
    data.racers.length > 0 &&
    finishedCount === data.racers.length &&
    document.getElementById('annRaceFinish').checked &&
    !raceFinishAnnounced
  ) {
    speakAnnouncement('raceFinish');
    raceFinishAnnounced = true;
  }

  if (
    data.raceState === "STOPPED" &&
    document.getElementById('annRaceFinish').checked &&
    !raceFinishAnnounced
  ) {
    speakAnnouncement('raceStopped');
    raceFinishAnnounced = true;
  }
}

// --- Race Countdown & Horn Delay (v4.7) ---
function onHornDelayModeChange() {
  const mode = document.getElementById('hornDelayModeSelect').value;
  document.getElementById('hornFixedRow').style.display = mode === 'fixed' ? '' : 'none';
  document.getElementById('hornRandomRow').style.display = mode === 'random' ? '' : 'none';
  saveCountdownSettings();
}

function saveCountdownSettings() {
  const start = document.getElementById('countdownStartInput').value;
  const mode = document.getElementById('hornDelayModeSelect').value;
  const fixedMs = Math.round((parseFloat(document.getElementById('hornFixedInput').value) || 0) * 1000);
  const minMs = Math.round((parseFloat(document.getElementById('hornRandomMinInput').value) || 0) * 1000);
  const maxMs = Math.round((parseFloat(document.getElementById('hornRandomMaxInput').value) || 0) * 1000);

  fetch(`/countdownSettings?start=${start}&mode=${mode}&fixedMs=${fixedMs}&minMs=${minMs}&maxMs=${maxMs}`)
    .then(res => res.text())
    .then(text => {
      if (text === 'INVALID_RANGE') {
        showToast('Rentang random tidak valid -- angka maksimum harus lebih besar dari minimum.');
      } else {
        loadData();
      }
    });
}

// Dipanggil dari renderData() (full state) supaya kotak-kotak di atas
// selalu mencerminkan nilai yang BENERAN aktif di firmware (bukan cuma
// nilai default HTML) -- termasuk setelah reboot device.
function applyCountdownSettingsFromData(data) {
  if (data.countdownStartNumber === undefined) return;

  const startInput = document.getElementById('countdownStartInput');
  const modeSelect = document.getElementById('hornDelayModeSelect');
  const fixedInput = document.getElementById('hornFixedInput');
  const minInput = document.getElementById('hornRandomMinInput');
  const maxInput = document.getElementById('hornRandomMaxInput');

  // Jangan timpa nilai yang lagi DIKETIK user (kalau elemen itu lagi
  // fokus) -- supaya update dari broadcast tidak mengganggu saat operator
  // lagi mengisi angka.
  if (document.activeElement !== startInput) startInput.value = data.countdownStartNumber;
  if (document.activeElement !== modeSelect) modeSelect.value = data.hornRandomDelayEnabled ? 'random' : 'fixed';
  if (document.activeElement !== fixedInput) fixedInput.value = (data.hornDelayFixedMs / 1000);
  if (document.activeElement !== minInput) minInput.value = (data.hornDelayMinMs / 1000);
  if (document.activeElement !== maxInput) maxInput.value = (data.hornDelayMaxMs / 1000);

  document.getElementById('hornFixedRow').style.display = data.hornRandomDelayEnabled ? 'none' : '';
  document.getElementById('hornRandomRow').style.display = data.hornRandomDelayEnabled ? '' : 'none';
}

function saveVoiceSetting() {
  const setting = {
    countdown: document.getElementById('annCountdown').checked,
    horn: document.getElementById('annHorn').checked,
    raceStart: document.getElementById('annRaceStart').checked,
    lapCount: document.getElementById('annLapCount').checked,
    lastLap: document.getElementById('annLastLap').checked,
    position: document.getElementById('annPosition').checked,
    finishPosition: document.getElementById('annFinishPosition').checked,
    raceFinish: document.getElementById('annRaceFinish').checked,
    bestLap: document.getElementById('annBestLap').checked,
    qualifyLapTime: document.getElementById('annQualifyLapTime').checked
  };

  localStorage.setItem('voiceSetting', JSON.stringify(setting));
  alert('Voice setting saved');
}

// v4.7: dulu fungsi ini juga yang "memuat" bahasa (radio input yang
// sekarang sudah tidak ada lagi -- diganti select box). Bahasa UI,
// bahasa pengumuman, kecepatan & jenis suara SEKARANG masing-masing
// tersimpan sendiri (localStorage terpisah, lihat getUILanguage() dkk)
// dan langsung aktif begitu dipilih -- TIDAK perlu tombol Save sama
// sekali buat bagian itu. Fungsi ini sekarang cuma isi ulang select box
// biar cocok sama nilai yang tersimpan, lalu load checkbox on/off.
function loadVoiceSetting() {
  loadAnnouncementTemplates();
  loadAnnouncementScopes();

  const uiLang = getUILanguage();
  const announceLang = getAnnounceLanguage();

  const uiSel = document.getElementById('uiLangSelect');
  const headerSel = document.getElementById('headerUiLangSelect');
  const announceSel = document.getElementById('announceLangSelect');
  const rateSel = document.getElementById('voiceRateSelect');
  const genderSel = document.getElementById('voiceGenderSelect');

  if (uiSel) uiSel.value = uiLang;
  if (headerSel) headerSel.value = uiLang;
  if (announceSel) announceSel.value = announceLang;
  if (rateSel) rateSel.value = String(getVoiceRate());
  if (genderSel) genderSel.value = getVoiceGenderPref();

  applyUILanguage();
  refreshAnnouncementTemplateInputs();

  const saved = localStorage.getItem('voiceSetting');
  if (!saved) return;

  const s = JSON.parse(saved);

  document.getElementById('annCountdown').checked = s.countdown;
  document.getElementById('annHorn').checked = s.horn;
  document.getElementById('annRaceStart').checked = s.raceStart;
  document.getElementById('annLapCount').checked = s.lapCount;
  document.getElementById('annLastLap').checked = s.lastLap;
  document.getElementById('annPosition').checked = s.position;
  document.getElementById('annFinishPosition').checked = s.finishPosition;
  document.getElementById('annRaceFinish').checked = s.raceFinish;
  document.getElementById('annBestLap').checked = s.bestLap;

  // Setting lama (sebelum v4.6) belum punya field ini -- biarkan default
  // HTML (checked) kalau memang belum pernah disimpan sebelumnya.
  if (s.qualifyLapTime !== undefined) {
    document.getElementById('annQualifyLapTime').checked = s.qualifyLapTime;
  }
}

let previousQualifiers = {};

function formatLapVoice(ms) {
  if (!ms || ms <= 0) return "-";
  return (ms / 1000).toFixed(2);
}

function startQualify() {
  // Sama seperti startRace() -- jaga-jaga kalau user mulai dari halaman
  // Qualifying duluan (bukan Race), suara tetap ter-unlock dari gesture ini.
  unlockAudio();

  previousQualifiers = {};
  fetch('/qualifyStart')
    .then(res => res.text())
    .then(text => {
      if (text === 'RACE_RUNNING') {
        showToast('Tidak bisa Start Qualifying -- Race sedang berjalan. Stop Race dulu.');
      }
      loadData();
    });
}

function stopQualify() {
  fetch('/qualifyStop').then(() => loadData());
}

function resetQualify() {
  document.getElementById('saveQualifyModal').style.display = 'flex';
}

function saveQualifyResult() {
  const name = document.getElementById('qualifyNameInput').value;

  if (!name) return;

  const dateTime = new Date().toLocaleString();

  fetch(
    '/saveQualify?name=' +
    encodeURIComponent(name) +
    '&datetime=' +
    encodeURIComponent(dateTime)
  )
  .then(() => {
    closeQualifyModal();

    fetch('/qualifyReset')
      .then(() => {
        document.getElementById('qualifyNameInput').value = "";
        loadData();
loadQualifyHistory();
showResultTab('qualify');
      });
  });
}

function closeQualifyModal() {
  document.getElementById('saveQualifyModal').style.display = 'none';
}

function discardQualify() {
  closeQualifyModal();

  fetch('/qualifyReset')
    .then(() => loadData());
}

// v4.5 FIX: dulu kirim ?index=<posisi array>, padahal firmware
// (handleQualifyDelete di .ino) sudah lama di-refactor buat baca
// parameter "id" (idDecimal) -- BUKAN "index". Akibatnya server SELALU
// balas 400 BAD_REQUEST dan tidak pernah benar-benar menghapus apapun;
// tombol Delete di Qualifying jadi TIDAK BERFUNGSI SAMA SEKALI sejak
// refactor itu. Sekarang idDecimal yang dikirim, cocok dengan firmware.
function deleteQualifier(idDecimal) {
  fetch('/qualifyDelete?id=' + idDecimal)
    .then(() => loadData());
}

function saveQualifySettings() {
  const maxLap = document.getElementById('qualifyMaxLapInput').value;
  const cooldown = document.getElementById('qualifyCooldownInput').value;
  const sort = document.getElementById('qualifySortInput').value;

  fetch('/qualifySettings?maxlap=' + maxLap + '&cooldown=' + cooldown + '&sort=' + sort)
    .then(() => loadData());
}

function renderQualify(data) {
  setQualifyStatusStyle(!!data.qualifyingRunning);
  updateQualifyButtons(!!data.qualifyingRunning);

  document.getElementById('qualifyMaxLapView').innerHTML =
    data.qualifyMaxLap;

  document.getElementById('qualifySortView').innerHTML =
    data.qualifySort === "best" ? "Best Lap" : "Lap Count";

if (document.activeElement.id !== 'qualifyMaxLapInput') {

  document.getElementById('qualifyMaxLapInput').value =

    data.qualifyMaxLap;

}

if (document.activeElement.id !== 'qualifyCooldownInput') {

  document.getElementById('qualifyCooldownInput').value =

    data.qualifyCooldown;

}

if (document.activeElement.id !== 'qualifySortInput') {

  document.getElementById('qualifySortInput').value =

    data.qualifySort;

}

  let rows = "";
  let flashIds = [];

  if (!data.qualifiers || data.qualifiers.length === 0) {
    rows = `
      <tr>
        <td colspan="8" style="color:#94a3b8;">
          No driver detected yet
        </td>
      </tr>
    `;
  } else {
    data.qualifiers.forEach((q, index) => {
      let avg = q.averageLapTime || 0;

      rows += `
        <tr id="qualifier-row-${q.idDecimal}" class="leaderboard-row">
          <td class="${index === 0 ? 'rank1' : ''}">${rankCellHtml(index + 1)}</td>
          <td>${q.name}</td>
          <td>${q.idDecimal}</td>
          <td>${q.laps} / ${data.qualifyMaxLap}</td>
          <td class="bestlap-cell">${formatMs(q.bestLapTime)}</td>
          <td>${formatMs(q.lastLapTime)}</td>
          <td>${formatMs(avg)}</td>
          <td>
            <button class="btn-delete" onclick="deleteQualifier(${q.idDecimal})">
              Delete
            </button>
          </td>
        </tr>
      `;

      if (q.lapLogs && q.lapLogs.length > 0) {
        rows += `
          <tr>
            <td colspan="8" style="text-align:left;background:rgba(2,6,23,0.45);">
              ${indicatorStatsHtml(q, data.qualifiers[0], qualifyGapToLeaderText, q.lapLogs)}
        `;

        q.lapLogs.forEach((lap, i) => {
          rows += `
            <span class="lap-chip">
              Lap ${i + 1}: ${formatMs(lap)}
            </span>
          `;
        });

        rows += `
            </td>
          </tr>
        `;
      }

      const old = previousQualifiers[q.idDecimal];

      if (old && q.laps > old.laps && q.lastLapTime > 0) {
        flashIds.push('qualifier-row-' + q.idDecimal);

        // v4.6: dulu tidak ada checkbox on/off buat announcement ini
        // sama sekali (selalu bunyi tiap qualifier nge-lap) -- sekarang
        // ikut sistem template modular + bisa dimatikan lewat
        // "Qualifying Lap Time" di Settings, konsisten dengan
        // announcement lain. Untuk bahasa Indonesia, titik desimal tetap
        // dibaca "koma" (bukan diam/aneh dibacakan TTS) sebelum masuk
        // ke {lastlap}.
        if (document.getElementById('annQualifyLapTime').checked && passesAnnouncementScope('qualifyLapTime', index + 1)) {
          const lang = getAnnounceLanguage();
          const lastlap = lang === 'id'
            ? formatLapVoice(q.lastLapTime).replace('.', ' koma ')
            : formatLapVoice(q.lastLapTime);

          speakAnnouncement('qualifyLapTime', { name: q.name, lastlap: lastlap, lap: q.laps });
        }
      }

      previousQualifiers[q.idDecimal] = {
        laps: q.laps,
        bestLapTime: q.bestLapTime
      };
    });
  }

  document.getElementById('qualifyBody').innerHTML = rows;
  flashIds.forEach(flashRow);
}

function showResultTab(type) {
  document.getElementById('raceResultBox').style.display = 'none';
  document.getElementById('qualifyResultBox').style.display = 'none';
  document.getElementById('raceDetailBox').style.display = 'none';

  if (type === 'race') {
    document.getElementById('raceResultBox').style.display = '';
    loadHistory();
  }

  if (type === 'qualify') {
    document.getElementById('qualifyResultBox').style.display = '';
    loadQualifyHistory();
  }
}

function loadQualifyHistory() {
  fetch('/qualifyHistory')
    .then(response => response.json())
    .then(data => {
      let html = '';

      data.history.forEach((q) => {
        html += `
          <div onclick="openQualifyDetail('${encodeURIComponent(q.file)}')"
            style="
              background:#020617;
              border:1px solid rgba(34,197,94,0.35);
              border-radius:14px;
              padding:16px;
              margin-bottom:12px;
              cursor:pointer;
            ">
            <div style="font-size:20px;font-weight:900;color:#22c55e;">
              ${q.name}
            </div>
            <div style="color:#94a3b8;margin-top:6px;">
              ${q.datetime}
            </div>
          </div>
        `;
      });

      if (html === '') {
        html = `<div style="color:#94a3b8;">No qualify result saved yet</div>`;
      }

      document.getElementById('qualifyHistoryList').innerHTML = html;
    });
}

function openQualifyDetail(file) {
  fetch('/qualifyDetail?file=' + decodeURIComponent(file))
    .then(response => response.json())
    .then(data => {
      currentQualifyDetailData = data;

      let html = `
        <button class="btn-stop"
          onclick="loadQualifyHistory()"
          style="margin-bottom:14px;">
          Back to Qualify Result List
        </button>

        <h2>${data.qualifyName}</h2>

        <div style="color:#94a3b8;margin-bottom:15px;">
          ${data.startDateTime}
        </div>

        <button class="btn-save" onclick="downloadQualifyCsv()" style="margin-bottom:15px;">
          Download CSV
        </button>

        <table>
          <thead>
            <tr>
              <th>${UI_TEXT[getUILanguage()].thQualPos}</th>
              <th>${UI_TEXT[getUILanguage()].thQualDriver}</th>
              <th>${UI_TEXT[getUILanguage()].thQualLap}</th>
              <th>${UI_TEXT[getUILanguage()].thQualBest}</th>
              <th>${UI_TEXT[getUILanguage()].thQualLast}</th>
              <th>${UI_TEXT[getUILanguage()].thQualAvg}</th>
            </tr>
          </thead>
          <tbody>
      `;

      data.qualifiers.forEach(q => {
        html += `
          <tr>
            <td>${rankCellHtml(q.rank)}</td>
            <td>${q.name}</td>
            <td>${q.laps}</td>
            <td>${formatMs(q.bestLapTime)}</td>
            <td>${formatMs(q.lastLapTime)}</td>
            <td>${formatMs(q.averageLapTime)}</td>
          </tr>
        `;

        if (q.lapLogs && q.lapLogs.length) {
          html += `
            <tr>
              <td colspan="6" style="text-align:left;">
                ${indicatorStatsHtml(q, data.qualifiers[0], qualifyGapToLeaderText, q.lapLogs)}
          `;

          q.lapLogs.forEach((lap, i) => {
            html += `
              <span class="lap-chip">
                Lap ${i + 1} : ${formatMs(lap)}
              </span>
            `;
          });

          html += `
              </td>
            </tr>
          `;
        }
      });

      html += `
        </tbody></table>
      `;

      document.getElementById('qualifyHistoryList').innerHTML = html;
    });
}


loadVoiceSetting();

// Pulihkan pilihan bahasa dari kunjungan sebelumnya (kalau ada), baru
// baru render pertama kali biar label sudah sesuai dari awal.
try {
  const savedLang = localStorage.getItem('yurlaps_ui_lang');
  if (savedLang === 'id' || savedLang === 'en') setUILanguage(savedLang);
} catch (e) {}

updateClock();
loadData();

setInterval(updateClock, 1000);

// --- Realtime via WebSocket ---
// Sebelumnya di sini polling /data tiap 250ms. Sekarang server yang push
// data begitu ada perubahan (lap baru, race start/stop, dll), jadi lebih
// realtime (nggak nunggu siklus polling) dan lebih hemat CPU di board
// (server nggak kerja generate JSON kalau memang tidak ada yang berubah).
let ws = null;
let wsConnected = false;

function connectWebSocket() {
  const wsUrl = `ws://${location.hostname}:81/`;
  ws = new WebSocket(wsUrl);

  ws.onopen = () => {
    wsConnected = true;
    console.log('WebSocket connected');
  };

  ws.onmessage = (event) => {
    try {
      const data = JSON.parse(event.data);
      if (data.type === 'live') {
        renderLiveData(data);
      } else if (data.type === 'terminalEntry') {
        addTerminalEntry(data.idDecimal, data.hits, data.quality);
      } else {
        renderData(data);
      }
    } catch (e) {
      console.error('WS parse error', e);
    }
  };

  ws.onclose = () => {
    wsConnected = false;
    console.log('WebSocket disconnected, retrying in 1s...');
    setTimeout(connectWebSocket, 1000);
  };

  ws.onerror = () => {
    ws.close();
  };
}

connectWebSocket();

// Kalau jendela ini dibuka sebagai overlay REPLAY (?overlay=1&replay=<file>),
// langsung mulai simulasi dari file race tersimpan -- tidak perlu tunggu
// data live sama sekali (WS tetap konek di atas, tapi diabaikan
// selama replay berjalan, lihat overlayReplayActive di renderOverlay()).
(function () {
  const replayFile = new URLSearchParams(location.search).get('replay');
  if (replayFile) startOverlayReplay(replayFile);
})();

// Fallback: kalau WebSocket lagi terputus (WiFi glitch dsb), tetap ada
// polling pelan (2 detik) supaya UI tidak benar-benar mati sampai WS
// berhasil reconnect. Saat WS aktif, ini tidak menambah beban karena tidak
// melakukan apa-apa selain cek status koneksi.
setInterval(() => {
  if (!wsConnected) {
    loadData();
  }
}, 2000);

// Browser mobile (terutama kalau layar dikunci / tab dipindah ke
// background lama) kadang MEMUTUS koneksi jaringan tab secara paksa tanpa
// memicu event `onclose` milik WebSocket sama sekali -- akibatnya
// `wsConnected` tetap kebaca `true` padahal socket-nya sebenarnya sudah
// mati, dan UI kelihatan "freeze" (tidak ada data baru masuk) sampai
// halaman di-refresh manual. `visibilitychange` mendeteksi begitu tab
// balik jadi aktif lagi, lalu paksa reconnect kalau socket bukan cuma
// OPEN secara state (readyState) -- fix ini di sisi BROWSER, saling
// melengkapi dengan enableHeartbeat() di sisi firmware (yang menangani
// socket zombie dari sudut pandang device).
document.addEventListener('visibilitychange', () => {
  if (document.visibilityState !== 'visible') return;

  if (!ws || ws.readyState !== WebSocket.OPEN) {
    console.log('Tab active again, WS not OPEN -- forcing reconnect');
    connectWebSocket();
  }
});

// --- Device Diagnostics (menu Settings) ---
function formatBytes(n) {
  if (typeof n !== 'number') return '-';
  if (n >= 1024) return (n / 1024).toFixed(1) + ' KB';
  return n + ' B';
}

function formatUptime(totalSeconds) {
  if (typeof totalSeconds !== 'number') return '-';

  const h = Math.floor(totalSeconds / 3600);
  const m = Math.floor((totalSeconds % 3600) / 60);
  const s = totalSeconds % 60;

  let out = '';
  if (h > 0) out += h + 'h ';
  out += m + 'm ' + s + 's';
  return out;
}

function loadDiagnostics() {
  fetch('/diagnostics')
    .then(r => r.json())
    .then(d => {
      document.getElementById('diagFreeHeap').textContent = formatBytes(d.freeHeap);
      document.getElementById('diagMinFreeHeap').textContent = formatBytes(d.minFreeHeap);
      document.getElementById('diagLargestBlock').textContent = formatBytes(d.maxAllocHeap);
      document.getElementById('diagWsClients').textContent = d.wsClients;
      document.getElementById('diagUptime').textContent = formatUptime(d.uptimeSec);
    })
    .catch(() => {});
}

// Auto-refresh tiap 5 detik, tapi CUMA selagi tab Settings yang sedang
// dibuka -- supaya tidak nambah beban request kalau user lagi di tab lain.
setInterval(() => {
  const settingPage = document.getElementById('settingPage');
  if (settingPage && settingPage.classList.contains('active')) {
    loadDiagnostics();
  }
}, 5000);

loadDiagnostics();

</script>

<div id="saveRaceModal"
style="
display:none;
position:fixed;
inset:0;
background:rgba(0,0,0,.7);
z-index:999;
align-items:center;
justify-content:center;
">

  <div class="card"
       style="width:420px;max-width:90%;">

      <h2 id="h2SaveRaceModal">Save Race Result</h2>

      <input
        id="raceNameInput"
        placeholder="Race Name">

      <br><br>

      <button
        id="btnSaveRace"
        class="btn-save"
        onclick="saveRaceResult()">

        Save Race

      </button>
      <button
  id="btnDiscardRace"
  class="btn-stop"
  onclick="discardRace()">
  Don't Save
</button>

      <button
        id="btnCancelSaveRace"
        class="btn-delete"
        onclick="closeSaveModal()">

        Cancel

      </button>

  </div>

</div>

<div id="saveQualifyModal"
style="
display:none;
position:fixed;
inset:0;
background:rgba(0,0,0,.7);
z-index:999;
align-items:center;
justify-content:center;
">

  <div class="card" style="width:420px;max-width:90%;">
    <h2 id="h2SaveQualifyModal">Save Qualify Result</h2>

    <input id="qualifyNameInput" placeholder="Qualify Name">

    <br><br>

    <button id="btnSaveQualify" class="btn-save" onclick="saveQualifyResult()">
      Save Qualify
    </button>

    <button id="btnDiscardQualify" class="btn-stop" onclick="discardQualify()">
      Don't Save
    </button>

    <button id="btnCancelSaveQualify" class="btn-delete" onclick="closeQualifyModal()">
      Cancel
    </button>
  </div>
</div>

</body>
</html>
)rawliteral";